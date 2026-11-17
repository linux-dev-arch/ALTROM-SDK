
#include <stdint.h>
#include <stdbool.h>

#include "../sdk/sdk.h"

#define W 240
#define H 320

#define BLACK       0x0000
#define WHITE       0xFFFF
#define CYAN        0x07FF
#define DARK_CYAN   0x03EF
#define GRAY        0x7BEF
#define DARK_GRAY   0x2104
#define RED         0xF800

#define FIELD_COUNT 6

enum
{
    FIELD_DAY,
    FIELD_MONTH,
    FIELD_YEAR,
    FIELD_HOUR,
    FIELD_MINUTE,
    FIELD_SECOND
};

static const char *field_names[FIELD_COUNT] =
{
    "DAY",
    "MONTH",
    "YEAR",
    "HOUR",
    "MINUTE",
    "SECOND"
};

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] =
    {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 31, 30
    };

    if (month < 1 || month > 12)
        return 31;

    if (month == 2 &&
        ((year % 4 == 0 && year % 100 != 0) ||
         year % 400 == 0))
        return 29;

    return days[month - 1];
}

static void clamp_day(RTC_DATETIME *dt)
{
    uint8_t max = days_in_month(
        dt->Date.Year,
        dt->Date.Month
    );

    if (dt->Date.Day > max)
        dt->Date.Day = max;

    if (dt->Date.Day < 1)
        dt->Date.Day = 1;
}

static void draw_text(int x, int y, const char *str,
                      uint16_t fg, uint16_t bg)
{
    draw_string8(x, y, str, fg, bg, 0, 0);
}

static void draw_value(int x, int y, uint16_t value,
                       uint16_t fg, uint16_t bg)
{
    char buf[6];
    int i = 0;

    if (value >= 1000)
    {
        buf[i++] = '0' + (value / 1000) % 10;
        buf[i++] = '0' + (value / 100) % 10;
        buf[i++] = '0' + (value / 10) % 10;
        buf[i++] = '0' + value % 10;
    }
    else if (value >= 100)
    {
        buf[i++] = '0' + (value / 100) % 10;
        buf[i++] = '0' + (value / 10) % 10;
        buf[i++] = '0' + value % 10;
    }
    else
    {
        buf[i++] = '0' + (value / 10) % 10;
        buf[i++] = '0' + value % 10;
    }

    buf[i] = '\0';

    draw_text(x, y, buf, fg, bg);
}

static uint16_t get_field_value(const RTC_DATETIME *dt,
                                uint8_t field)
{
    switch (field)
    {
        case FIELD_DAY:    return dt->Date.Day;
        case FIELD_MONTH:  return dt->Date.Month;
        case FIELD_YEAR:   return dt->Date.Year;
        case FIELD_HOUR:   return dt->Time.Hour;
        case FIELD_MINUTE: return dt->Time.Min;
        case FIELD_SECOND: return dt->Time.Sec;
    }

    return 0;
}

static void change_field(RTC_DATETIME *dt,
                         uint8_t field, int direction)
{
    switch (field)
    {
        case FIELD_DAY:
        {
            uint8_t max = days_in_month(
                dt->Date.Year,
                dt->Date.Month
            );

            if (direction > 0)
            {
                dt->Date.Day++;
                if (dt->Date.Day > max)
                    dt->Date.Day = 1;
            }
            else
            {
                if (dt->Date.Day <= 1)
                    dt->Date.Day = max;
                else
                    dt->Date.Day--;
            }
            break;
        }

        case FIELD_MONTH:
            if (direction > 0)
            {
                if (++dt->Date.Month > 12)
                    dt->Date.Month = 1;
            }
            else
            {
                if (dt->Date.Month <= 1)
                    dt->Date.Month = 12;
                else
                    dt->Date.Month--;
            }

            clamp_day(dt);
            break;

        case FIELD_YEAR:
            if (direction > 0)
            {
                if (dt->Date.Year < 2127)
                    dt->Date.Year++;
                else
                    dt->Date.Year = 2000;
            }
            else
            {
                if (dt->Date.Year > 2000)
                    dt->Date.Year--;
                else
                    dt->Date.Year = 2127;
            }

            clamp_day(dt);
            break;

        case FIELD_HOUR:
            if (direction > 0)
                dt->Time.Hour = (dt->Time.Hour + 1) % 24;
            else
                dt->Time.Hour = (dt->Time.Hour + 23) % 24;
            break;

        case FIELD_MINUTE:
            if (direction > 0)
                dt->Time.Min = (dt->Time.Min + 1) % 60;
            else
                dt->Time.Min = (dt->Time.Min + 59) % 60;
            break;

        case FIELD_SECOND:
            if (direction > 0)
                dt->Time.Sec = (dt->Time.Sec + 1) % 60;
            else
                dt->Time.Sec = (dt->Time.Sec + 59) % 60;
            break;
    }
}

