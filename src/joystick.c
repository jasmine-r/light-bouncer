/* joystick.c -- > Application module to manage joystick input to control speed and flash-mode*/
#include "soc_AM335x.h"
#include "beaglebone.h"
#include "gpio_v2.h"
#include "hw_types.h"      // For HWREG(...) macro
#include "watchdog.h"
#include "uart_irda_cir.h"
#include "consoleUtils.h"
#include <stdint.h>
#include "bbg_leds.h"

/*****************************************************************************
**                INTERNAL MACRO DEFINITIONS
*****************************************************************************/
// Boot btn on BBB: SOC_GPIO_2_REGS, pin 8
// Down on Zen cape: SOC_GPIO_1_REGS, pin 14  NOTE: Must change other "2" constants to "1" for correct initialization.
// Left on Zen cape: SOC_GPIO_2_REGS, pin 1
// --> This code uses left on the ZEN:

// LEFT ON JOYSTICK
#define BUTTON_GPIO_BASE     (SOC_GPIO_2_REGS)
#define BUTTON_PIN           (1)

// UP AND DOWN ON JOYSTICK
#define BUTTON_GPIO_BASE_UP   (SOC_GPIO_0_REGS)
#define BUTTON_PIN_UP         (26)
#define BUTTON_GPIO_BASE_DOWN (SOC_GPIO_1_REGS)
#define BUTTON_PIN_DOWN       (14)

#define BAUD_RATE_115200          (115200)
#define UART_MODULE_INPUT_CLK     (48000000)

//#define DELAY_TIME 0x4000000
#define DELAY_TIME 0x40000
/*****************************************************************************
**                INTERNAL FUNCTION PROTOTYPES
*****************************************************************************/
static _Bool readButtonWithStarterWare(void);

static _Bool readUpButtonWithBitTwiddling(void);
static _Bool readDownButtonWithBitTwiddling(void);

static _Bool isJoystickPressedLeft = false;
static _Bool isJoystickPressedUp = false;
static _Bool isJoystickPressedDown = false;


static int flashMode = 0; // default: bouncing LEDs
/*****************************************************************************
**                INTERNAL FUNCTION DEFINITIONS
*****************************************************************************/
#include "hw_cm_per.h"


void GPIO2ModuleClkConfig(void)
{
    /* Writing to MODULEMODE field of CM_PER_GPIO1_CLKCTRL register. */
    HWREG(SOC_CM_PER_REGS + CM_PER_GPIO2_CLKCTRL) |=
          CM_PER_GPIO2_CLKCTRL_MODULEMODE_ENABLE;

    /* Waiting for MODULEMODE field to reflect the written value. */
    while(CM_PER_GPIO2_CLKCTRL_MODULEMODE_ENABLE !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_GPIO2_CLKCTRL) &
           CM_PER_GPIO2_CLKCTRL_MODULEMODE));
    /*
    ** Writing to OPTFCLKEN_GPIO_2_GDBCLK bit in CM_PER_GPIO2_CLKCTRL
    ** register.
    */
    HWREG(SOC_CM_PER_REGS + CM_PER_GPIO2_CLKCTRL) |=
          CM_PER_GPIO2_CLKCTRL_OPTFCLKEN_GPIO_2_GDBCLK;

    /*
    ** Waiting for OPTFCLKEN_GPIO_1_GDBCLK bit to reflect the desired
    ** value.
    */
    while(CM_PER_GPIO2_CLKCTRL_OPTFCLKEN_GPIO_2_GDBCLK !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_GPIO2_CLKCTRL) &
           CM_PER_GPIO2_CLKCTRL_OPTFCLKEN_GPIO_2_GDBCLK));

    /*
    ** Waiting for IDLEST field in CM_PER_GPIO2_CLKCTRL register to attain the
    ** desired value.
    */
    while((CM_PER_GPIO2_CLKCTRL_IDLEST_FUNC <<
           CM_PER_GPIO2_CLKCTRL_IDLEST_SHIFT) !=
           (HWREG(SOC_CM_PER_REGS + CM_PER_GPIO2_CLKCTRL) &
            CM_PER_GPIO2_CLKCTRL_IDLEST));

    /*
    ** Waiting for CLKACTIVITY_GPIO_2_GDBCLK bit in CM_PER_L4LS_CLKSTCTRL
    ** register to attain desired value.
    */
    while(CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_GPIO_2_GDBCLK !=
          (HWREG(SOC_CM_PER_REGS + CM_PER_L4LS_CLKSTCTRL) &
           CM_PER_L4LS_CLKSTCTRL_CLKACTIVITY_GPIO_2_GDBCLK));

}

