#include <wiringPi.h>
#include <stdio.h>
#include <pcf8591.h>

#define FlashPin 28
#define PCF      120

int init()
{
	// global setup
	if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
		printf("setup wiringPi failed !");
		return 1; 
	}

	// Setup flash pin
	printf("linker FlashPin : GPIO %d(wiringPi pin)\n", FlashPin); //when initialize wiring successfully,print message to screen
	pinMode(FlashPin, OUTPUT);

	// Setup pcf8591 on base pin 120, and address 0x48
	pcf8591Setup (PCF, 0x48);

	return 0;
}

void trigFlash()
{
	digitalWrite(FlashPin, HIGH); 
	delay(1);
	digitalWrite(FlashPin, LOW);
	printf("Trig the flash\n");
}

int main(void)
{
	if ( init() != 0 )
		return -1;

	int value;
	int count = 0;
	while(1) {
		value = analogRead  (PCF + 0);
		if( value < 50 ) {
			printf("%d\n", value);
			trigFlash();
			delay(500); // wait for 500 ms to prevent a second flash
		}
	}

	return 0;
}