static void draw_screen(const RTC_DATETIME *dt,
                        uint8_t selected,
                        bool saved)
{
    char time[9];

    fb_clear(BLACK);

    /* Header */
    fb_rect(0, 0, W, 32, DARK_GRAY);
    draw_text(10, 11, "ALTROM", CYAN, DARK_GRAY);
    draw_text(170, 11, "SETTINGS", GRAY, DARK_GRAY);

    fb_rect(0, 32, W, 2, CYAN);

    /* Clock preview */
    draw_text(10, 45, "SYSTEM CLOCK", GRAY, BLACK);

    time[0] = '0' + dt->Time.Hour / 10;
    time[1] = '0' + dt->Time.Hour % 10;
    time[2] = ':';
    time[3] = '0' + dt->Time.Min / 10;
    time[4] = '0' + dt->Time.Min % 10;
    time[5] = ':';
    time[6] = '0' + dt->Time.Sec / 10;
    time[7] = '0' + dt->Time.Sec % 10;
    time[8] = '\0';

    draw_string12(47, 66, time, CYAN, BLACK, 0, 0);

    /* Date preview */
    draw_text(10, 91, "DATE", GRAY, BLACK);

    draw_value(10, 108, dt->Date.Day, WHITE, BLACK);
    draw_text(27, 108, "/", GRAY, BLACK);

    draw_value(35, 108, dt->Date.Month, WHITE, BLACK);
    draw_text(52, 108, "/", GRAY, BLACK);

    draw_value(60, 108, dt->Date.Year, WHITE, BLACK);

    fb_rect(10, 132, 220, 1, DARK_GRAY);

    /* Field list */
    draw_text(10, 143, "ADJUST VALUES", GRAY, BLACK);

    for (uint8_t i = 0; i < FIELD_COUNT; i++)
    {
        int y = 163 + (i * 21);

        if (selected == i)
        {
            fb_rect(7, y - 3, 226, 20, DARK_CYAN);
            fb_rect(7, y - 3, 3, 20, CYAN);
        }

        uint16_t fg = (selected == i) ? WHITE : GRAY;
        uint16_t bg = (selected == i) ? DARK_CYAN : BLACK;

        draw_text(16, y, field_names[i], fg, bg);

        draw_value(183, y,
                   get_field_value(dt, i),
                   (selected == i) ? CYAN : WHITE,
                   bg);
    }

    /* Footer */
    fb_rect(0, 294, W, 2, DARK_GRAY);

    if (saved)
        draw_text(10, 302, "DATE/TIME SAVED", CYAN, BLACK);
    else
        draw_text(10, 302, "UP/DOWN SELECT  L/R EDIT",
                  GRAY, BLACK);

    draw_text(10, 313, "CANCEL", WHITE, BLACK);
    draw_text(183, 313, "SAVE", CYAN, BLACK);

    fb_flush();
}

void main(void)
{
    RTC_DATETIME dt;
    uint8_t selected = 0;
    bool saved = false;

    dt = RTC_GetDateTime();

    clamp_day(&dt);

    draw_screen(&dt, selected, false);

    while (1)
    {
        watchdog_feed();

        if (KP_LEFT == 0)
        {
            jump(0x70000000);
        }

        if (KP_RIGHT == 0)
        {
            while (KP_RIGHT == 0)
                watchdog_feed();

            if (RTC_SetDateTime(&dt))
            {
                saved = true;
                draw_screen(&dt, selected, saved);

                /* Briefly show confirmation. */
                for (volatile uint32_t i = 0;
                     i < 300000;
                     i++)
                    watchdog_feed();
            }

            break;
        }

        if (KP_CENTER_UP == 0)
        {
            selected = (selected == 0)
                     ? FIELD_COUNT - 1
                     : selected - 1;

            draw_screen(&dt, selected, false);

            while (KP_CENTER_UP == 0)
                watchdog_feed();
        }

        if (KP_CENTER_DOWN == 0)
        {
            selected = (selected + 1) % FIELD_COUNT;

            draw_screen(&dt, selected, false);

            while (KP_CENTER_DOWN == 0)
                watchdog_feed();
        }

        if (KP_CENTER_LEFT == 0)
        {
            change_field(&dt, selected, -1);
            draw_screen(&dt, selected, false);

            while (KP_CENTER_LEFT == 0)
                watchdog_feed();
        }

        if (KP_CENTER_RIGHT == 0)
        {
            change_field(&dt, selected, 1);
            draw_screen(&dt, selected, false);

            while (KP_CENTER_RIGHT == 0)
                watchdog_feed();
        }
    }

    fb_clear(BLACK);
    fb_flush();
}
