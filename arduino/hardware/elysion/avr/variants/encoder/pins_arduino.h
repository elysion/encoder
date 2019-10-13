/*
  pins_arduino.h - Pin definition functions for Arduino
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
*/

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <avr/pgmspace.h>

#define NUM_DIGITAL_PINS            24
#define NUM_ANALOG_INPUTS           10
#define analogInputToDigitalPin(p)  ((p == 7) ? 22 : p == 6 ? 19 : p < 6 ? p + 23 : -1)

#define digitalPinHasPWM(p)         ((p) == 1 || (p) == 9 || (p) == 10 || (p) == 13 || (p) == 14 || (p) == 15)


#define PIN_SPI_SS    (14)
#define PIN_SPI_MOSI  (15)
#define PIN_SPI_MISO  (16)
#define PIN_SPI_SCK   (17)

static const uint8_t SS   = PIN_SPI_SS;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

#define PIN_WIRE_SDA        (27)
#define PIN_WIRE_SCL        (28)

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

#define LED_BUILTIN 13

#define PIN_A0   (23)
#define PIN_A1   (24)
#define PIN_A2   (25)
#define PIN_A3   (26)
#define PIN_A4   (27)
#define PIN_A5   (28)
#define PIN_A6   (19)
#define PIN_A7   (22)

static const uint8_t A0 = PIN_A0;
static const uint8_t A1 = PIN_A1;
static const uint8_t A2 = PIN_A2;
static const uint8_t A3 = PIN_A3;
static const uint8_t A4 = PIN_A4;
static const uint8_t A5 = PIN_A5;
static const uint8_t A6 = PIN_A6;
static const uint8_t A7 = PIN_A7;

// TODO
#define digitalPinToPCICR(p)    (((p) >= 0 && (p) <= 21) ? (&PCICR) : (uint8_t*) 0)
#define digitalPinToPCICRbit(p) (((p) <= 7) ? 2 : (((p) <= 13) ? 0 : 1)) // TODO
#define digitalPinToPCMSK(p)    (((p) <= 2 || (p) > 29 || ((p) >= 9 && (p) <= 11)) ? (&PCMSK2) : ((p) <= 17) ? (&PCMSK0) : ((p) < 29) ? (&PCMSK1) : (uint8_t*) 0)
//#define digitalPinToPCMSK(p)    (((p) <= 7) ? (&PCMSK2) : (((p) <= 13) ? (&PCMSK0) : (((p) <= 21) ? (&PCMSK1) : (uint8_t*) 0)))
#define digitalPinToPCMSKbit(p) ((p) == 1 ? 3 : (p) == 2 ? 4 : (p) >= 30 ? (p - 30) : (p) >= 23 ? (p - 23) : (p) >= 12 ? (p - 12) : (p) >= 9 ? (p - 4) : (p) >= 7 ? (p - 1) : -1)
//#define digitalPinToPCMSKbit(p) (((p) <= 7) ? (p) : (((p) <= 13) ? ((p) - 8) : ((p) - 14)))
#define enablePCINT(p)					*digitalPinToPCMSK(p) |= (1<<digitalPinToPCMSKbit(p))

#define digitalPinToInterrupt(p)  ((p) == 32 ? 0 : ((p) == 1 ? 1 : NOT_AN_INTERRUPT))

extern const uint8_t PROGMEM analog_pin_to_channel_PGM[];
#define analogPinToChannel(p)  (p == 19 ? 6 : p == 22 ? 7 : p >= 23 & p <= 28 ? p - 23 : -1 )

#ifdef ARDUINO_MAIN

// On the Arduino board, digital pins are also used
// for the analog output (software PWM).  Analog input
// pins are a separate set.