void initializeJoystick(void)
{
	 /* Enabling functional clocks for GPIO0 and GPIO2 instance. */
	GPIO0ModuleClkConfig();
	//GPIO1ModuleClkConfig(); // already initialized in bbg_leds.c
	GPIO2ModuleClkConfig();

    /* Selecting GPIO1[23] pin for use. 6*/
    //GPIO1Pin23PinMuxSetup();

    /* Enabling the GPIO module. */
    GPIOModuleEnable(BUTTON_GPIO_BASE);

    /* Resetting the GPIO module. */
    GPIOModuleReset(BUTTON_GPIO_BASE);

    /* Setting the GPIO pin as an input pin. */
    GPIODirModeSet(BUTTON_GPIO_BASE,
                   BUTTON_PIN,
                   GPIO_DIR_INPUT);

    // DOWN (joystick)
     /* Setting the GPIO pin as an input pin. */
     GPIODirModeSet(BUTTON_GPIO_BASE_DOWN,
                      BUTTON_PIN_DOWN,
                      GPIO_DIR_INPUT);

      // UP (joystick)
      /* Enabling the GPIO module. */
	  GPIOModuleEnable(BUTTON_GPIO_BASE_UP);

	  /* Resetting the GPIO module. */
	  GPIOModuleReset(BUTTON_GPIO_BASE_UP);

	  /* Setting the GPIO pin as an input pin. */
	  GPIODirModeSet(BUTTON_GPIO_BASE_UP,
					 BUTTON_PIN_UP,
					 GPIO_DIR_INPUT);
}

// check for joystick input
void joystick_doBackgroundWork(void){
	// if joystick pressed left, toggle flash mode (b/w bouncing and bar)
	isJoystickPressedLeft = readButtonWithStarterWare();
	if(isJoystickPressedLeft == true){
		flashMode = (flashMode + 1) % 2;
		if(flashMode == 1){
			barLEDs();
		}
		else if(flashMode == 0){
			bounceLEDs();
		}
	}

	// if joystick pressed up, increase volume by one
	isJoystickPressedUp = readUpButtonWithBitTwiddling();
	if(isJoystickPressedUp == true){
		int currentLEDspeed = getLEDFlashSpeed();
		if(currentLEDspeed < 9){
			setLEDFlashSpeed(currentLEDspeed + 1);
		}
		else{
			setLEDFlashSpeed(9);
		}
	}

	// if joystick pressed down, decrease volume by one
	isJoystickPressedDown = readDownButtonWithBitTwiddling();
	if(isJoystickPressedDown == true){
		int currentLEDspeed = getLEDFlashSpeed();
		if(currentLEDspeed > 0){
			setLEDFlashSpeed(currentLEDspeed - 1);
		}
		else{
			setLEDFlashSpeed(0);
		}
	}

	// debounce
	busyWait(DELAY_TIME);

}


// check if joystick was pressed LEFT (basic functionality)
static _Bool readButtonWithStarterWare(void)
{
	return GPIOPinRead(BUTTON_GPIO_BASE, BUTTON_PIN) == 0;
}

/* BONUS */

// check if joystick was pressed DOWN
static _Bool readDownButtonWithBitTwiddling(void)
{
	uint32_t regValue = HWREG(BUTTON_GPIO_BASE_DOWN + GPIO_DATAIN);
	uint32_t mask     = (1 << BUTTON_PIN_DOWN);

	return (regValue & mask) == 0;
}

// check if joystick was pressed UP
static _Bool readUpButtonWithBitTwiddling(void)
{
	uint32_t regValue = HWREG(BUTTON_GPIO_BASE_UP + GPIO_DATAIN);
	uint32_t mask     = (1 << BUTTON_PIN_UP);

	return (regValue & mask) == 0;
}

/***************************************end of file *******************************************************/
