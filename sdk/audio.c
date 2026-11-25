/*
 * audio.c - speaker audio for ALTROM (MT6261 AFE + analog baseband audio path).
 * Include after timer.c (uses delay_us for all timing; falls back to a sleep()-based
 * guess if timer.c isn't present).
 *
 *   audio_init();     once - full power-up sequence, proven working on DZ09 (a different
 *                     phone on the same chip family), ported close to line-for-line
 *   audio_beep();     built-in sine-wave test tone, no PCM buffer needed
 *
 * PROVENANCE / HONESTY NOTE - read this before debugging a silent speaker:
 * Three different address families are involved here, verified to three different
 * degrees. Don't assume they're equally trustworthy:
 *   - PMU_BASE (0xA0700000) registers (SPK_CON0/3/6/7, speaker enable/volume/bias):
 *     SAME base this whole project has proven repeatedly (vibration, charger, ISINK,
 *     card power). SPK_CON0 itself has 8 direct literal references in the stock
 *     firmware - about as well corroborated as anything in this project.
 *   - ABBSYS_BASE (0xA0720000) registers (ABBA_AUDIODL_CON*, ABBA_VBITX_CON0; headset/
 *     handset buffer power-up, zero-crossion-detect, bias): the BASE address matches the
 *     normal address pattern of this chip (adjacent to PMU_BASE, same style as every
 *     other proven block), but the SPECIFIC sub-registers used here have NOT been found
 *     as direct literals in the stock firmware - plausible but unconfirmed.
 *   - AFE_BASE (0x82CD0000) and MD2GCONFG_BASE (0x82C00000) (AFE_MCU_CON0/1, AFE_AMCU_*,
 *     AFE_VMCU_*, AFE_DAC_TEST, MD2GSYS_CG_SET2/CLR2): a genuinely different, unusual
 *     address family from everything else in this whole project (everything else lives
 *     in 0xA0xx0000). A read-only peek of AFE_MCU_CON0 (0x82cd0210) was tested on real
 *     hardware before this was written: it returned a stable 0x00000000 with no hang,
 *     which is consistent with (but does not prove) this being the real register at its
 *     power-on-reset value. This is the part most likely to need correction if nothing
 *     happens on real hardware.
 * All of it is ported as close to line-for-line as reasonable from DZ09 (a different
 * phone sharing this chip family), not reinvented - DZ09's sequence is proven to drive a
 * real speaker on real MT6261 hardware, just not on *this* specific phone yet.
 */

#ifndef PMU_BASE
#define PMU_BASE 0xA0700000u
#endif
#ifndef ABBSYS_BASE
#define ABBSYS_BASE 0xA0720000u
#endif
#ifndef AFE_BASE
#define AFE_BASE 0x82CD0000u
#endif
#ifndef MD2GCONFG_BASE
#define MD2GCONFG_BASE 0x82C00000u
#endif

#define AU16(a) (*(volatile uint16_t *)(a))

/* ---- speaker control (PMU_BASE; the well-corroborated part) ---- */
#define SPK_CON0  AU16(PMU_BASE + 0x0D00)
#define SPK_CON3  AU16(PMU_BASE + 0x0D0C)
#define SPK_CON6  AU16(PMU_BASE + 0x0D18)
#define SPK_CON7  AU16(PMU_BASE + 0x0D1C)
#define RG_SPK_EN           (1u << 0)
#define RG_SPK_OUT_FLOAT_B  (1u << 11)
#define RG_SPK_IN_FLOAT_B   (1u << 14)
#define RG_SPK_VCM_SEL      (1u << 7)
#define RG_SPK_PBIAS        (1u << 10)
#define SPK_GAIN(v)         (((v) & 0x03) << 4)
#define SPK_GAIN_M6DB 0
#define SPK_GAIN_3DB  1
#define SPK_GAIN_6DB  2
#define SPK_GAIN_12DB 3

