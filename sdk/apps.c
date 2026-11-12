#include "file_mgr.c"
void bounce(void){
	int x=120;
	int y=160;
	int old_x;
	int old_y;
	int speed_x=6;
	int speed_y=5;
	fb_clear(COL_BLACK);
	fb_rect(x,y,15,10,COL_GREEN);
	fb_flush();
	for(;;){
		if(KP_RIGHT==0){
			sleep(10000);
			break;
		}
		old_x=x;
		old_y=y;
		x+=speed_x;
		y+=speed_y;
		if(x+15>240 || x <= 0){
			speed_x=-speed_x;
		}
		if(y+15>320 || y <= 0){
			speed_y=-speed_y;
		}
		fb_rect(old_x,old_y,15,15,COL_BLACK);
        	fb_rect(x,y,15,15,COL_GREEN);
        	fb_flush();
	}
}
void calc(void){
	int32_t out =0;
	int32_t temp=0;
	int32_t in=0;
	fb_clear(COL_BLACK);
	fb_rect(10,30,220,40,COL_GREEN);
	draw_int(10,50,out,COL_BLACK,COL_GREEN);
	fb_flush();
	for(;;){
		if(KP_RIGHT==0){break;}
		if(KP_CENTER_UP==0){out++;}
		fb_clear(COL_BLACK);
        fb_rect(10,30,220,40,COL_GREEN);
        draw_int(150,40,out,COL_BLACK,COL_GREEN);
        fb_flush();
	}
}
