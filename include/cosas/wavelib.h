
#ifndef COSA_WAVELIB_H
#define COSA_WAVELIB_H

#include <memory>
#include <vector>

#include "cosas/wavetable.h"


class Wavelib {

public:

  Wavelib();
  Wavetable& operator[](size_t idx);
  size_t size();

  // ideally, these would be constants
  size_t sine_start;
  size_t sine_gamma_1;
  size_t square_start;
  size_t square_duty_05;
  size_t saw_start;
  size_t saw_offset_0;
  size_t noise_start;
  size_t noise_smooth_1;

private:
  
  void init_wavetables();
  
  std::unique_ptr<std::vector<std::unique_ptr<Wavetable>>> all_wavetables;

};


#endif
