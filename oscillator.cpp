
import std;
using namespace std;

#include "constants.h"
#include "oscillator.h"


uint16_t Wavetable::next(int64_t tick, int32_t phi) const {
  size_t table_size = quarter_table.size();
  uint16_t full_idx = (tick + phi) % sample_rate;
  uint16_t quarter_idx = full_idx % table_size;
  if (full_idx < table_size) return quarter_table.at(quarter_idx) + sample_zero;
  else if (full_idx < 2 * table_size) return quarter_table.at(table_size - 1 - quarter_idx) + sample_zero;
  else if (full_idx < 3 * table_size) return sample_zero - quarter_table.at(quarter_idx);
  else return sample_zero - quarter_table.at(table_size - 1 - quarter_idx);
}


Sine::Sine() {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    quarter_table.at(i) = (sample_zero - 1) * sin(2 * numbers::pi * i / sample_rate);
  }
};


Square::Square() {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    quarter_table.at(i) = sample_zero - 1;
  }
};


Triangle::Triangle() {
  size_t table_size = quarter_table.size();
  for (size_t i = 0; i < table_size; i++) {
    quarter_table.at(i) = (sample_zero - 1) * (i / (float)table_size);
  }
};


InterpWtable::InterpWtable(Wavetable& wtable1, Wavetable& wtable2, float weight1) {
  Balance b = Balance(weight1);
  for (size_t i = 0; i < quarter_table.size(); i++) {
    quarter_table.at(i) = b.combine(wtable1.raw(i), wtable2.raw(i));
  }
}


Oscillator::Oscillator(const Wavetable& wave, const Amplitude& amp, uint16_t freq, const Multiplier& mult)
  : wavetable(wave), amplitude(amp), frequency(freq), multiplier(mult) {};

Oscillator::Oscillator(const Wavetable& wave, const Amplitude& amp, uint16_t freq)
  : wavetable(wave), amplitude(amp), frequency(freq), multiplier(unit_mult) {};

uint16_t Oscillator::next(int64_t tick, int32_t phi) const {
  return amplitude.scale(wavetable.next(tick * multiplier.scale(frequency), phi));
}
