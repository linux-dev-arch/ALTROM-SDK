
#include "../sdk/sdk.h"

#define SCREEN_W 320//320
#define SCREEN_H 240//240

#define LCD_W 240
#define LCD_H 320

#define RAY_COUNT 40
#define COLUMN_WIDTH 8

#define MAP_W 10
#define MAP_H 10

#define FP 256

#define MOVE_SPEED 64
#define TURN_SPEED 3

#define SKY_COLOR    0x1082
#define FLOOR_COLOR  0x4208
#define WALL_COLOR   0xBDF7
#define WALL_DARK    0x7BEF

#define COL_WHITE    0xFFFF
#define COL_BLACK    0x0000

/*
 * Landscape framebuffer coordinates are rotated
 * 90 degrees clockwise into the portrait LCD.
 *
 * Landscape (x,y) -> LCD (239-y,x)
 */

static inline void put_pixel_rotated(int x, int y, uint16_t color)
{
    int lcd_x = 239 - y;
    int lcd_y = x;

    FB[lcd_y * LCD_W + lcd_x] = color;
}

static void rect_rotated(int x, int y, int w, int h,
                         uint16_t color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }

    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;

    if (w <= 0 || h <= 0) return;

    for (int py = y; py < y + h; py++)
    {
        for (int px = x; px < x + w; px++)
        {
            put_pixel_rotated(px, py, color);
        }
    }
}

static const uint8_t world[MAP_H][MAP_W] =
{
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};

/* Player position in Q8.8 */
static int32_t posX = 384;
static int32_t posY = 384;

/* Direction vectors in Q8.8 */
static int32_t dirX = 256;
static int32_t dirY = 0;

/* Camera plane */
static int32_t planeX = 0;
static int32_t planeY = 170;

/*
 * 64-entry sine table, Q8.8.
 * One full revolution = 64 steps.
 */
static const int16_t sin_table[64] =
{
      0,  25,  50,  74,  98, 121, 142, 163,
    181, 198, 213, 226, 237, 245, 251, 255,
    256, 255, 251, 245, 237, 226, 213, 198,
    181, 163, 142, 121,  98,  74,  50,  25,
      0, -25, -50, -74, -98,-121,-142,-163,
   -181,-198,-213,-226,-237,-245,-251,-255,
   -256,-255,-251,-245,-237,-226,-213,-198,
   -181,-163,-142,-121, -98, -74, -50, -25
};

static int angle = 0;

static int abs_i(int x)
{
    return x < 0 ? -x : x;
}

static void update_direction(void)
{
    dirX = sin_table[(angle + 16) & 63];
    dirY = sin_table[angle & 63];

    planeX = -(dirY * 170) / FP;
    planeY =  (dirX * 170) / FP;
}

static int wall_at(int x, int y)
{
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H)
        return 1;

    return world[y][x];
}

static void move_player(int32_t dx, int32_t dy)
{
    int nx = (posX + dx) / FP;
    int ny = (posY + dy) / FP;

    if (!wall_at(nx, posY / FP))
        posX += dx;

    if (!wall_at(posX / FP, ny))
        posY += dy;
}

static void draw_background(void)
{
    rect_rotated(0, 0, SCREEN_W, SCREEN_H / 2, SKY_COLOR);

    rect_rotated(0, SCREEN_H / 2,
                 SCREEN_W, SCREEN_H / 2, FLOOR_COLOR);
}

static void render(void)
{
    draw_background();

    for (int ray = 0; ray < RAY_COUNT; ray++)
    {
        int cameraX =
            ((2 * ray + 1 - RAY_COUNT) * FP) / RAY_COUNT;

        int32_t rayX = dirX + (planeX * cameraX) / FP;
        int32_t rayY = dirY + (planeY * cameraX) / FP;

        int mapX = posX / FP;
        int mapY = posY / FP;

        int stepX, stepY;

        int32_t deltaX, deltaY;
        int32_t sideX, sideY;

        if (rayX == 0)
        {
            deltaX = 0x7FFFFFFF;
            sideX = 0x7FFFFFFF;
            stepX = 0;
        }
        else
        {
            deltaX = 65536 / abs_i(rayX);

            if (rayX < 0)
            {
                stepX = -1;
                sideX = ((posX - mapX * FP) * deltaX) / FP;
            }
            else
            {
                stepX = 1;
                sideX = (((mapX + 1) * FP - posX) * deltaX) / FP;
            }
        }

        if (rayY == 0)
        {
            deltaY = 0x7FFFFFFF;
            sideY = 0x7FFFFFFF;
            stepY = 0;
        }
        else
        {
            deltaY = 65536 / abs_i(rayY);

            if (rayY < 0)
            {
                stepY = -1;
                sideY = ((posY - mapY * FP) * deltaY) / FP;
            }
            else
            {
                stepY = 1;
                sideY = (((mapY + 1) * FP - posY) * deltaY) / FP;
            }
        }

        int side = 0;
        int hit = 0;
        int32_t distance = FP;

        while (!hit)
        {
            if (sideX < sideY)
            {
                sideX += deltaX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideY += deltaY;
                mapY += stepY;
                side = 1;
            }

            if (wall_at(mapX, mapY))
            {
                hit = 1;
                distance = side ? sideY - deltaY : sideX - deltaX;
            }
        }

        if (distance < 16)
            distance = 16;

        int wallHeight = (SCREEN_H * FP) / distance;

        if (wallHeight > SCREEN_H)
            wallHeight = SCREEN_H;

        int drawStart = (SCREEN_H - wallHeight) / 2;

        int drawEnd = drawStart + wallHeight;

        uint16_t color = side ? WALL_DARK : WALL_COLOR;

        rect_rotated(ray * COLUMN_WIDTH,
                     drawStart,
                     COLUMN_WIDTH,
                     drawEnd - drawStart,
                     color);
    }

    /* Flush the portrait LCD framebuffer */
    fb_flush_rect(0, 0, LCD_W, LCD_H);
}

void main(void)
{
    angle = 0;
    update_direction();

    for (;;)
    {
        watchdog_feed();

        /* Landscape controls, assuming clockwise rotation */
        if (KP_CENTER_RIGHT == 0)
            move_player((dirX * MOVE_SPEED) / FP,
                        (dirY * MOVE_SPEED) / FP);

        if (KP_CENTER_LEFT == 0)
            move_player(-(dirX * MOVE_SPEED) / FP,
                        -(dirY * MOVE_SPEED) / FP);

        if (KP_CENTER_UP == 0)
        {
            angle = (angle - TURN_SPEED) & 63;
            update_direction();
        }

        if (KP_CENTER_DOWN == 0)
        {
            angle = (angle + TURN_SPEED) & 63;
            update_direction();
        }

        if (KP_LEFT == 0)
            break;

        render();

        delay_ms(16);
    }

    fb_clear(COL_BLACK);
    fb_flush();
    jump(0x70000000);
}

