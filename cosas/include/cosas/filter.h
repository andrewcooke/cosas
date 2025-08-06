
#ifndef WEAS_FILTER_H
#define WEAS_FILTER_H

#include <stdint.h>


// https://cytomic.com/files/dsp/DynamicSmoothing.pdf

class SelfModLP {

public:
  SelfModLP(uint8_t bits, uint32_t sample_freq, uint32_t cutoff, float sensitivity);
  uint16_t next(uint16_t in);
  int16_t next(int16_t in);
  uint16_t next_or(uint16_t in, uint16_t same);
  int16_t next_or(int16_t in, int16_t same);

private:
  uint16_t max;
  uint16_t zero;
  uint16_t low1 = 0;
  uint16_t low2 = 0;
  float g0;
  float sense;

};


#endif

