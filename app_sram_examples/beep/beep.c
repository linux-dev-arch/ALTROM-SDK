#include "../../sdk/sdk.h"

void main(void){
	audio_tone(8,7,200);
	audio_tone(80,20,200);
	audio_tone(22,64,200);
	jump(ALTROM_RETURN);
}