// ATMEL ATMEGA8 & 168 / ARDUINO
//
//                  +-\/-+
//            PC6  1|    |28  PC5 (AI 5)
//      (D 0) PD0  2|    |27  PC4 (AI 4)
//      (D 1) PD1  3|    |26  PC3 (AI 3)
//      (D 2) PD2  4|    |25  PC2 (AI 2)
// PWM+ (D 3) PD3  5|    |24  PC1 (AI 1)
//      (D 4) PD4  6|    |23  PC0 (AI 0)
//            VCC  7|    |22  GND
//            GND  8|    |21  AREF
//            PB6  9|    |20  AVCC
//            PB7 10|    |19  PB5 (D 13)
// PWM+ (D 5) PD5 11|    |18  PB4 (D 12)
// PWM+ (D 6) PD6 12|    |17  PB3 (D 11) PWM
//      (D 7) PD7 13|    |16  PB2 (D 10) PWM
//      (D 8) PB0 14|    |15  PB1 (D 9) PWM
//                  +----+
//
// (PWM+ indicates the additional PWM pins on the ATmega168.)

// these arrays map port names (e.g. port B) to the
// appropriate addresses for various functions (e.g. reading
// and writing)
const uint16_t PROGMEM port_to_mode_PGM[] = {
	NOT_A_PORT,
	NOT_A_PORT,
	(uint16_t) &DDRB,
	(uint16_t) &DDRC,
	(uint16_t) &DDRD,
};

const uint16_t PROGMEM port_to_output_PGM[] = {
	NOT_A_PORT,
	NOT_A_PORT,
	(uint16_t) &PORTB,
	(uint16_t) &PORTC,
	(uint16_t) &PORTD,
};

const uint16_t PROGMEM port_to_input_PGM[] = {
	NOT_A_PORT,
	NOT_A_PORT,
	(uint16_t) &PINB,
	(uint16_t) &PINC,
	(uint16_t) &PIND,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
	NOT_A_PIN,
	PD, // PD3
	PD, // PD4
	NOT_A_PIN, // GND
	NOT_A_PIN, // VCC
	NOT_A_PIN, // GND
	NOT_A_PIN, // VCC
	PB, // PB6
	PB, // PB7
	PD, // PD5
	PD, // PD6
	PD, // PD7
	PB, // PB0
	PB, // PB1
	PB, // PB2
	PB, // PB3
	PB, // PB4
	PB, // PB5
	NOT_A_PIN, // VCC
	NOT_A_PIN, // ADC6
	NOT_A_PIN, // AREF
	NOT_A_PIN, // GND
	NOT_A_PIN, // ADC7
	PC, // PC0
	PC, // PC1
	PC, // PC2
	PC, // PC3
	PC, // PC4
	PC, // PC5
	PC, // PC6
	PD, // PD0
	PD, // PD1
	PD, // PD2
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
    0,
	_BV(3), // PD3
	_BV(4), // PD4
	0, // GND
	0, // VCC
	0, // GND
	0, // VCC
	_BV(6), // PB6
	_BV(7), // PB7
	_BV(5), // PD5
	_BV(6), // PD6
	_BV(7), // PD7
	_BV(0), // PB0
	_BV(1), // PB1
	_BV(2), // PB2
	_BV(3), // PB3
	_BV(4), // PB4
	_BV(5), // PB5
	0, // VCC
	0, // ADC6
	0, // AREF
	0, // GND
	0, // ADC7
	_BV(0), // PC0
	_BV(1), // PC1
	_BV(2), // PC2
	_BV(3), // PC3
	_BV(4), // PC4
	_BV(5), // PC5
	_BV(6), // PC6
	_BV(0), // PD0
	_BV(1), // PD1
	_BV(2), // PD2
};

