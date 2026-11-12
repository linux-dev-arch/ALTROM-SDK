
#include <stdint.h>
#include "../sdk/sdk.h"
#define GRID_W 20
#define GRID_H 15
#define MAX_SNAKE (GRID_W * GRID_H)

#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

typedef struct {
    int16_t x;
    int16_t y;
} Point;

static Point snake[MAX_SNAKE];
static uint8_t occupied[GRID_H][GRID_W];

static Point food;

static int snake_length;
static int direction;
static int next_direction;
static int score;
static int game_over;

static uint32_t rng_state = 1234567;


static uint32_t random_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void spawn_food(void)
{
    if (snake_length >= MAX_SNAKE)
        return;

    do {
        food.x = random_next() % GRID_W;
        food.y = random_next() % GRID_H;
    } while (occupied[food.y][food.x]);
}


#define CELL_SIZE 8
#define GAME_X 0
#define GAME_Y 24

static void snake_draw(void)
{
    fb_clear(0x0000);

    // Food
    fb_rect(
        GAME_X + food.x * CELL_SIZE,
        GAME_Y + food.y * CELL_SIZE,
        CELL_SIZE, CELL_SIZE,
        0xF800
    );

    // Snake
    for (int i = 0; i < snake_length; i++) {
        fb_rect(
            GAME_X + snake[i].x * CELL_SIZE,
            GAME_Y + snake[i].y * CELL_SIZE,
            CELL_SIZE, CELL_SIZE,
            0x07E0
        );
    }

    fb_flush();
}



static void snake_delay(void)
{
    delay_ms(200);
}

void snake_init(void)
{
    snake_length = 3;

    snake[0] = (Point){10, 7};
    snake[1] = (Point){9, 7};
    snake[2] = (Point){8, 7};

    occupied[7][10] = 1;
    occupied[7][9] = 1;
    occupied[7][8] = 1;

    direction = DIR_RIGHT;
    next_direction = DIR_RIGHT;

    score = 0;
    game_over = 0;

    spawn_food();
}

void snake_set_direction(int dir)
{
    if ((direction == DIR_UP && dir == DIR_DOWN) ||
        (direction == DIR_DOWN && dir == DIR_UP) ||
        (direction == DIR_LEFT && dir == DIR_RIGHT) ||
        (direction == DIR_RIGHT && dir == DIR_LEFT))
        return;

    next_direction = dir;
}

void snake_move(void)
{
    direction = next_direction;

    Point head = snake[0];

    switch (direction) {
        case DIR_UP:    head.y--; break;
        case DIR_DOWN:  head.y++; break;
        case DIR_LEFT:  head.x--; break;
        case DIR_RIGHT: head.x++; break;
    }

    if (head.x < 0 || head.x >= GRID_W ||
        head.y < 0 || head.y >= GRID_H) {
        game_over = 1;
        return;
    }

    int eating = (head.x == food.x &&
                  head.y == food.y);

    Point tail = snake[snake_length - 1];

    if (occupied[head.y][head.x] &&
        (eating || head.x != tail.x || head.y != tail.y)) {
        game_over = 1;
        return;
    }

    if (!eating)
        occupied[tail.y][tail.x] = 0;

    if (eating)
        snake_length++;

    // Circular buffer can replace this loop later.
    for (int i = snake_length - 1; i > 0; i--)
        snake[i] = snake[i - 1];

    snake[0] = head;
    occupied[head.y][head.x] = 1;

    if (eating) {
        score += 10;
        spawn_food();
    }
}

void main(void)
{
    fb_clear(COL_BLACK);
    fb_flush();
    snake_init();

    while (!game_over) {
        snake_move();
        snake_draw();
        snake_delay();
    }

    draw_string8(40, 80, "GAME OVER", COL_WHITE,COL_BLACK,0,0);
    fb_flush();

    snake_delay();

    jump(0x70000000);
}
