#include <wiringPi.h>
#include <stdio.h>
#include <pcf8591.h>
#include <signal.h>

static volatile int keepRunning = 1;

#define LaserPin 27
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

	// power on the laser
	pinMode(LaserPin, OUTPUT);
	digitalWrite(LaserPin, HIGH); 
	printf("Laser on ... (GPIO %d)\n", LaserPin);

	return 0;
}

void cleanup()
{
	digitalWrite(LaserPin, LOW);
	printf("Laser off ... (GPIO %d)\n", LaserPin);
}

void trigFlash()
{
	digitalWrite(FlashPin, HIGH); 
	delay(1);
	digitalWrite(FlashPin, LOW);
	printf("Trig the flash\n");
}

void intHandler(int dummy) {
    keepRunning = 0;
}

int main(void)
{
	signal(SIGINT, intHandler);
	if ( init() != 0 )
		return -1;

	int value;
	int count = 0;
	while(keepRunning) {
		value = analogRead  (PCF + 0);
		if( value > 100 ) {
			printf("%d\n", value);
			trigFlash();
			delay(500); // wait for 500 ms to prevent a second flash
		}
	}


	return 0;
}
