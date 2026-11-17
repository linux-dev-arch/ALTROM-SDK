
#include "../../../sdk/sdk.h"

#define BG          0x0841
#define C_WHITE     0xFFFF
#define C_GREY      0x7BEF
#define C_DIM       0x3186
#define C_CYAN      0x07FF
#define C_GREEN     0x07E0
#define C_YELLOW    0xFFE0
#define C_ORANGE    0xFD20
#define C_RED       0xF800
#define C_MAGENTA   0xF81F
#define C_BLUE      0x5DFF

#define CELL        48
#define GAP         5
#define GRID_SIZE   (CELL * 4 + GAP * 3)
#define GRID_X      ((LCD_W - GRID_SIZE) / 2)
#define GRID_Y      68

#define KEY_UP      0x01
#define KEY_DOWN    0x02
#define KEY_LEFT    0x04
#define KEY_RIGHT   0x08
#define KEY_RESTART 0x10
#define KEY_EXIT    0x20

static uint32_t board[4][4];

static uint32_t score;
static uint32_t best_score;
static uint32_t rng;

static int game_over;
static int game_won;

/* -------------------------------------------------
   RANDOM NUMBER GENERATOR
------------------------------------------------- */

static uint32_t random32(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;

    return rng;
}

/* -------------------------------------------------
   KEYPAD
------------------------------------------------- */

static uint8_t read_keys(void)
{
    uint8_t keys = 0;

    if (KP_CENTER_UP == 0)     keys |= KEY_UP;
    if (KP_CENTER_DOWN == 0)     keys |= KEY_DOWN;
    if (KP_CENTER_LEFT == 0)     keys |= KEY_LEFT;
    if (KP_CENTER_RIGHT == 0)     keys |= KEY_RIGHT;
    if (KP_OK == 0)     keys |= KEY_RESTART;
    if (KP_LEFT == 0) keys |= KEY_EXIT;

    return keys;
}

/* -------------------------------------------------
   SPAWN TILE
------------------------------------------------- */

static void spawn_tile(void)
{
    uint32_t r = random32();
    int start = (int)(r & 15);

    for (int n = 0; n < 16; n++) {
        watchdog_feed();

        int index = (start + n) & 15;
        int row = index >> 2;
        int col = index & 3;

        if (board[row][col] == 0) {
            board[row][col] =
                ((r >> 8) & 7) == 0 ? 4 : 2;
            return;
        }
    }
}

/* -------------------------------------------------
   RESET
------------------------------------------------- */

static void reset_game(void)
{
    for (int y = 0; y < 4; y++) {
        watchdog_feed();

        for (int x = 0; x < 4; x++) {
            watchdog_feed();
            board[y][x] = 0;
        }
    }

    score = 0;
    game_over = 0;
    game_won = 0;

    spawn_tile();
    spawn_tile();
}

/* -------------------------------------------------
   PROCESS ONE ROW/COLUMN
------------------------------------------------- */

static int merge_line(uint32_t line[4])
{
    uint32_t original[4];
    uint32_t compact[4] = {0, 0, 0, 0};
    uint32_t result[4] = {0, 0, 0, 0};

    int count = 0;
    int output = 0;

    for (int i = 0; i < 4; i++) {
        watchdog_feed();
        original[i] = line[i];

        if (line[i] != 0)
            compact[count++] = line[i];
    }

    for (int i = 0; i < count; i++) {
        watchdog_feed();

        if (i + 1 < count &&
            compact[i] == compact[i + 1]) {

            result[output] = compact[i] * 2;
            score += result[output];

            i++;
        } else {
            result[output] = compact[i];
        }

        output++;
    }

    int changed = 0;

    for (int i = 0; i < 4; i++) {
        watchdog_feed();

        line[i] = result[i];

        if (line[i] != original[i])
            changed = 1;
    }

    return changed;
}

/* -------------------------------------------------
   MOVE BOARD

   0 = UP
   1 = DOWN
   2 = LEFT
   3 = RIGHT
------------------------------------------------- */

static int move_board(int direction)
{
    int changed = 0;

    for (int line = 0; line < 4; line++) {
        watchdog_feed();

        uint32_t values[4];

        for (int k = 0; k < 4; k++) {
            watchdog_feed();

            int row, col;

            if (direction == 0) {
                row = k;
                col = line;
            } else if (direction == 1) {
                row = 3 - k;
                col = line;
            } else if (direction == 2) {
                row = line;
                col = k;
            } else {
                row = line;
                col = 3 - k;
            }

            values[k] = board[row][col];
        }

        if (merge_line(values))
            changed = 1;

        for (int k = 0; k < 4; k++) {
            watchdog_feed();

            int row, col;

            if (direction == 0) {
                row = k;
                col = line;
            } else if (direction == 1) {
                row = 3 - k;
                col = line;
            } else if (direction == 2) {
                row = line;
                col = k;
            } else {
                row = line;
                col = 3 - k;
            }

            board[row][col] = values[k];
        }
    }

    return changed;
}

/* -------------------------------------------------
   CHECK GAME STATE
------------------------------------------------- */

static int has_moves(void)
{
    for (int y = 0; y < 4; y++) {
        watchdog_feed();

        for (int x = 0; x < 4; x++) {
            watchdog_feed();

            uint32_t v = board[y][x];

            if (v == 0)
                return 1;

            if (x < 3 && v == board[y][x + 1])
                return 1;

            if (y < 3 && v == board[y + 1][x])
                return 1;
        }
    }

    return 0;
}

