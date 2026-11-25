#include "../sdk/sdk.h"

void main(void){
	fb_clear(COL_BLACK);
	draw_string8(10,120,"It Works!",COL_TEAL,COL_BLACK,0,0);
	fb_flush();
	delay_ms(1000);
	jump(ALTROM_RETURN);
}
