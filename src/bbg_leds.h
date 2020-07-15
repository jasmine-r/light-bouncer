// bbg_leds.h:
// Application module to control BBG LEDs (i.e. initialize LEDs, drive LEDs, display flash mode patterns)

#ifndef _BBG_LEDS_H_
#define _BBG_LEDS_H_

// enable GPIO, set direction (output) for GPIO pins
void initializeLeds(void);

//pattern A: bouncing LEDs
void bounceLEDs(void);

//pattern B: LEDs light up to create bar/line effect
void barLEDs(void);

// set flash speed based on user input
void setLEDFlashSpeed(int specifiedSpeed);

// returns the current flash speed
uint32_t getLEDFlashSpeed();

// busy wait used to debounce joystick input
void busyWait(unsigned int count);

#endif
