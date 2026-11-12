#include "../../sdk/sdk.h"

#define CH8_CODE       0x002DA200
#define CH8_MEM        ((volatile uint8_t *)CH8_CODE)
#define CH8_REG_ADDR   0x002DB200
#define CH8_REG        ((volatile uint8_t *)CH8_REG_ADDR)
#define CH8_STACK_ADDR 0x002DB210
#define CH8_STACK      ((volatile uint16_t *)CH8_STACK_ADDR)

#define CH8_W 64
#define CH8_H 32

#define CH8_DISPLAY_ADDR 0x002DB240
#define CH8_DISPLAY ((volatile uint8_t *)CH8_DISPLAY_ADDR)

#define CH8_FONT_ADDR 0x000
#define CH8_ROM_ADDR  0x200

static const uint8_t font[80] = {
    0xF0,0x90,0x90,0x90,0xF0,
    0x20,0x60,0x20,0x20,0x70,
    0xF0,0x10,0xF0,0x80,0xF0,
    0xF0,0x10,0xF0,0x10,0xF0,
    0x90,0x90,0xF0,0x10,0x10,
    0xF0,0x80,0xF0,0x10,0xF0,
    0xF0,0x80,0xF0,0x90,0xF0,
    0xF0,0x10,0x20,0x40,0x40,
    0xF0,0x90,0xF0,0x90,0xF0,
    0xF0,0x90,0xF0,0x10,0xF0,
    0xF0,0x90,0xF0,0x90,0x90,
    0xE0,0x90,0xE0,0x90,0xE0,
    0xF0,0x80,0x80,0x80,0xF0,
    0xE0,0x90,0x90,0x90,0xE0,
    0xF0,0x80,0xF0,0x80,0xF0,
    0xF0,0x80,0xF0,0x80,0x80
};

static uint32_t rng = 0x12345678;

static uint8_t random_byte(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (uint8_t)rng;
}

/* Replace this mapping with your preferred keypad mapping. */
/*
static int chip8_key(void)
{
    if (KP_1 == 0) return 1;
    if (KP_2 == 0) return 2;
    if (KP_3 == 0) return 3;
    if (KP_4 == 0) return 4;
    if (KP_5 == 0) return 5;
    if (KP_6 == 0) return 6;
    if (KP_7 == 0) return 7;
    if (KP_8 == 0) return 8;
    if (KP_9 == 0) return 9;
    if (KP_0 == 0) return 0;
    if (KP_STAR == 0) return 10;
    if (KP_HASH == 0) return 11;
    return -1;
}
*/
static int chip8_key(void)
{
    if (KP_CENTER_UP == 0)    return 2;
    if (KP_CENTER_DOWN == 0)  return 8;
    if (KP_CENTER_LEFT == 0)  return 4;
    if (KP_CENTER_RIGHT == 0) return 6;
    if (KP_OK == 0)    return 5;

    if (KP_1 == 0)     return 1;
    if (KP_3 == 0)     return 3;
    if (KP_7 == 0)     return 7;
    if (KP_9 == 0)     return 9;
    if (KP_0 == 0)     return 0;
    if (KP_STAR == 0)  return 10;
    if (KP_HASH == 0)  return 11;

    return -1;
}

static void chip8_draw(void)
{
    int x, y;

    fb_clear(COL_BLACK);

    for (y = 0; y < CH8_H; y++) {
        for (x = 0; x < CH8_W; x++) {
            if (CH8_DISPLAY[y * CH8_W + x]) {
                fb_rect(
                    24 + x * 3,
                    112 + y * 3,
                    3, 3,
                    COL_WHITE
                );
            }
        }
    }

    fb_flush();
}