static void check_state(void)
{
    game_won = 0;

    for (int y = 0; y < 4; y++) {
        watchdog_feed();

        for (int x = 0; x < 4; x++) {
            watchdog_feed();

            if (board[y][x] >= 2048)
                game_won = 1;
        }
    }

    game_over = !has_moves();
}

/* -------------------------------------------------
   NUMBER TO STRING
------------------------------------------------- */

static void number_string(uint32_t value, char *buffer)
{
    char temp[11];
    int count = 0;

    do {
        watchdog_feed();

        temp[count++] = '0' + (value % 10);
        value /= 10;

    } while (value && count < 10);

    for (int i = 0; i < count; i++) {
        watchdog_feed();
        buffer[i] = temp[count - i - 1];
    }

    buffer[count] = '\0';
}

/* -------------------------------------------------
   TILE COLORS
------------------------------------------------- */

static uint16_t tile_color(uint32_t value)
{
    switch (value) {
        case 2:    return C_GREY;
        case 4:    return C_CYAN;
        case 8:    return C_GREEN;
        case 16:   return C_YELLOW;
        case 32:   return C_ORANGE;
        case 64:   return C_RED;
        case 128:  return C_MAGENTA;
        case 256:  return C_BLUE;
        case 512:  return C_CYAN;
        case 1024: return C_YELLOW;
        case 2048: return C_WHITE;
        default:   return C_MAGENTA;
    }
}

/* -------------------------------------------------
   DRAW TILE NUMBER
------------------------------------------------- */

static void draw_tile_number(int x, int y, uint32_t value)
{
    char buffer[12];

    number_string(value, buffer);

    int length = 0;

    while (buffer[length]) {
        watchdog_feed();
        length++;
    }

    int width = (length - 1) * 9 + 8;

    int tx = x + (CELL - width) / 2;
    int ty = y + (CELL - 8) / 2;

    draw_string8(tx, ty, buffer,
                 tile_color(value), BG, 1, 0);
}

/* -------------------------------------------------
   RENDER GAME
------------------------------------------------- */

static void draw_game(void)
{
    watchdog_feed();
    fb_clear(BG);

    draw_string8(12, 10, "ALTROM 2048",
                 C_CYAN, BG, 1, 0);

    draw_string8(12, 32, "SCORE",
                 C_GREY, BG, 1, 0);

    char score_text[12];
    number_string(score, score_text);

    draw_string8(60, 32, score_text,
                 C_WHITE, BG, 1, 0);

    draw_string8(130, 32, "BEST",
                 C_GREY, BG, 1, 0);

    char best_text[12];
    number_string(best_score, best_text);

    draw_string8(168, 32, best_text,
                 C_WHITE, BG, 1, 0);

    fb_rect(12, 53, LCD_W - 24, 1, C_DIM);

    for (int y = 0; y < 4; y++) {
        watchdog_feed();

        for (int x = 0; x < 4; x++) {
            watchdog_feed();

            int tx = GRID_X + x * (CELL + GAP);
            int ty = GRID_Y + y * (CELL + GAP);

            uint32_t value = board[y][x];

            uint16_t color = value ?
                tile_color(value) : C_DIM;

            fb_rect_outline(tx, ty, CELL, CELL, color);

            if (value)
                draw_tile_number(tx, ty, value);
        }
    }

    if (game_over) {
        draw_string8(12, 290, "GAME OVER",
                     C_RED, BG, 1, 0);

        draw_string8(12, 304, "5 RESTART LEFT EXIT",
                     C_WHITE, BG, 1, 0);

    } else if (game_won) {
        draw_string8(12, 290, "2048 REACHED!",
                     C_GREEN, BG, 1, 0);

        draw_string8(12, 304, "KEEP PLAYING",
                     C_WHITE, BG, 1, 0);

    } else {
        draw_string8(12, 290, "2/4/6/8 MOVE",
                     C_GREY, BG, 1, 0);

        draw_string8(12, 304, "LEFT EXIT  5 RESTART",
                     C_GREY, BG, 1, 0);
    }

    watchdog_feed();
    fb_flush();
    watchdog_feed();
}

/* -------------------------------------------------
   APP ENTRY POINT
------------------------------------------------- */

int main(void)
{
    rng = 0x9E3779B9u ^ WDT_TIMER ^ KP_STATUS;

    if (rng == 0)
        rng = 0x12345678;

    best_score = 0;

    reset_game();

    uint8_t previous = read_keys();

    draw_game();

    while (1) {
        watchdog_feed();

        uint8_t keys = read_keys();

        uint8_t pressed = keys & (uint8_t)(~previous);

        previous = keys;

        if (pressed & KEY_EXIT)
            jump(0x70000000);

        if (pressed & KEY_RESTART) {
            reset_game();
            draw_game();
            continue;
        }

        if (game_over)
            continue;

        int direction = -1;

        if (pressed & KEY_UP)
            direction = 0;
        else if (pressed & KEY_DOWN)
            direction = 1;
        else if (pressed & KEY_LEFT)
            direction = 2;
        else if (pressed & KEY_RIGHT)
            direction = 3;

        if (direction < 0)
            continue;

        if (move_board(direction)) {
            spawn_tile();

            if (score > best_score)
                best_score = score;

            check_state();
            draw_game();
        }
    }
}
