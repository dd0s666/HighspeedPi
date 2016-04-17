#include <wiringPi.h>
#include <stdio.h>
#include <pcf8591.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "highSpeed.h"

// gobal variables used by the rotary encoder module
unsigned char flag;
unsigned char Last_RoB_Status;
unsigned char Current_RoB_Status;

static volatile int keepRunning = 1;
static volatile int mode = MODE_CONFIG;
static volatile int sensor;
int fd;
int levelOrDelay;

// Start photo loop (sound or laser)
void startPhotoMode()
{
	printf("Entering in photo mode\n");
	LCDWrite(0,0, "== Photo mode ==");
	if( sensor == LASER_SENSOR )
		laserLoop(levelOrDelay);
	else if( sensor == SOUND_SENSOR )
		soundLoop(levelOrDelay); 
}

// start or stop configuration mode
void changeMode()
{
printf("----\n");
	if( mode == MODE_CONFIG ) {
		mode = MODE_PHOTO;
		startPhotoMode();
	} else {
		mode = MODE_CONFIG;
		configurationLoop();
	}
}

void rotaryDeal()
{
	Last_RoB_Status = digitalRead(RoBPin);

	while(!digitalRead(RoAPin)){
		Current_RoB_Status = digitalRead(RoBPin);
		flag = 1;
	}

	if(flag == 1){
		flag = 0;
		if((Last_RoB_Status == 0)&&(Current_RoB_Status == 1)){
			levelOrDelay ++;	
		}
		if((Last_RoB_Status == 1)&&(Current_RoB_Status == 0)){
			levelOrDelay --;
		}
	}
}

int init(int activeSensor)
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
	if( activeSensor == LASER_SENSOR ) {
		pinMode(LaserPin, OUTPUT);
		digitalWrite(LaserPin, HIGH); 
		printf("Laser on ... (GPIO %d)\n", LaserPin);

		levelOrDelay = DEFAULT_LASER_DELAY;
	}
	else if( activeSensor == SOUND_SENSOR )
		levelOrDelay = DEFAULT_SOUND_LEVEL;

	LCDInit();

	// Setup rotary module
	pinMode(SWPin, INPUT);
	pinMode(RoAPin, INPUT);
	pinMode(RoBPin, INPUT);
	pullUpDnControl(SWPin, PUD_UP);
    	if( wiringPiISR(SWPin, INT_EDGE_FALLING, &changeMode) < 0 ) {
		fprintf(stderr, "Unable to init ISR\n",strerror(errno));	
		return 1;
	}

	return 0;
}

void cleanup()
{
	digitalWrite(LaserPin, LOW);
	printf("Laser off ... (GPIO %d)\n", LaserPin);
	clearText();
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

void configurationLoop()
{
	printf("Starting configuration loop (%d)", levelOrDelay);
	LCDWrite(0,0, "== Config mode ==");
	char text[20];
	sprintf(text, "%d", levelOrDelay);
	LCDWrite(7, 1, text);

	int tmp = levelOrDelay;
	while( keepRunning ) {
		rotaryDeal();
		if (tmp != levelOrDelay ){
			printf("Rotary: %d\n", levelOrDelay);
			sprintf(text, "<%d>", levelOrDelay);
			LCDWrite(6, 1, text);
			tmp = levelOrDelay;
		}
	}
}

void laserLoop(int delayBeforeFlash)
{
	printf("Starting laser loop (%d)", delayBeforeFlash);
	char text[20];
	sprintf(text, "LASER <%d>", delayBeforeFlash);
	LCDWrite(0, 1, text);

	int value;
	while(keepRunning) {
		value = analogRead  (PCF + 0);
		if( value > 100 ) {
			delay(delayBeforeFlash);
			trigFlash();
			printf("LASER: %d\n", value);
			delay(500); // wait for 500 ms to prevent a second flash
		}
	}
}

void soundLoop(int triggerValue)
{
	printf("Starting sound loop (%d)", triggerValue);
	char text[20];
	sprintf(text, "SOUND <%d>", triggerValue);
	LCDWrite(0, 1, text);

	int value;
	while(keepRunning) {
		value = analogRead  (PCF + 0);
		if( value <= triggerValue ) {
			trigFlash();
			printf("%d\n", value);
			delay(500); // wait for 500 ms to prevent a second flash
		}
		delay(1);
	}
}

void LCDWriteWord(int data){
	wiringPiI2CWrite(fd, data |= 0x08);
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

	if( argc == 2 && strcmp(argv[1], "--laser") == 0 )
		sensor = LASER_SENSOR;
	else if( argc == 2 && strcmp(argv[1], "--sound") == 0 )
		sensor = SOUND_SENSOR;
	else {
		printf("Invalid mode");
		return -1;
	}

	if( init(sensor) != 0 )
		return -1;

	mode = MODE_PHOTO;
	changeMode();

	cleanup();
	return 0;
}