/* ---- analog baseband audio path (ABBSYS_BASE; pattern-consistent, unconfirmed) ---- */
#define ABBA_VBITX_CON0   AU16(ABBSYS_BASE + 0x0050)
#define ABBA_AUDIODL_CON0 AU16(ABBSYS_BASE + 0x0080)
#define ABBA_AUDIODL_CON1 AU16(ABBSYS_BASE + 0x0084)
#define ABBA_AUDIODL_CON2 AU16(ABBSYS_BASE + 0x0088)
#define ABBA_AUDIODL_CON4 AU16(ABBSYS_BASE + 0x0090)
#define ABBA_AUDIODL_CON5 AU16(ABBSYS_BASE + 0x0094)
#define ABBA_AUDIODL_CON7 AU16(ABBSYS_BASE + 0x009C)
#define ABBA_AUDIODL_CON8 AU16(ABBSYS_BASE + 0x00A0)
#define ABBA_AUDIODL_CON9 AU16(ABBSYS_BASE + 0x00A4)
#define ABBA_AUDIODL_CON10 AU16(ABBSYS_BASE + 0x00A8)
#define ABBA_AUDIODL_CON11 AU16(ABBSYS_BASE + 0x00AC)
#define ABBA_AUDIODL_CON12 AU16(ABBSYS_BASE + 0x00B0)
#define ABBA_AUDIODL_CON13 AU16(ABBSYS_BASE + 0x00B4)
#define ABBA_AUDIODL_CON14 AU16(ABBSYS_BASE + 0x00B8)
#define ABBA_AUDIODL_CON15 AU16(ABBSYS_BASE + 0x00BC)

#define RG_AUDIBIASPWRUP  (1u << 0)
#define RG_AUDHPRPWRUP    (1u << 2)
#define RG_AUDHPLPWRUP    (1u << 3)
#define RG_AUDHSPWRUP     (1u << 4)
#define RG_AUDDACRPWRUP   (1u << 5)
#define RG_AUDDACLPWRUP   (1u << 6)
#define RG_ULPGA_GAIN(v)  (((v) & 0x1F) << 6)
#define RG_LCLDO_TBST_EN  (1u << 1)
#define RG_DEPOP_CHARGEOPTION (1u << 5)
#define RG_DEPOP_VCM_EN   (1u << 0)
#define RG_AUDHSBIAS      (1u << 12)
#define AUD_ZCD_ENABLE    (1u << 0)
#define RG_GAINR(v) (((v) & 0x1F) << 0)
#define RG_GAINL(v) (((v) & 0x1F) << 5)
#define RG_GAINS(v) (((v) & 0x1F) << 10)
#define MINIMUM_AGAIN_SETTING 0x000Eu

/* ---- digital audio front-end (AFE_BASE; the unverified-but-not-hanging part) ---- */
#define AFE_VMCU_CON0 AU16(AFE_BASE + 0x0000)
#define AFE_VMCU_CON1 AU16(AFE_BASE + 0x000C)
#define AFE_VMCU_CON2 AU16(AFE_BASE + 0x0010)
#define AFE_VMCU_CON3 AU16(AFE_BASE + 0x001C)
#define AFE_AMCU_CON0 AU16(AFE_BASE + 0x0020)
#define AFE_AMCU_CON1 AU16(AFE_BASE + 0x0024)
#define AFE_AMCU_CON2 AU16(AFE_BASE + 0x002C)
#define AFE_DAC_TEST  AU16(AFE_BASE + 0x0030)
#define AFE_AMCU_CON5 AU16(AFE_BASE + 0x0180)
#define AFE_MCU_CON0  AU16(AFE_BASE + 0x0210)
#define AFE_MCU_CON1  AU16(AFE_BASE + 0x0214)

#define VIRQON       (1u << 0)
#define VRSDON       (1u << 7)
#define VTX_CK_PHASE (1u << 11)
#define VSDM_GAIN(v) (((v) & 0x3F) << 0)
#define AIRQON       (1u << 0)
#define ARAMPSP(v)   (((v) & 0x03) << 4)
#define AFS(v)       (((v) & 0x0F) << 6)
#define ASDM_GAIN(v) (((v) & 0x3F) << 0)
#define FREQ_DIV(v)  (((v) & 0x00FF) << 0)
#define AMP_DIV(v)   (((v) & 0x07) << 8)
#define ADAC_SINUS   (1u << 14)
#define VDAC_SINUS   (1u << 15)
#define AFE_ON       (1u << 0)
#define A_IF_UL_ON   (1u << 0)
#define UDSP_UL_ON   (1u << 1)
#define A_IF_DL_ON   (1u << 2)
#define UDSP_DL_ON   (1u << 3)
#define AFE_PATH_MASK (UDSP_DL_ON | A_IF_DL_ON | UDSP_UL_ON | A_IF_UL_ON)