const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
	NOT_ON_TIMER,
	TIMER2B, // PD3
	NOT_ON_TIMER, // PD4
	NOT_ON_TIMER, // GND
	NOT_ON_TIMER, // VCC
	NOT_ON_TIMER, // GND
	NOT_ON_TIMER, // VCC
	NOT_ON_TIMER, // PB6
	NOT_ON_TIMER, // PB7
	TIMER0B, // PD5
	TIMER0A, // PD6
	NOT_ON_TIMER, // PD7
	NOT_ON_TIMER, // PB0
	TIMER1A, // PB1
	TIMER1B, // PB2
	TIMER2A, // PB3
	NOT_ON_TIMER, // PB4
	NOT_ON_TIMER, // PB5
	NOT_ON_TIMER, // VCC
	NOT_ON_TIMER, // ADC6
	NOT_ON_TIMER, // AREF
	NOT_ON_TIMER, // GND
	NOT_ON_TIMER, // ADC7
	NOT_ON_TIMER, // PC0
	NOT_ON_TIMER, // PC1
	NOT_ON_TIMER, // PC2
	NOT_ON_TIMER, // PC3
	NOT_ON_TIMER, // PC4
	NOT_ON_TIMER, // PC5
	NOT_ON_TIMER, // PC6
	NOT_ON_TIMER, // PD0
	NOT_ON_TIMER, // PD1
	NOT_ON_TIMER, // PD2
};

#endif /* ARDUINO_MAIN */

// These serial port names are intended to allow libraries and architecture-neutral
// sketches to automatically default to the correct port name for a particular type
// of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
// the first hardware serial port whose RX/TX pins are not dedicated to another use.
//
// SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
//
// SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
//
// SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
//
// SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
//
// SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
//                            pins are NOT connected to anything by default.
#define SERIAL_PORT_MONITOR   Serial
#define SERIAL_PORT_HARDWARE  Serial

#if PCB_VERSION == 3
#define BOARD_COUNT 6
#else
#define BOARD_COUNT 5
#endif


// TODO: Update for new version / create a new header / define a version macro
#if PCB_VERSION == 1
// L2 MSK1
#define ENCL2A (25)
#define ENCL2B (26)
// L1 MSK2
#define ENCL1A (2)
#define ENCL1B (32)
// M MSK2
#define ENC1A (31)
#define ENC1B (30)
// R1 MSK0
#define ENCR1A (8)
#define ENCR1B (7)
// R2 MSK1 & MSK0
#define ENCR2A (23)
#define ENCR2B (13)

// TODO: check the values below
// MSK2
#define SWL (1)
#define SWM (11)
#define SWR (9)
#define SWITCH_PORT (PIND)
#elif PCB_VERSION == 2 // TODO: these correspond to pins in i2c_encoder_4.*
#define ENC1A (30)
#define ENC1B (26)

// TODO: check the values below
#define ENCL1A (1)
#define ENCL1B (2)
#define ENCL2A (32)
#define ENCL2B (31)
#define ENCR1A (7)
#define ENCR1B (8)
#define ENCR2A (12)
#define ENCR2B (13)
#define SWM (23)
#define SWL (24)
#define SWR (25)
#define SWITCH_PORT (PINC)
#elif PCB_VERSION == 3
#define ENCM1A (32)
#define ENCM1B (1)
#define ENCM2A (7)
#define ENCM2B (8)
#define ENCL1A (23)
#define ENCL1B (24)
#define ENCL2A (31)
#define ENCL2B (30)
#define ENCR1A (9)
#define ENCR1B (10)
#define ENCR2A (12)
#define ENCR2B (13)
#define SWM (26)
#define SWL (22)
#define SWR (19)
#endif

#define TOUCH (11)

#if PCB_VERSION == 3
#define LEDL (25)
#define LEDM (2)
#define LEDR (11)
#else
#if PCB_VERSION == 1
#define LED1 (24)
#elif PCB_VERSION == 2
#define LED1 (9)
#endif
#define LED2 (10)
#endif

#if PCB_VERSION != 3
// TODO: On v2 SWM is connected to TOUCH -> Not an analog input
#define POTM SWM
#define POTL SWL
#define POTR SWR
#endif

#define NOT_POSSIBLE (-1)

