#ifndef FILTER_DEF_H_
#define FILTER_DEF_H_

const int MASK_N = 2;
const int MASK_X = 3;
const int MASK_Y = 3;

const int SOBEL_FILTER_R_ADDR = 0x00000000;
const int SOBEL_FILTER_RESULT_ADDR = 0x00000004;
const int SOBEL_FILTER_CHECK_ADDR = 0x00000008;

union word {
  int sint[2];
  unsigned int uint[2];
  unsigned char uc[8];
};

#endif
