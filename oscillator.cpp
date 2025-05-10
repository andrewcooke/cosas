
import std;
using namespace std;

#include "constants.h"
#include "maths.h"
#include "oscillator.h"


Square::Square(float duty) : duty_idx(duty * full_table_size) {};

uint16_t Square::next(int64_t tick, int32_t phi) const {
  size_t full_idx = (tick + phi) % full_table_size;
  if (full_idx <= duty_idx) return sample_max;
  else return 0;
};


uint16_t QuarterWtable::next(int64_t tick, int32_t phi) const {
  size_t quarter_table_size = quarter_table.size();
  size_t full_idx = (tick + phi) % full_table_size;
  size_t quarter_idx = full_idx % quarter_table_size;
  if (full_idx < quarter_table_size) return sample_zero + quarter_table.at(quarter_idx);
  else if (full_idx < 2 * quarter_table_size) return sample_zero + quarter_table.at(quarter_table_size - 1 - quarter_idx);
  else if (full_idx < 3 * quarter_table_size) return sample_zero - quarter_table.at(quarter_idx);
  else return sample_zero - quarter_table.at(quarter_table_size - 1 - quarter_idx);
}


Sine::Sine(float gamma) {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    float shape = sin(2 * numbers::pi * i / full_table_size);
    if (gamma != 1) {shape = pow(shape, gamma);}
    quarter_table.at(i) = clip_u16(sample_zero * shape);
  }
};


Triangle::Triangle() {
  size_t quarter_table_size = quarter_table.size();
  for (size_t i = 0; i < quarter_table_size; i++) {
    quarter_table.at(i) = clip_u16(sample_zero * (i / (float)quarter_table_size));
  }
};


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
    half_table.at(i) = clip_u16(sample_zero * (i / (float)peak_index));
  }
  for (size_t i = peak_index; i < half_table_size; i++) {
    half_table.at(i) = clip_u16(sample_zero * ((half_table_size - i) / (float)(half_table_size - 1 - peak_index)));
  }
};


uint16_t FullWtable::next(int64_t tick, int32_t phi) const {
  size_t full_idx = (tick + phi) % full_table.size();
  return full_table.at(full_idx);
}


Noise::Noise(int smooth) {
  const size_t full_table_size = full_table.size();
  random_device rd;
  mt19937 gen(rd());
  uniform_int_distribution<> distrib(0, sample_max);
  for (size_t i = 0; i < full_table_size; i++) {
    full_table.at(i) = distrib(gen);
  }
  if (smooth > 1) {
    array<int32_t, full_table_size> smoothed;
    fill(smoothed.begin(), smoothed.end(), 0);
    for (size_t i = 0; i < full_table_size; i++) {
      for (size_t j = 0; j < smooth; j++) {
	smoothed.at(i) += ((int32_t)full_table.at((i + j) % full_table_size) - sample_zero);
      }
    }
    float norm = max(*max_element(smoothed.begin(), smoothed.end()), -1 * *min_element(smoothed.begin(), smoothed.end()));
    for (size_t i = 0; i < full_table_size; i++) {
      full_table.at(i) = clip_u16((sample_zero * (float)smoothed.at(i)) / norm + sample_zero);
    }
  }
};


Oscillator::Oscillator(Wavetable& wave, unique_ptr<Amplitude> amp, unique_ptr<Frequency> freq)
  : wavetable(wave), amplitude(move(amp)), frequency(move(freq)) {};

uint16_t Oscillator::next(int64_t tick, int32_t phi) const {
  return amplitude->scale(wavetable.next(tick * frequency->get(), phi));
}

void Oscillator::set_freq_abs(uint16_t freq) {
  frequency = move(make_unique<AbsoluteFreq>(freq));
}

void Oscillator::set_freq_ratio(const AbsoluteFreq& root, float ratio) {
  frequency = move(make_unique<RelativeFreq>(root, ratio));
}

const Frequency& Oscillator::get_frequency() {
  return *frequency;
}
