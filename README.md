# light-bouncer
light-bouncer is a bare metal application which runs on a Beaglebone & Zen Cape. 

Functionality
---------------
- Flashes the four LEDs on the BeagleBone in one of the following patterns:
  - a bouncing pattern (turning on 1 LED at a time, illumination sequence: 0, 1, 2, 3, 2, 1, 0, ... ) 
  - a bar pattern (turn on multiple LEDs at one time, illumination sequence: 0, 1, 2, 3, 2, 1, 0, ... )
- Joystick on the Zen Cape can be used for the following functions:
  - toggle flash-modes between bounce mode or bar mode by pressing joystick left
  - increment speed (press up) and decrement speed (press down)
 
 References
 ---------- 
 - Sample code provided by Dr. Brian Fraser 
 - TI AM335x StarterWare package
