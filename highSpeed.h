#define  RoAPin      0
#define  RoBPin      1
#define  SWPin       2
#define  LaserPin   27
#define  FlashPin   28
#define  PCF       120

static const int LCDAddr = 0x27;
static const int PCF8591_Addr = 0x48;

#define DEFAULT_LASER_DELAY    180
#define DEFAULT_SOUND_LEVEL    35

#define MODE_PHOTO	1
#define MODE_CONFIG	0
#define SOUND_SENSOR	0
#define LASER_SENSOR	1

void startPhotoMode();

// start or stop configuration mode
void changeMode();
void rotaryDeal();
int init(int startLaser);
void cleanup();
void trigFlash();
void intHandler(int dummy);

void laserLoop(int delayBeforeFlash);
void soundLoop(int triggerValue);
void configurationLoop();


// LCD functions
void LCDInit();
void LCDWriteWord(int data);
void send_command(int comm);
void send_data(int data);
void clearText();
void LCDWrite(int x, int y, char data[]);