void main(void)
{
    volatile uint8_t *mem = CH8_MEM;
    volatile uint8_t *V = CH8_REG;
    volatile uint16_t *stack = CH8_STACK;
    volatile uint8_t *display = CH8_DISPLAY;

    uint16_t pc = CH8_ROM_ADDR;
    uint16_t I = 0;
    uint8_t sp = 0;

    uint8_t delay_timer = 0;
    uint8_t sound_timer = 0;

    uint16_t instr;
    uint8_t x, y, n, kk;
    uint16_t nnn;
    uint8_t key;

    int i;

    for (i = 0; i < 16; i++) {
        V[i] = 0;
        stack[i] = 0;
    }

    for (i = 0; i < 2048; i++)
        display[i] = 0;

    for (i = 0; i < 80; i++)
        mem[CH8_FONT_ADDR + i] = font[i];

    rng ^= (uint32_t)delay_ms; /* Seed is not critical. */

    while (KP_LEFT != 0) {
        watchdog_feed();

        if (pc > 4094)
            break;

        instr = ((uint16_t)mem[pc] << 8) | mem[pc + 1];

        x = (instr >> 8) & 0xF;
        y = (instr >> 4) & 0xF;
        n = instr & 0xF;
        kk = instr & 0xFF;
        nnn = instr & 0xFFF;

        pc += 2;

        switch (instr & 0xF000) {

        case 0x0000:
            if (instr == 0x00E0) {
                for (i = 0; i < 2048; i++)
                    display[i] = 0;
                chip8_draw();
            }
            else if (instr == 0x00EE) {
                if (sp == 0)
                    goto exit;
                pc = stack[--sp];
            }
            break;

        case 0x1000:
            pc = nnn;
            break;

        case 0x2000:
            if (sp >= 16)
                goto exit;
            stack[sp++] = pc;
            pc = nnn;
            break;

        case 0x3000:
            if (V[x] == kk) pc += 2;
            break;

        case 0x4000:
            if (V[x] != kk) pc += 2;
            break;

        case 0x5000:
            if (n == 0 && V[x] == V[y]) pc += 2;
            break;

        case 0x6000:
            V[x] = kk;
            break;

        case 0x7000:
            V[x] += kk;
            break;

        case 0x8000:
            switch (n) {
            case 0x0:
                V[x] = V[y];
                break;
            case 0x1:
                V[x] |= V[y];
                break;
            case 0x2:
                V[x] &= V[y];
                break;
            case 0x3:
                V[x] ^= V[y];
                break;
            case 0x4: {
                uint16_t sum = V[x] + V[y];
                V[15] = sum > 255;
                V[x] = (uint8_t)sum;
                break;
            }
            case 0x5:
                V[15] = V[x] >= V[y];
                V[x] -= V[y];
                break;
            case 0x6:
                V[15] = V[x] & 1;
                V[x] >>= 1;
                break;
            case 0x7:
                V[15] = V[y] >= V[x];
                V[x] = V[y] - V[x];
                break;
            case 0xE:
                V[15] = (V[x] >> 7) & 1;
                V[x] <<= 1;
                break;
            }
            break;

        case 0x9000:
            if (n == 0 && V[x] != V[y]) pc += 2;
            break;

        case 0xA000:
            I = nnn;
            break;

        case 0xB000:
            pc = nnn + V[0];
            break;

        case 0xC000:
            V[x] = random_byte() & kk;
            break;

        case 0xD000: {
            uint8_t row, bit;
            uint16_t sprite;
            uint8_t px, py;

            V[15] = 0;

            for (row = 0; row < n; row++) {
                if (I + row >= 4096)
                    break;

                sprite = mem[I + row];

                for (bit = 0; bit < 8; bit++) {
                    if (!(sprite & (0x80 >> bit)))
                        continue;

                    px = (V[x] + bit) & 63;
                    py = (V[y] + row) & 31;

                    if (display[py * 64 + px])
                        V[15] = 1;

                    display[py * 64 + px] ^= 1;
                }
            }

            chip8_draw();
            break;
        }

        case 0xE000:
            key = chip8_key();

            if (kk == 0x9E) {
                if (key == V[x]) pc += 2;
            }
            else if (kk == 0xA1) {
                if (key != V[x]) pc += 2;
            }
            break;

        case 0xF000:
            switch (kk) {

            case 0x07:
                V[x] = delay_timer;
                break;

            case 0x0A:
                key = chip8_key();
                if (key < 0)
                    pc -= 2;
                else
                    V[x] = key;
                break;

            case 0x15:
                delay_timer = V[x];
                break;

            case 0x18:
                sound_timer = V[x];
                break;

            case 0x1E:
                I += V[x];
                break;

            case 0x29:
                I = V[x] * 5;
                break;

            case 0x33:
                if (I + 2 < 4096) {
                    mem[I]     = V[x] / 100;
                    mem[I + 1] = (V[x] / 10) % 10;
                    mem[I + 2] = V[x] % 10;
                }
                break;

            case 0x55:
                for (i = 0; i <= x && I + i < 4096; i++)
                    mem[I + i] = V[i];
                break;

            case 0x65:
                for (i = 0; i <= x && I + i < 4096; i++)
                    V[i] = mem[I + i];
                break;
            }
            break;
        }

        /* Timers are approximate here; proper 60 Hz scheduling
           should be added separately. */
        if (delay_timer)
            delay_timer--;

        if (sound_timer)
            sound_timer--;
    }

exit:
    jump(0x70000000);
}
