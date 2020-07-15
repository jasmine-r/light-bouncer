// Fake Typing bare metal sample application
// On the serial port, fakes

#include "consoleUtils.h"
#include <stdint.h>
#include "hw_types.h"      // For HWREG(...) macro

// My hardware abstraction modules
#include "serial.h"
#include "timer.h"
#include "wdtimer.h"

// My application's modules
//#include "fakeTyper.h"
#include "joystick.h"
#include "bbg_leds.h"

//#define HWREG(x) (*((volatile unsigned int *)(x)))
#define PRM_DEV (0x44E00F00)    // base
#define PRM_RSTST_OFFSET (0x08) // reg offset

// hex values in reset source register
#define COLD_RESET (0x1)      // register value in binary --> 0...01   (LSB; decimal value = 1)
#define WATCHDOG_RESET (0x10) // register value in binary --> 0..1000  (4th from last bit; decimal value = 16)
#define EXTERNAL_RESET (0x20) // register value in binary --> 0..10000 (5th bit from last bit; decimal value = 32)
#define MASK_TO_CLEAR_BITS (0xFFFFFFFF) // used to clear bits in reset source register

/******************************************************************************
 **              SERIAL PORT HANDLING
 ******************************************************************************/
static uint32_t stopHittingWatchdog = 0;
static uint32_t speedLED = 0;
static volatile uint8_t s_rxByte = 0;
static void serialRxIsrCallback(uint8_t rxByte) {
	s_rxByte = rxByte;
}

/* This function is used to output the commands (help message);
 * it is only used as part of background serial work (as well as once during start-up) */
static void helpMessage(){
	ConsoleUtilsPrintf("\n\n***********Commands*************\n");
	ConsoleUtilsPrintf("\n ?  : Display this help message\n");
	ConsoleUtilsPrintf("\n0-9 : Set speed 0 (slow) to 9 (fast).\n");
	ConsoleUtilsPrintf("\na   : Select pattern A (bounce).\n");
	ConsoleUtilsPrintf("\nb   : Select pattern B (bar).\n");
	ConsoleUtilsPrintf("\nx   : Stop hitting the watchdog.\n");
	ConsoleUtilsPrintf("\nBTN LEFT : Push-button LEFT to toggle mode.\n");
	ConsoleUtilsPrintf("\n       ---Bonus Features---  \n");
	ConsoleUtilsPrintf("\nBTN UP: Push-button UP to increase speed.\n");
	ConsoleUtilsPrintf("\nBTN DOWN: Push-button DOWN to decrease speed.\n");
	ConsoleUtilsPrintf("\n*********************************\n");

}

/* The function below is used to determine the speed specified by user for LED flashing (atoi not functional here)
 * This speed value is stored as an integer in variable, passed to LED module when a LED flash pattern is selected
 * or when joystick is pressed left to toggle between flash modes */
uint32_t determineSpeedSpecified(uint8_t s_rxByte){
	int speedLED = 0;
	switch (s_rxByte){
		case '0':
			speedLED = 0;
			break;
		case '1':
			speedLED = 1;
			break;
		case '2':
			speedLED = 2;
			break;
		case '3':
			speedLED = 3;
			break;
		case '4':
			speedLED = 4;
			break;
		case '5':
			speedLED = 5;
			break;
		case '6':
			speedLED = 6;
			break;
		case '7':
			speedLED = 7;
			break;
		case '8':
			speedLED = 8;
			break;
		case '9':
			speedLED = 9;
			break;
		default:
			speedLED = 4;
	}
	return speedLED;
}

static void doBackgroundSerialWork(void)
{
	if (s_rxByte != 0) {
		// Display help message
		if (s_rxByte == '?') {
			ConsoleUtilsPrintf("\nNow displaying help message..\n\n");
			helpMessage();
		}

		// Set speed
		else if(s_rxByte >= '0' && s_rxByte <= '9'){
			speedLED = determineSpeedSpecified(s_rxByte);
			ConsoleUtilsPrintf("\nSetting LED speed to %u...\n", speedLED);
			setLEDFlashSpeed(speedLED);
		}

		// Select Pattern A -- BOUNCE
		else if(s_rxByte == 'a'){
			ConsoleUtilsPrintf("\nChanging to bounce mode.\n");
			bounceLEDs();
		}

		// Select Pattern B -- BAR
		else if(s_rxByte == 'b'){
			ConsoleUtilsPrintf("\nChanging to bar mode.\n");
			barLEDs();
		}

		// Stop hitting the watchdog (result in reboot)
		else if(s_rxByte == 'x'){
			ConsoleUtilsPrintf("\nNo longer hitting the watchdog..\n");
			stopHittingWatchdog = 1;
		}

		else{
			ConsoleUtilsPrintf("\nError: unknown command. Please enter one of the following commands:\n");
			helpMessage();
		}

		s_rxByte = 0;
	}
}

/******************************************************************************
 **              MAIN
 ******************************************************************************/
int main(void)
{
	// Initialization
	Serial_init(serialRxIsrCallback);
	Timer_init();
	Watchdog_init();
	initializeLeds();
	initializeJoystick();


	// Setup callbacks from hardware abstraction modules to application:
	Serial_setRxIsrCallback(serialRxIsrCallback);

	// Display startup messages to console:
	ConsoleUtilsPrintf("\nWelcome to the LightBouncer Application!\n");
	ConsoleUtilsPrintf("\nCMPT 433 - Assignment 5 - FALL 2019 - Jasmine Rai\n");

	// Print the reset sources
	ConsoleUtilsPrintf("Value in Reset Register: %u\n", HWREG(PRM_DEV + PRM_RSTST_OFFSET));
	// determine the source of previous reset -- cold reset, watchdog reset or external reset
	_Bool wasColdReset = (HWREG(PRM_DEV + PRM_RSTST_OFFSET)) == COLD_RESET;
	_Bool wasWatchdogReset = (HWREG(PRM_DEV + PRM_RSTST_OFFSET)) == WATCHDOG_RESET;
	_Bool wasExternalReset = (HWREG(PRM_DEV + PRM_RSTST_OFFSET)) == EXTERNAL_RESET;
	ConsoleUtilsPrintf("List of Reset Sources "
			"(External Reset, Watchdog Reset, Cold Reset): %u, %u, %u\n",
			 wasExternalReset, wasWatchdogReset, wasColdReset);

	// Clear reset reset source reg. bits -- clean for next reboot
	HWREG(PRM_DEV + PRM_RSTST_OFFSET) |= MASK_TO_CLEAR_BITS;

	// Display help message (commands)
	helpMessage();

	// Main loop:
	while(1) {
		// Handle background processing
		doBackgroundSerialWork();
		joystick_doBackgroundWork();

		// Timer ISR signals intermittent background activity.
		if(Timer_isIsrFlagSet()) {
			Timer_clearIsrFlag();
			if(!stopHittingWatchdog){
				Watchdog_hit();
			}
		}
	}
}
