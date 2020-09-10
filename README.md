# Interface module for modular controllers

The module can be used for converting encoder, pot and button movement
into messages sent over TWI / I2C. You can e.g. combine up to 6 encoders 
(with push buttons) and connect them to a single 
microcontroller (ATMega168p) using only two separate PCB designs. This 
helps to bring down the costs of the BOM and of the boards. The design 
also  includes a configurable LED ring that can be used to visualize the 
positions of the encoders. You can connect up to 112 microcontroller 
boards to a single bus to create a device with up to 672 encoders!

By connecting a USB enabled microcontroller to the I2C bus you can
create an UI for all kinds of devices such as musical instruments,
DJ controllers, home automation control panels, etc.

The PCBs are connected using FFC cables making building, reusing
reconfiguring and debugging the parts easy while at the same time
keeping the BOM cost low.

You can find a proof of concept programming jig / bed of nails tester
for the microcontroller board here: https://a360.co/2ylXmQ3

## Currently supported components and settings
* Encoders
  * Absolute / Relative
  * Loop around / constrained between values
* Potentiometers
* Single switches and 3x3 button matrices
* AT42QT1010 capacitive touch sensor
* Sparkfun 2x2 button pads with SMT LED
* SK6812 / WS2812 serially addressable LEDs

## Project status
* If you would like to use the designs and the code in your project, please add a issue to this repository
  so that I can check whether the needed features have been implemented.
* There have been multiple iterations of the PCBs and not all software has been updated to work on the
  latest PCBs. The current implementation uses the PCB_VERSION constant with value 3 in the code.
  * Features not updated currently:
    * Touch
    * Pads
    * Potentiometers
    * Button matrices (?)
