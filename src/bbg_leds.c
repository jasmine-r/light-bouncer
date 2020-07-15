// bbg_leds.h:
/* Application module to control BBG LEDs (i.e. initialize LEDs, drive LEDs, display flash mode patterns)*/
#include "soc_AM335x.h"
#include "beaglebone.h"
#include "gpio_v2.h"
#include "hw_types.h"      // For HWREG(...) macro
#include "watchdog.h"
#include <stdint.h>
#include "wdtimer.h"
#include "bbg_leds.h"


/*****************************************************************************
 **                INTERNAL MACRO DEFINITIONS
 *****************************************************************************/
#define LED_GPIO_BASE           (SOC_GPIO_1_REGS)
#define LED0_PIN (21)
#define LED1_PIN (22)
#define LED2_PIN (23)
#define LED3_PIN (24)

#define LED_MASK ((1<<LED0_PIN) | (1<<LED1_PIN) | (1<<LED2_PIN) | (1<<LED3_PIN))

//#define DELAY_TIME 0x4000000		// Delay with MMU enabled
//#define DELAY_TIME 0x40000		// Delay without MMU and cache

#define DEFAULT_LED_SPEED  (4)

/*****************************************************************************
 **                INTERNAL FUNCTION PROTOTYPES
 *****************************************************************************/
static int calculatePower(int base, int exp);
/*****************************************************************************
 **                INTERNAL FUNCTION DEFINITIONS
 *****************************************************************************/

// Speed of LED-flashing
static volatile uint32_t speed = DEFAULT_LED_SPEED; // default

// function below is used to calculate flashPeriod timing using speed specified by user
static int calculatePower(int base, int exp){
	int power = base;
	for(int i = 0; i < exp-1; i++){
		power *= base;
	}
	return power;
}

void initializeLeds(void)
{
	/* Enabling functional clocks for GPIO1 instance. */
	GPIO1ModuleClkConfig();

	/* Selecting GPIO1[23] pin for use. */
	//GPIO1Pin23PinMuxSetup();

	/* Enabling the GPIO module. */
	GPIOModuleEnable(LED_GPIO_BASE);

	/* Resetting the GPIO module. */
	GPIOModuleReset(LED_GPIO_BASE);

	/* Setting the GPIO pin as an output pin. */
	GPIODirModeSet(LED_GPIO_BASE,
			LED0_PIN,
			GPIO_DIR_OUTPUT);
	GPIODirModeSet(LED_GPIO_BASE,
			LED1_PIN,
			GPIO_DIR_OUTPUT);
	GPIODirModeSet(LED_GPIO_BASE,
			LED2_PIN,
			GPIO_DIR_OUTPUT);
	GPIODirModeSet(LED_GPIO_BASE,
			LED3_PIN,
			GPIO_DIR_OUTPUT);
}

void setLEDFlashSpeed(int specifiedSpeed){
	speed = specifiedSpeed;
}

uint32_t getLEDFlashSpeed(){
	return speed;
}

// LED FLASHING -- PATTERN A
// LEDs are turned on and off in sequence -- creating "bouncing" effect with LEDs
void bounceLEDs(void){
	// light up LEDs one by one (D2 -> D5)
	uint32_t speedDividerFactor = calculatePower(2, (9-speed));
	uint32_t flashTimePeriod = 10000 * speedDividerFactor;

	for (int pin = LED0_PIN; pin <= LED3_PIN; pin++) {
		/* Driving a logic HIGH on the GPIO pin. */
		GPIOPinWrite(LED_GPIO_BASE, pin, GPIO_PIN_HIGH);
		Watchdog_hit(); // prevent reboot while flashing LEDs
		busyWait(flashTimePeriod);
		/* Driving a logic LOW on the GPIO pin. */
		GPIOPinWrite(LED_GPIO_BASE, pin, GPIO_PIN_LOW);
	}

	// light up LEDs one by one in opposite direction
	for (int pin = LED3_PIN; pin >= LED0_PIN; pin--) {
			/* Driving a logic HIGH on the GPIO pin. */
			GPIOPinWrite(LED_GPIO_BASE, pin, GPIO_PIN_HIGH);
			Watchdog_hit(); // prevent reboot while flashing LEDs
			busyWait(flashTimePeriod);
			/* Driving a logic LOW on the GPIO pin. */
			GPIOPinWrite(LED_GPIO_BASE, pin, GPIO_PIN_LOW);
	}

	// Hit the watchdog (must #include "watchdog.h"
	// Each time you hit the WD, must pass it a different number
	// than the last time you hit it.
	Watchdog_hit();
}

// LED FLASHING -- PATTERN B
// LEDs light up in sequence, then turn off in reverse order -- gives effect of line/bar lighting up
void barLEDs(void){
	uint32_t speedDividerFactor = calculatePower(2, (9-speed));
	uint32_t flashTimePeriod = 10000 * speedDividerFactor;

	// turn all ON
	for (int pin = LED0_PIN; pin <= LED3_PIN; pin++) {
		/* Driving a logic HIGH on the GPIO pin. */
		GPIOPinWrite(LED_GPIO_BASE, pin, GPIO_PIN_HIGH);
		Watchdog_hit(); // prevent reboot while flashing LEDs
		busyWait(flashTimePeriod);
	}

	// turn all OFF (moving in opposite direction towards the first LED that lit up)
	for (int pin = LED3_PIN; pin >= LED0_PIN; pin--) {
		/* Driving a logic LOW on the GPIO pin. */
		GPIOPinWrite(LED_GPIO_BASE, pin, GPIO_PIN_LOW);
		Watchdog_hit(); // prevent reboot while flashing LEDs
		busyWait(flashTimePeriod);
	}

	// Hit the watchdog (must #include "watchdog.h"
	// Each time you hit the WD, must pass it a different number
	// than the last time you hit it.
	Watchdog_hit();
}


/*
 ** Busy-wait function -- also used to debounce joystick
 */
void busyWait(volatile unsigned int count)
{
	while(count--)
		;
}
