
#include <algorithm>
#include <numbers>
#include <random>
#include <cmath>

#include "doctest.h"
#include "constants.h"
#include "maths.h"
#include "wavetable.h"


Square::Square(float duty) : duty_idx(duty * full_table_size) {};

int16_t Square::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table_size;
  if (full_idx <= duty_idx) return sample_max;
  else return -sample_max;
};


// handle symmetry of triangular or sine wave
int16_t QuarterWtable::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table_size;
  size_t quarter_idx = full_idx % quarter_table_size;
  if (full_idx < quarter_table_size) return quarter_table.at(quarter_idx);
  else if (full_idx < 2 * quarter_table_size) return quarter_table.at(quarter_table_size - 1 - quarter_idx);
  else if (full_idx < 3 * quarter_table_size) return -quarter_table.at(quarter_idx);
  else return -quarter_table.at(quarter_table_size - 1 - quarter_idx);
}


Sine::Sine(float gamma) {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    float shape = sin(2 * std::numbers::pi * i / full_table_size);
    if (gamma != 1) {shape = pow(shape, gamma);}
    quarter_table.at(i) = clip_16(shape * sample_max);
  }
};

WTriangle::WTriangle() {
  size_t quarter_table_size = quarter_table.size();
  for (size_t i = 0; i < quarter_table_size; i++) {
    quarter_table.at(i) = clip_16(sample_max * (i / (float)quarter_table_size));
  }
};


// constant for division with shift
const int64_t k = ((int64_t)sample_max << 32) / (sample_rate / 4);

int16_t Triangle::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table_size;
  size_t quarter_idx = full_idx % quarter_table_size;
  if (full_idx < quarter_table_size) return clip_16((int64_t)(quarter_idx * k) >> 32);
  else if (full_idx < 2 * quarter_table_size) return clip_16((int64_t)((quarter_table_size - 1 - quarter_idx) * k) >> 32);
  else if (full_idx < 3 * quarter_table_size) return clip_16((int64_t)(-quarter_idx * k) >> 32);
  else return clip_16((int64_t)(-(quarter_table_size - 1 - quarter_idx) * k) >> 32);
}


int16_t HalfWtable::next(int32_t tick, int32_t phi) const {
  size_t half_table_size = half_table.size();
  size_t full_idx = (tick + phi) % full_table_size;
  size_t half_idx = full_idx % half_table_size;
  if (full_idx < half_table_size) return half_table.at(half_idx);
  else return -half_table.at(half_table_size - 1 - half_idx);
}


WSaw::WSaw(float offset) {
  size_t half_table_size = half_table.size();
  size_t peak_index = half_table_size * (1 + offset) / 2;
  for (size_t i = 0; i < peak_index; i++) {
    half_table.at(i) = clip_16(sample_max * (i / (float)peak_index));
  }
  for (size_t i = peak_index; i < half_table_size; i++) {
    half_table.at(i) = clip_16(sample_max * ((half_table_size - i) / (float)(half_table_size - 1 - peak_index)));
  }
};


int16_t FullWtable::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table.size();
  return full_table.at(full_idx);
}


Saw::Saw(float offset) :
  peak_idx(quarter_table_size + offset * quarter_table_size),
  k1(((int64_t)sample_max << 32) / (int64_t)((1 + offset) * sample_rate / 4)),
  k2(((int64_t)sample_max << 32) / (int64_t)((1 - offset) * sample_rate / 4)) {};

int16_t Saw::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table_size;
  if (full_idx < peak_idx) return clip_16((int64_t)(full_idx * k1) >> 32);
  else if (full_idx < half_table_size) return clip_16((int64_t)((half_table_size - full_idx) * k2) >> 32);
  else if (full_idx < full_table_size - peak_idx) return clip_16((int64_t)(-(full_idx - half_table_size) * k2) >> 32);
  else return clip_16((int64_t)(-(full_table_size - full_idx) * k1) >> 32);
}


Noise::Noise(int smooth) {
  const size_t full_table_size = full_table.size();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(sample_min, sample_max);
  for (size_t i = 0; i < full_table_size; i++) {
    full_table.at(i) = distrib(gen);
  }
  if (smooth > 1) {
    std::array<float, full_table_size> smoothed;
    std::fill(smoothed.begin(), smoothed.end(), 0);
    for (size_t i = 0; i < full_table_size; i++) {
      for (size_t j = 0; j < smooth; j++) {
	smoothed.at(i) += full_table.at((i + j) % full_table_size);
      }
    }
    float norm = std::max(*std::max_element(smoothed.begin(), smoothed.end()), -1 * *std::min_element(smoothed.begin(), smoothed.end()));
    for (size_t i = 0; i < full_table_size; i++) {
      full_table.at(i) = clip_16(smoothed.at(i) * sample_max / norm);
    }
  }
};

