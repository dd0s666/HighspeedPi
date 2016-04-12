#include <wiringPi.h>
#include <stdio.h>
#include <pcf8591.h>
#include <signal.h>
#include <string.h>


#define LaserPin 27
#define FlashPin 28
#define PCF      120

static const int LCDAddr = 0x27;
static const int PCF8591_Addr = 0x48;


#define DEFAULT_LASER_DELAY    180
#define DEFAULT_SOUND_LEVEL    35


static volatile int keepRunning = 1;
int fd;

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
	pcf8591Setup (PCF, PCF8591_Addr);

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

void laserLoop(int delayBeforeFlash)
{
	printf("Starting laser loop (%d)", delayBeforeFlash);
	int value;
	while(keepRunning) {
		value = analogRead  (PCF + 0);
		if( value > 100 ) {
			printf("%d\n", value);
			delay(delayBeforeFlash);
			trigFlash();
			delay(500); // wait for 500 ms to prevent a second flash
		}
	}
}

void soundLoop(int triggerValue)
{
	printf("Starting sound loop (%d)", triggerValue);
	int value;
	while(keepRunning) {
		value = analogRead  (PCF + 0);
		if( value <= triggerValue ) {
			trigFlash();
			printf("%d\n", value);
			delay(500); // wait for 500 ms to prevent a second flash
		}
	}
}

int BLEN = 1;

void LCDWriteWord(int data){
	int temp = data;
	if ( BLEN == 1 )
		temp |= 0x08;
	else
		temp &= 0xF7;
	wiringPiI2CWrite(fd, temp);
}

void send_command(int comm){
	int buf;
	// Send bit7-4 firstly
	buf = comm & 0xF0;
	buf |= 0x04;			// RS = 0, RW = 0, EN = 1
	LCDWriteWord(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	LCDWriteWord(buf);

	// Send bit3-0 secondly
	buf = (comm & 0x0F) << 4;
	buf |= 0x04;			// RS = 0, RW = 0, EN = 1
	LCDWriteWord(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	LCDWriteWord(buf);
}

void send_data(int data){
	int buf;
	// Send bit7-4 firstly
	buf = data & 0xF0;
	buf |= 0x05;			// RS = 1, RW = 0, EN = 1
	LCDWriteWord(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	LCDWriteWord(buf);

	// Send bit3-0 secondly
	buf = (data & 0x0F) << 4;
	buf |= 0x05;			// RS = 1, RW = 0, EN = 1
	LCDWriteWord(buf);
	delay(2);
	buf &= 0xFB;			// Make EN = 0
	LCDWriteWord(buf);
}

void LCDInit(){
	fd = wiringPiI2CSetup(LCDAddr);
	send_command(0x33);	// Must initialize to 8-line mode at first
	delay(5);
	send_command(0x32);	// Then initialize to 4-line mode
	delay(5);
	send_command(0x28);	// 2 Lines & 5*7 dots
	delay(5);
	send_command(0x0C);	// Enable display without cursor
	delay(5);
	send_command(0x01);	// Clear Screen
	wiringPiI2CWrite(fd, 0x08);
}

void clearText(){
	send_command(0x01);	//clear Screen
}

void LCDWrite(int x, int y, char data[]){
	int addr, i;
	int tmp;
	if (x < 0)  x = 0;
	if (x > 15) x = 15;
	if (y < 0)  y = 0;
	if (y > 1)  y = 1;

	// Move cursor
	addr = 0x80 + 0x40 * y + x;
	send_command(addr);
	
	tmp = strlen(data);
	for (i = 0; i < tmp; i++){
		send_data(data[i]);
	}
}

int main(int argc, char* argv[])
{
	signal(SIGINT, intHandler);
	if( init() != 0 )
		return -1;
	LCDInit();
	LCDWrite(0, 0, argv[1]);
	LCDWrite(0, 1, "By www.ddphoto.fr");

	if( argc == 2 && strcmp(argv[1], "--laser") == 0 )
		laserLoop(DEFAULT_LASER_DELAY);
	else if( argc == 2 && strcmp(argv[1], "--sound") == 0 )
		soundLoop(DEFAULT_SOUND_LEVEL); 
	else 
		printf("invalid mode");

	cleanup();
	clearText();
	return 0;
}
