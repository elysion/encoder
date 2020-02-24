#ifndef TYPES_H
#define TYPES_H

enum EncoderType {
  ENCODER_TYPE_RELATIVE,
  ENCODER_TYPE_ABSOLUTE
};

enum Board {
  BOARD_L2,
  BOARD_L1,
#if PCB_VERSION == 3
  BOARD_M1,
  BOARD_M2,
#else
  BOARD_M,
#endif
  BOARD_R1,
  BOARD_R2
};

#endif //TYPES_H
