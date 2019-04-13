byte POT_CHANGE_THRESHOLD = 5;

const byte ENC1A_INT = PCINT16;
const byte ENC1B_INT = PCINT11;
const byte ENCL1A_INT = PCINT19;
const byte ENCL1B_INT = PCINT20;
const byte ENCL2A_INT = PCINT18;
const byte ENCL2B_INT = PCINT17;
const byte ENCR1A_INT = PCINT6;
const byte ENCR1B_INT = PCINT7;
const byte ENCR2A_INT = PCINT0;
const byte ENCR2B_INT = PCINT1;

//NOTE: Current implementation expects the switches to be on the same port
const byte SW1_INT = PCINT8;
const byte SWL_INT = PCINT9;
const byte SWR_INT = PCINT10;
const byte TOUCH_INT = PCINT23;
const byte SW_INTS_MASK = (1 << SW1_INT) | (1 << SWL_INT) | (1 << SWR_INT);
const byte SW_INTS[] = {PCINT9, PCINT8, PCINT10};

enum Board {
  BOARD_L2,
  BOARD_L1,
  BOARD_M,
  BOARD_R1,
  BOARD_R2
};

#define NO_BOARD (0)
#define BOARD_FEATURE_ENCODER _BV(0)
#define BOARD_FEATURE_BUTTON _BV(1)
#define BOARD_FEATURE_POT _BV(2)
#define BOARD_FEATURE_TOUCH _BV(3)
#define BOARD_FEATURE_PADS _BV(4)
#define BOARD_FEATURE_LED _BV(5)

const byte ENCODER_PINS[] = {
  ENCL2A, ENCL2B,
  ENCL1A, ENCL1B,
  ENC1A, ENC1B, 
  ENCR1A, ENCR1B, 
  ENCR2A, ENCR2B
};

const byte BUTTON_PINS[] = {
  0,
  SWL,
  SW1,
  SWR,
  0
};

const byte POT_PINS[] = {
  0,
  POTL,
  POT1,
  POTR,
  0
};


const byte TOUCH_PINS[] = {
  0,
  ENCL2B,
  TOUCH,
  ENCR2B,
  0
};
