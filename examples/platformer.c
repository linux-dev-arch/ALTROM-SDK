#include "../sdk/sdk.h"
#include <stdint.h>

#define GAME_W LCD_W
#define GAME_H LCD_H

#define SKY       0x867D
#define WHITE     0xFFFF
#define BLACK     0x0000
#define GREEN     0x07E0
#define DARKGREEN 0x03E0
#define BROWN     0xA145
#define RED       0xF800
#define YELLOW    0xFFE0

#define PLAYER_W  10
#define PLAYER_H  14

#define MAX_PLATFORMS 8
/*
extern void fb_clear(uint16_t color);
extern void fb_flush(void);
extern void fb_rect(int x, int y, int width, int height, uint16_t color);
extern void fb_pixel(int x, int y, uint16_t color);
extern void fb_rect_outline(int x, int y, int width, int height,
                            uint16_t color);
*/
typedef struct {
    int x, y, w;
} Platform;

static Platform platforms[MAX_PLATFORMS];

static int px, py;
static int vy;
static int jumps;
static int speed;
static int score;
static int game_over;

static uint32_t rng = 0x12345678;

static uint32_t random32(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static int random_range(int min, int max)
{
    return min + (int)(random32() % (uint32_t)(max - min + 1));
}

static void draw_player(void)
{
    /* Body */
    fb_rect(px, py, PLAYER_W, PLAYER_H, YELLOW);

    /* Eyes */
    fb_pixel(px + 7, py + 3, BLACK);

    /* Feet */
    fb_rect(px + 1, py + PLAYER_H - 2, 3, 3, RED);
    fb_rect(px + 6, py + PLAYER_H - 2, 3, 3, RED);
}

static void draw_platforms(void)
{
    int i;

    for (i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].w <= 0)
            continue;

        fb_rect(platforms[i].x, platforms[i].y,
                platforms[i].w, 5, GREEN);

        fb_rect(platforms[i].x, platforms[i].y + 5,
                platforms[i].w, 3, BROWN);
    }
}

static void spawn_platform(int i, int x)
{
    platforms[i].x = x;
    platforms[i].y = random_range(GAME_H / 2, GAME_H - 35);
    platforms[i].w = random_range(35, 75);
}

static void reset_game(void)
{
    int i;

    px = GAME_W / 4;
    py = GAME_H / 2;
    vy = 0;
    jumps = 0;
    speed = 2;
    score = 0;
    game_over = 0;

    for (i = 0; i < MAX_PLATFORMS; i++) {
        spawn_platform(i, i * (GAME_W / 3));
    }

    platforms[0].x = 0;
    platforms[0].y = GAME_H - 35;
    platforms[0].w = GAME_W;
}

static void update_platforms(void)
{
    int i;

    for (i = 0; i < MAX_PLATFORMS; i++) {
        if (platforms[i].w <= 0)
            continue;

        platforms[i].x -= speed;

        if (platforms[i].x + platforms[i].w < 0) {
            int rightmost = 0;
            int j;

            for (j = 0; j < MAX_PLATFORMS; j++) {
                if (platforms[j].x + platforms[j].w > rightmost)
                    rightmost = platforms[j].x + platforms[j].w;
            }

            spawn_platform(i, rightmost + random_range(15, 45));
        }
    }
}

static void update_player(int jump_pressed)
{
    int old_y = py;
    int i;

    if (jump_pressed && jumps < 2) {
        vy = -8;
        jumps++;
    }

    vy += 1;

    if (vy > 9)
        vy = 9;

    py += vy;

    /* Platform landing */
    if (vy >= 0) {
        for (i = 0; i < MAX_PLATFORMS; i++) {
            int top = platforms[i].y;

            if (platforms[i].w <= 0)
                continue;

            if (px + PLAYER_W > platforms[i].x &&
                px < platforms[i].x + platforms[i].w &&
                old_y + PLAYER_H <= top &&
                py + PLAYER_H >= top) {

                py = top - PLAYER_H;
                vy = 0;
                jumps = 0;
            }
        }
    }

    if (py > GAME_H)
        game_over = 1;
}

static void draw_game(void)
{
    fb_clear(SKY);

    draw_platforms();
    draw_player();

    /* Simple score indicator */
    fb_rect(5, 5, score % (GAME_W - 10), 4, YELLOW);

    if (game_over) {
        fb_rect_outline(GAME_W / 2 - 45, GAME_H / 2 - 20,
                        90, 40, WHITE);
        fb_rect(GAME_W / 2 - 35, GAME_H / 2 - 10,
                70, 20, RED);
    }

    fb_flush();
}

/*
 * Replace these with your actual keypad polling definitions.
 * They must return nonzero while the key is pressed.
 */
/*
extern int KP_5;
extern int KP_RIGHT;
extern int KP_LEFT;
*/
void main(void)
{
    int previous_jump = 0;
    watchdog_feed();
    reset_game();

    while (1) {
	watchdog_feed();
        int jump_now = (KP_5 == 0);
        int jump_pressed = jump_now && !previous_jump;

        previous_jump = jump_now;

        if (KP_LEFT == 0)
            jump(0x70000000);

        if (game_over) {
            if (KP_RIGHT == 0)
                reset_game();

            draw_game();
            continue;
        }

        update_player(jump_pressed);
        update_platforms();

        score++;

        if (score % 500 == 0 && speed < 6)
            speed++;

        draw_game();
    }
}
