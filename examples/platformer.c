#include "../sdk/sdk.h"

void main(void){
	int x=120;
	int y=160;
	int x_vel=0;
	int y_vel=0;
	int speed=10;
	int old_x=0;
	int old_y=0;
	fb_clear(COL_BLACK);
	for(;;){
		watchdog_feed();
		if(!KP_CENTER_LEFT){
			x_vel=-speed;
		}
		if(!KP_CENTER_RIGHT){
			x_vel=speed;
		}
		old_x=x;
		old_y=y;
		x+=x_vel;
		y+=y_vel;
		fb_rect(old_x,old_y,10,10,COL_BLACK);
                fb_rect_outline(x,y,10,10,COL_TEAL);
                fb_flush();
		if(!KP_LEFT){
			jump(0x70000000);
		}
	}
}
