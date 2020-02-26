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

enum EncoderDirection {
  ENCODE_DIRECTION_CW = 1,
  ENCODE_DIRECTION_CCW = -1
};

#endif //TYPES_H
