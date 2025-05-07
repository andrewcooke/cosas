
import std;
using namespace std;

#include "constants.h"
#include "oscillator.h"


Wavetable::Wavetable() : quarter_table() {
  for (int i = 0; i < table_size; i++) {
    quarter_table[i] = (sample_zero - 1) * sin(2 * numbers::pi * i / sample_rate);
  }
};

uint16_t Wavetable::next(int64_t tick, int32_t phi) const {
  uint16_t index = (uint16_t)((tick + phi) % sample_rate);
  if (index < table_size) return quarter_table[index % table_size] + sample_zero;
  else if (index < 2 * table_size) return quarter_table[table_size - 1 - (index % table_size)] + sample_zero;
  else if (index < 3 * table_size) return sample_zero - quarter_table[index % table_size];
  else return sample_zero - quarter_table[table_size - 1 - (index % table_size)];
}

// these were used in early testing but are maybe not needed later

uint16_t Wavetable::at_uint16_t(uint64_t tick, StepScale scale) const {
  return next(scale.scale(tick), 0);
}

float Wavetable::at_float(uint64_t tick, StepScale scale) const {
  return ((at_uint16_t(tick, scale) - sample_zero) / (float)sample_zero);
}


Oscillator::Oscillator(Wavetable wave, AmpScale vol, uint16_t freq, Multiplier mult) : wavetable(wave), volume(vol), frequency(freq), multiplier(mult);

Oscillator::Oscillator(Wavetable wave, AmpScale vol, uint16_t freq) : wavetable(wave), volume(vol), frequency(freq), multiplier(unit_mult);

Oscillator uint16_t next(int64_t tick, int32_t phi) const override {
  return amplitude.scale(wavetable.next(tick * multiplier.scale(frequency) + phi));
}
