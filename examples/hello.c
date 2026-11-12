#include "../sdk/sdk.h"

void main(void){
	for(;;){
		fb_clear(COL_BLACK);
		draw_string12(10,120,"Hello World",COL_RED,COL_BLACK,0,5);
		fb_flush();
		if(KP_LEFT==0){
			jump(0x70000000);
		}
		watchdog_feed();
	}
}
