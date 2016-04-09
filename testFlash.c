#include <wiringPi.h>
#include <stdio.h>

#define FlashPin 28

int init()
{
	if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
		printf("setup wiringPi failed !");
		return 1; 
	}
	printf("linker FlashPin : GPIO %d(wiringPi pin)\n", FlashPin); //when initialize wiring successfully,print message to screen
	pinMode(FlashPin, OUTPUT);
	return 0;
}

void trigFlash()
{
	printf("Trig the flash\n");
	digitalWrite(FlashPin, HIGH); 
	delay(1);
	digitalWrite(FlashPin, LOW);
}

int main(void)
{
	if ( init() == 0 )
	{
		trigFlash();
	}

	return 0;
}
