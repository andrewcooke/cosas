
#include <algorithm>
#include <numbers>
#include <random>
#include <cmath>

#include "doctest.h"
#include "constants.h"
#include "maths.h"
#include "lookup.h"


Square::Square(float duty) : duty_idx(duty * full_wavetable_size) {};

int16_t Square::next(int64_t tick, int32_t phi) {
  size_t full_idx = (tick + phi) % full_wavetable_size;
  if (full_idx <= duty_idx) return sample_max;
  else return -sample_max;
};


// handle symmetry of triangular or sine wave
int16_t QuarterWtable::next(int64_t tick, int32_t phi) {
  size_t quarter_table_size = quarter_table.size();
  size_t full_idx = (tick + phi) % full_wavetable_size;
  size_t quarter_idx = full_idx % quarter_table_size;
  if (full_idx < quarter_table_size) return quarter_table.at(quarter_idx);
  else if (full_idx < 2 * quarter_table_size) return quarter_table.at(quarter_table_size - 1 - quarter_idx);
  else if (full_idx < 3 * quarter_table_size) return -quarter_table.at(quarter_idx);
  else return -quarter_table.at(quarter_table_size - 1 - quarter_idx);
}


Sine::Sine(float gamma) {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    float shape = sin(2 * std::numbers::pi * i / full_wavetable_size);
    if (gamma != 1) {shape = pow(shape, gamma);}
    quarter_table.at(i) = clip_16(shape);
  }
};


// can't find a compact way to do this without division (and ayway we
// use saw)
Triangle::Triangle() {
  size_t quarter_table_size = quarter_table.size();
  for (size_t i = 0; i < quarter_table_size; i++) {
    quarter_table.at(i) = clip_16(sample_max * (i / (float)quarter_table_size));
  }
};


int16_t HalfWtable::next(int64_t tick, int32_t phi) {
  size_t half_table_size = half_table.size();
  size_t full_idx = (tick + phi) % full_wavetable_size;
  size_t half_idx = full_idx % half_table_size;
  if (full_idx < half_table_size) return half_table.at(half_idx);
  else return -half_table.at(half_table_size - 1 - half_idx);
}


// again, can't find way to do a compact version without division.
Saw::Saw(float offset) {
  size_t half_table_size = half_table.size();
  size_t peak_index = half_table_size * (1 + offset) / 2;
  for (size_t i = 0; i < peak_index; i++) {
    half_table.at(i) = clip_16(sample_max * (i / (float)peak_index));
  }
  for (size_t i = peak_index; i < half_table_size; i++) {
    half_table.at(i) = clip_16(sample_max * ((half_table_size - i) / (float)(half_table_size - 1 - peak_index)));
  }
};

TEST_CASE("Saw") {
  Saw s = Saw(0.5);
  size_t i = 123;
  CHECK(s.next(i, 0) == -s.next(sample_rate - i, 0));
  CHECK(s.next(i, 0) == s.next(sample_rate/2 - i, 0));
}


int16_t FullWtable::next(int64_t tick, int32_t phi) {
  size_t full_idx = (tick + phi) % full_table.size();
  return full_table.at(full_idx);
}


Noise::Noise(int smooth) {
  const size_t full_wavetable_size = full_table.size();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, sample_max);
  for (size_t i = 0; i < full_wavetable_size; i++) {
    full_table.at(i) = distrib(gen);
  }
  if (smooth > 1) {
    std::array<int32_t, full_wavetable_size> smoothed;
    std::fill(smoothed.begin(), smoothed.end(), 0);
    for (size_t i = 0; i < full_wavetable_size; i++) {
      for (size_t j = 0; j < smooth; j++) {
	smoothed.at(i) += full_table.at((i + j) % full_wavetable_size);
      }
    }
    float norm = std::max(*std::max_element(smoothed.begin(), smoothed.end()), -1 * *std::min_element(smoothed.begin(), smoothed.end()));
    for (size_t i = 0; i < full_wavetable_size; i++) {
      full_table.at(i) = clip_16((float)smoothed.at(i) / norm);
    }
  }
};

