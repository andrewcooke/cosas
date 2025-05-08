
import std;
using namespace std;

#include "constants.h"
#include "oscillator.h"


uint16_t QuarterWtable::next(int64_t tick, int32_t phi) const {
  size_t quarter_table_size = quarter_table.size();
  size_t full_idx = (tick + phi) % full_table_size;
  size_t quarter_idx = full_idx % quarter_table_size;
  if (full_idx < quarter_table_size) return sample_zero + quarter_table.at(quarter_idx);
  else if (full_idx < 2 * quarter_table_size) return sample_zero + quarter_table.at(quarter_table_size - 1 - quarter_idx);
  else if (full_idx < 3 * quarter_table_size) return sample_zero - quarter_table.at(quarter_idx);
  else return sample_zero - quarter_table.at(quarter_table_size - 1 - quarter_idx);
}


Sine::Sine() {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    quarter_table.at(i) = (sample_zero - 1) * sin(2 * numbers::pi * i / full_table_size);
  }
};


Square::Square() {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    quarter_table.at(i) = sample_zero - 1;
  }
};


Triangle::Triangle() {
  size_t quarter_table_size = quarter_table.size();
  for (size_t i = 0; i < quarter_table_size; i++) {
    quarter_table.at(i) = (sample_zero - 1) * (i / (float)quarter_table_size);
  }
};


InterpQWtable::InterpQWtable(QuarterWtable& wtable1, QuarterWtable& wtable2, float weight1) {
  Balance b = Balance(weight1);
  for (size_t i = 0; i < quarter_table.size(); i++) {
    quarter_table.at(i) = b.combine(wtable1.raw(i), wtable2.raw(i));
  }
}


uint16_t HalfWtable::next(int64_t tick, int32_t phi) const {
  size_t half_table_size = half_table.size();
  size_t full_idx = (tick + phi) % full_table_size;
  size_t half_idx = full_idx % half_table_size;
  if (full_idx < half_table_size) return sample_zero + half_table.at(half_idx);
  else return sample_zero - half_table.at(half_table_size - 1 - half_idx);
}


Saw::Saw(float offset) {
  size_t half_table_size = half_table.size();
  size_t peak_index = half_table_size * (1 + offset) / 2;
  for (size_t i = 0; i < peak_index; i++) {
    half_table.at(i) = (sample_zero - 1) * (i / (float)peak_index);
  }
  for (size_t i = peak_index; i < half_table_size; i++) {
    half_table.at(i) = (sample_zero - 1) * ((half_table_size - i) / (float)(half_table_size - 1 - peak_index));
  }
};


uint16_t FullWtable::next(int64_t tick, int32_t phi) const {
  size_t full_idx = (tick + phi) % full_table.size();
  return full_table.at(full_idx);
}


WhiteNoise::WhiteNoise() {
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> distrib(0, 1 << bit_depth - 1);
  for (size_t i = 0; i < full_table.size(); i++) {
    full_table.at(i) = distrib(gen);
  }
};


Oscillator::Oscillator(const Wavetable& wave, const Amplitude& amp, uint16_t freq, const Multiplier& mult)
  : wavetable(wave), amplitude(amp), frequency(freq), multiplier(mult) {};

Oscillator::Oscillator(const Wavetable& wave, const Amplitude& amp, uint16_t freq)
  : wavetable(wave), amplitude(amp), frequency(freq), multiplier(unit_mult) {};

uint16_t Oscillator::next(int64_t tick, int32_t phi) const {
  return amplitude.scale(wavetable.next(tick * multiplier.scale(frequency), phi));
}