#define MD2GSYS_CG_SET2 AU16(MD2GCONFG_BASE + 0x0018)
#define MD2GSYS_CG_CLR2 AU16(MD2GCONFG_BASE + 0x0028)
#define PDN_VAFE (1u << 8)

#define CLK_GATE_22C (*(volatile uint16_t *)0xA001022Cu)   /* same 0xA0010xxx family as every PD_* clock gate used elsewhere in ALTROM */

#ifdef TIMER_PRESENT
#define AU_US(x) delay_us(x)
#else
#define AU_US(x) sleep((x) / 5 + 1)   /* sleep()'s real unit is unknown; this is a rough placeholder - use timer.c if at all possible */
#endif

/* ---- ported from DZ09's afe.c, as literally as reasonable; no interrupt disable/
 * restore around the clock sequence below, since ALTROM never enables interrupts */

static void audio_dclk_on(void)
{
	CLK_GATE_22C = CLK_GATE_22C | 0x0013;

	ABBA_AUDIODL_CON8 |= RG_LCLDO_TBST_EN;
	ABBA_AUDIODL_CON10 = RG_AUDHSBIAS | 0x0406;         /* LDO: 2.4V */
	ABBA_AUDIODL_CON9 |= 0x0001;
	ABBA_AUDIODL_CON11 |= 0x0003;
	AU_US(1);

	ABBA_AUDIODL_CON7 |= 0x0003;                        /* "Audio" (music) path, not voice */
	ABBA_AUDIODL_CON0 |= (RG_AUDDACRPWRUP | RG_AUDDACLPWRUP);
	ABBA_AUDIODL_CON15 = 0x11A1;
	ABBA_AUDIODL_CON14 |= 0x0001;

	MD2GSYS_CG_CLR2 = PDN_VAFE;

	AFE_MCU_CON1 = (AFE_MCU_CON1 & ~AFE_PATH_MASK) | UDSP_DL_ON | A_IF_DL_ON;
	AFE_AMCU_CON0 |= AIRQON;
	AFE_MCU_CON0 = AFE_ON;
}

static void audio_switch_hs_on(void)
{
	uint16_t restore;

	ABBA_AUDIODL_CON11 |= 0x0003;
	ABBA_AUDIODL_CON7  |= 0x0003;
	ABBA_AUDIODL_CON0  |= (RG_AUDDACRPWRUP | RG_AUDDACLPWRUP);
	ABBA_AUDIODL_CON1  &= (uint16_t)~0x6000;
	ABBA_AUDIODL_CON1  |= 0x6000;                       /* enable HS zero-crossing detect */
	restore = ABBA_AUDIODL_CON13;
	ABBA_AUDIODL_CON13 = (uint16_t)((restore & ~0x7C00) | 0x2000);  /* 0dB, zcd on */
	ABBA_AUDIODL_CON12 |= AUD_ZCD_ENABLE;
	ABBA_AUDIODL_CON13 = restore;
	ABBA_AUDIODL_CON1  &= (uint16_t)~0x1FF0;
	ABBA_AUDIODL_CON1  |= 0x0800;
	ABBA_AUDIODL_CON0  |= RG_AUDHSPWRUP;                /* HS buffer power on */
}

static void audio_switch_int_amp_on(void)
{
	uint16_t restore;
	int16_t targ_gain, curr_gain, gain_step;

	SPK_CON3 = 0x4800;
	SPK_CON6 = 0x0988;             /* bypass mode */
	SPK_CON7 = 0x0180;             /* vcm fast startup */
	AU_US(2000);
	SPK_CON7 = 0x0100;

	ABBA_AUDIODL_CON0 |= RG_AUDHSPWRUP;
	AU_US(10);
	ABBA_AUDIODL_CON1 |= 0x0004;   /* speaker mode */
	ABBA_AUDIODL_CON12 &= (uint16_t)~AUD_ZCD_ENABLE;

	restore = ABBA_AUDIODL_CON13;
	ABBA_AUDIODL_CON13 = (uint16_t)((restore & ~0x7C00) | 0x2000);  /* 0dB */

	SPK_CON7 |= RG_SPK_VCM_SEL;    /* fast VCM track */
	SPK_CON0 |= RG_SPK_EN;
	AU_US(2000);
	SPK_CON3 |= RG_SPK_IN_FLOAT_B;
	SPK_CON7 &= (uint16_t)~RG_SPK_VCM_SEL;   /* vcm high-PSRR mode */

	targ_gain = (int16_t)(restore & 0x7C00);
	curr_gain = (int16_t)(ABBA_AUDIODL_CON13 & 0x7C00);
	gain_step = (int16_t)(targ_gain >= curr_gain ? 0x0400 : -0x0400);
	while (curr_gain != targ_gain) {
		curr_gain = (int16_t)(curr_gain + gain_step);
		ABBA_AUDIODL_CON13 = (uint16_t)(curr_gain | (restore & ~0x7C00));
		AU_US(500);
	}
	ABBA_AUDIODL_CON13 = restore;

	ABBA_AUDIODL_CON1  |= 0x6000;             /* zcd: handset mode */
	ABBA_AUDIODL_CON12 |= AUD_ZCD_ENABLE;
}

