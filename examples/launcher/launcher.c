
#include <stdint.h>
#include "../../sdk/sdk.h"
/* Existing ALTROM display functions */
//extern void fb_clear(uint16_t color);
//extern void fb_round_rect(int x, int y, int w, int h,
//                          int radius, uint16_t color);
//extern void fb_rect(int x, int y, int w, int h,
//                    uint16_t color);
//extern void draw_string12(int x, int y, const char *str,
//                          uint16_t fg, uint16_t bg);
//extern void fb_flush(void);

/* RGB565 theme */
#define UI_BG       0x10A2
#define UI_TEXT     0xEF1E
#define UI_MUTED    0x9CB3
#define UI_ACCENT   0xB5BC

#define SCREEN_W    240
#define SCREEN_H    320

#define MENU_COUNT  7
#define VISIBLE     6

#define ROW_HEIGHT  40
#define MENU_X      16
#define MENU_W      208

static const char *menu_items[MENU_COUNT] = {
    "File Manager",
    "Games",
    "Music Player",
    "Calculator",
    "Image Viewer",
    "Settings",
    "About ALTROM"
};

/* Draw one menu item */
static void draw_menu_item(int index, int y, int selected)
{
    int text_y = y + (ROW_HEIGHT - 12) / 2;

    if (selected) {
        /* Draw violet outline */
        fb_round_rect(
            MENU_X,
            y,
            MENU_W,
            ROW_HEIGHT - 2,
            8,
            UI_ACCENT
        );

        /* Inner background to make it an outline */
        fb_round_rect(
            MENU_X + 1,
            y + 1,
            MENU_W - 2,
            ROW_HEIGHT - 4,
            7,
            UI_BG
        );
    }

    draw_string12(
        MENU_X + 12,
        text_y,
        menu_items[index],
        UI_TEXT,
        UI_BG,
	1,
	0
    );
}

/* Draw the static main menu */
void draw_main_menu(void)
{
    int i;
    int start_y = (SCREEN_H - VISIBLE * ROW_HEIGHT) / 2;

    fb_clear(UI_BG);

    for (i = 0; i < VISIBLE; i++) {
        draw_menu_item(i, start_y + i * ROW_HEIGHT, i == 0);
    }

    fb_flush();
}

void main(void){
	for(;;){
		draw_main_menu();
		watchdog_feed();
		if(KP_LEFT==0){
			jump(0x70000000);
		}
	}
}