// TODO: Update for new version / create a new header / define a version macro
// TODO: Use an array for defining PCMSK registers to use?
#if PCB_VERSION == 1
/*
static const uint8_t ENC1A_INT = PCINT17;
static const uint8_t ENC1B_INT = PCINT16;
static const uint8_t ENCL1A_INT = PCINT20;
static const uint8_t ENCL1B_INT = PCINT18;
static const uint8_t ENCL2A_INT = PCINT10;
static const uint8_t ENCL2B_INT = PCINT11;
static const uint8_t ENCR1A_INT = PCINT7;
static const uint8_t ENCR1B_INT = PCINT6;
static const uint8_t ENCR2A_INT = PCINT8;
static const uint8_t ENCR2B_INT = PCINT1;

//NOTE: Current implementation expects the switches to be on the same port
#define SWM_INT PCINT23
#define SWL_INT PCINT19
#define SWR_INT PCINT21
 */
#elif PCB_VERSION == 2
static const uint8_t ENC1A_INT = PCINT16;
static const uint8_t ENC1B_INT = PCINT11;
// TODO: make sure the pins are correct below
static const uint8_t ENCL1A_INT = PCINT19;
static const uint8_t ENCL1B_INT = PCINT20;
static const uint8_t ENCL2A_INT = PCINT18;
static const uint8_t ENCL2B_INT = PCINT17;
static const uint8_t ENCR1A_INT = PCINT6;
static const uint8_t ENCR1B_INT = PCINT7;
static const uint8_t ENCR2A_INT = PCINT0;
static const uint8_t ENCR2B_INT = PCINT1;

//NOTE: Current implementation expects the switches to be on the same port
#define SWM_INT PCINT8
#define SWL_INT PCINT9
#define SWR_INT PCINT10
#elif PCB_VERSION == 3
static const uint8_t ENCL1A_INT = PCINT8;
static const uint8_t ENCL1B_INT = PCINT9;
static const uint8_t ENCL2A_INT = PCINT17;
static const uint8_t ENCL2B_INT = PCINT16;
static const uint8_t ENCM1A_INT = PCINT18;
static const uint8_t ENCM1B_INT = PCINT19;
static const uint8_t ENCM2A_INT = PCINT6;
static const uint8_t ENCM2B_INT = PCINT7;
static const uint8_t ENCR1A_INT = PCINT21;
static const uint8_t ENCR1B_INT = PCINT22;
static const uint8_t ENCR2A_INT = PCINT0;
static const uint8_t ENCR2B_INT = PCINT1;

//NOTE: Current implementation expects the switches to be on the same port
#define SWL_INT NOT_POSSIBLE
#define SWM_INT PCINT11
#define SWR_INT NOT_POSSIBLE
#endif

#if PCB_VERSION != 3
static const uint8_t TOUCH_INT = PCINT23;
#endif

#if PCB_VERSION == 3 // TODO: add interrupt mask validation (i.e. != -1)
static const uint8_t SW_INTS_MASK = (1 << digitalPinToPCMSKbit(SWM));
#else
static const uint8_t SW_INTS_MASK = (1 << digitalPinToPCMSKbit(SWL)) | (1 << digitalPinToPCMSKbit(SWM)) | (1 << digitalPinToPCMSKbit(SWR));
#endif

#if PCB_VERSION == 3
static const uint8_t SW_INTS[] = {(byte) NOT_POSSIBLE, (byte) NOT_POSSIBLE, digitalPinToPCMSKbit(SWM), (byte) NOT_POSSIBLE};
#else
static const uint8_t SW_INTS[] = {(byte) NOT_POSSIBLE, digitalPinToPCMSKbit(SWL), digitalPinToPCMSKbit(SWM), digitalPinToPCMSKbit(SWR)};
#endif
/*
static const uint8_t TOUCH_INTS[] = {(byte) NOT_POSSIBLE, SWL_INT, PCINT23, SWR_INT};
*/
#endif