static void audio_chip_init(void)
{
	AFE_AMCU_CON1 = (uint16_t)(0x0C00 | AFS(0x08) | ARAMPSP(0));
	AFE_AMCU_CON5 = 0x0002;
	AFE_VMCU_CON3 = 0x0002;
	AFE_VMCU_CON2 = (uint16_t)(VTX_CK_PHASE | VSDM_GAIN(0x3C));
	AFE_AMCU_CON2 = (uint16_t)ASDM_GAIN(0x3C);
	AFE_VMCU_CON1 = VRSDON;

	ABBA_VBITX_CON0 |= (uint16_t)RG_ULPGA_GAIN(0x2);

	ABBA_AUDIODL_CON8 |= RG_LCLDO_TBST_EN;
	ABBA_AUDIODL_CON4  = (uint16_t)(0x198 | RG_DEPOP_CHARGEOPTION | RG_DEPOP_VCM_EN);
	AU_US(10);
	ABBA_AUDIODL_CON4 |= 0x0040;
	/* DZ09 notes: "should wait 2s~3s to charge cap" here on their hardware; audio_init()
	 * below uses 5ms (matching DZ09's own AFE_initialize, not this longer comment) - if
	 * the speaker pops or sounds wrong, try lengthening the final AU_US(5000) in
	 * audio_init() first. */
	ABBA_AUDIODL_CON10 = RG_AUDHSBIAS | 0x0406;
	ABBA_AUDIODL_CON9 |= 0x0001;

	SPK_CON0 = (uint16_t)((SPK_CON0 & ~SPK_GAIN(-1)) | SPK_GAIN(SPK_GAIN_M6DB));
}

void audio_init(void)
{
	audio_chip_init();
	audio_dclk_on();
	AFE_AMCU_CON1 = (uint16_t)((AFE_AMCU_CON1 & ~AFS(-1)) | AFS(0));  /* 8 kHz */

	audio_switch_hs_on();
	audio_switch_int_amp_on();

	SPK_CON0 = (uint16_t)(SPK_GAIN(3) | RG_SPK_EN);
	ABBA_AUDIODL_CON13 = (uint16_t)(RG_GAINS(0x10) | RG_GAINL(0x1F) | RG_GAINR(0x1F));
	AU_US(5000);
}

/*
 * Built-in sine-wave test tone: no PCM buffer, no sample feeding, just a register flip.
 *
 *   audio_tone(freq_div, amp_div, duration_ms)
 *     freq_div  0-255  - raw divider, lower = higher pitch (not datasheet-confirmed in
 *                         Hz; sweep it by ear - 22 was the first value tried and it beeped)
 *     amp_div   0-7    - raw amplitude divider, lower = louder (7 was the first value tried)
 *     duration_ms      - how long to hold the tone, in milliseconds
 *
 *   audio_beep() - the original fixed test tone (freq_div=22, amp_div=7, 50ms), kept so
 *                   existing code calling it still works unchanged
 */
void audio_tone(uint8_t freq_div, uint8_t amp_div, uint32_t duration_ms)
{
	AFE_DAC_TEST = (uint16_t)(VDAC_SINUS | ADAC_SINUS | AMP_DIV(amp_div) | FREQ_DIV(freq_div));
	AU_US(duration_ms * 1000);
	AFE_DAC_TEST &= (uint16_t)~(VDAC_SINUS | ADAC_SINUS);
}

void audio_beep(void)
{
	audio_tone(22, 7, 50);
}
