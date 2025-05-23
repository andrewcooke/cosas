
#ifndef COSA_WAVELIB_H
#define COSA_WAVELIB_H

#include <memory>
#include <vector>

#include "wavetable.h"


class Manager;  // forwards def


class Wavelib {

public:

  Wavelib();
  Wavetable& operator[](size_t idx);
  size_t size();

  friend class Manager;  // needs to see the size_t "constants" below
  
private:
  
  void init_wavetables();
  
  size_t sine_start;
  size_t sine_gamma_1;
  size_t square_start;
  size_t square_duty_05;
  size_t saw_start;
  size_t saw_offset_0;
  size_t noise_start;
  size_t noise_smooth_1;

  std::unique_ptr<std::vector<std::unique_ptr<Wavetable>>> all_wavetables;

};


#endif
