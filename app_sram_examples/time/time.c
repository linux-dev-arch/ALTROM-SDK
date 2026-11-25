#include "../../sdk/sdk.h"

void show_date(void)
{
    RTC_DATETIME dt = RTC_GetDateTime();

    draw_int8(10, 10, dt.Date.Day, COL_WHITE, COL_BLACK);
    draw_int8(35, 10, dt.Date.Month, COL_WHITE, COL_BLACK);
    draw_int8(60, 10, dt.Date.Year, COL_WHITE, COL_BLACK);
    draw_int8(10, 30, dt.Time.Hour,COL_WHITE, COL_BLACK);
    draw_int8(35, 30, dt.Time.Min, COL_WHITE, COL_BLACK);
    draw_int8(60, 30, dt.Time.Sec, COL_WHITE, COL_BLACK);
    fb_flush();
}

void main(void){
	for(;;){
		fb_clear(COL_BLACK);
		show_date();
		watchdog_feed();
		if(!KP_LEFT){
			jump(ALTROM_RETURN);
		}
	}
}
