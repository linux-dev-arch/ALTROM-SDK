#include "../sdk/sdk.h"

void main(void){
	uint16_t col=0;
	fb_clear(COL_BLACK);
	fb_flush();
	while(col < 0xffff){
		watchdog_feed();
		for(int j=0;j<100;j++){
			for(int i=0;i<128;i++){
				fb_pixel(i,j,col);
				col+=20;
				if(KP_LEFT==0){
                        		jump(0x70000000);
                		}
			}
		}
		fb_flush();
	}
	for(;;){
		watchdog_feed();
		if(KP_LEFT==0){
			jump(0x70000000);
		}
	}
}
