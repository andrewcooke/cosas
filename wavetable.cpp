
#include <algorithm>
#include <numbers>
#include <random>
#include <cmath>
#include <iostream>

#include "doctest.h"
#include "constants.h"
#include "maths.h"
#include "wavetable.h"


Square::Square(float duty) : duty_idx(duty * full_table_size) {}

int16_t Square::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table_size;
  if (full_idx <= duty_idx) return sample_max;
  else return -sample_max;
}


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
}

WTriangle::WTriangle() {
  for (size_t i = 0; i < quarter_table_size; i++) {
    quarter_table.at(i) = clip_16(sample_max * (i / static_cast<float>(quarter_table_size)));
  }
}


// constant for division with shift
const int64_t k = (static_cast<int64_t>(sample_max) << 32) / (sample_rate / 4);

int16_t Triangle::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table_size;
  size_t quarter_idx = full_idx % quarter_table_size;
  if (full_idx < quarter_table_size) return clip_16(static_cast<int64_t>(quarter_idx * k) >> 32);
  else if (full_idx < 2 * quarter_table_size) return clip_16(static_cast<int64_t>((quarter_table_size - 1 - quarter_idx) * k) >> 32);
  else if (full_idx < 3 * quarter_table_size) return clip_16(static_cast<int64_t>(-quarter_idx * k) >> 32);
  else return clip_16(static_cast<int64_t>(-(quarter_table_size - 1 - quarter_idx) * k) >> 32);
}


int16_t HalfWtable::next(int32_t tick, int32_t phi) const {
  // careful w signs
  while (tick + phi < static_cast<int32_t>(full_table_size)) tick += static_cast<int32_t>(full_table_size);
  size_t full_idx = static_cast<size_t>(tick) % full_table_size;
  size_t half_idx = full_idx % half_table_size;
  if (full_idx < half_table_size) return half_table.at(half_idx);
  else return -half_table.at(half_table_size - 1 - half_idx);
}


WSaw::WSaw(float offset) {
  size_t peak_index = half_table_size * (1 + offset) / 2;
  for (size_t i = 0; i < peak_index; i++) {
    half_table.at(i) = clip_16(sample_max * (i / static_cast<float>(peak_index)));
  }
  for (size_t i = peak_index; i < half_table_size; i++) {
    half_table.at(i) = clip_16(sample_max * ((half_table_size - i) / static_cast<float>(half_table_size - 1 - peak_index)));
  }
}


int16_t FullWtable::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table.size();
  return full_table.at(full_idx);
}


Saw::Saw(float offset) :
  peak_idx(quarter_table_size + offset * quarter_table_size),
  k1((static_cast<int64_t>(sample_max) << 32) / static_cast<int64_t>((1 + offset) * sample_rate / 4)),
  k2((static_cast<int64_t>(sample_max) << 32) / static_cast<int64_t>((1 - offset) * sample_rate / 4)) {}

int16_t Saw::next(int32_t tick, int32_t phi) const {
  size_t full_idx = tick2idx(tick + phi) % full_table_size;
  if (full_idx < peak_idx) return clip_16((static_cast<int64_t>(full_idx) * k1) >> 32);
  else if (full_idx < half_table_size) return clip_16((static_cast<int64_t>(half_table_size - full_idx) * k2) >> 32);
  else if (full_idx < full_table_size - peak_idx) return clip_16((-static_cast<int64_t>(full_idx - half_table_size) * k2) >> 32);
  else return clip_16((-static_cast<int64_t>(full_table_size - full_idx) * k1) >> 32);
}


Noise::Noise(uint smooth) {
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
}


PolyTable::PolyTable(size_t shape, size_t asym, size_t offset) {
  make_half(half_table, shape % (square + 1), 0, offset);
  make_half(half_table, (shape + asym) % (square + 1), offset, half_table_size);  
}

float PolyTable::pow2(float x, size_t n) {
  while (n-- > 0) x = x * x;
  return x;
}

float PolyTable::tox(size_t i, size_t lo, size_t hi) {
  return lo ? static_cast<float>(hi - i) / (hi - lo) : static_cast<float>(i - lo) / (hi - lo);
}

void PolyTable::make_concave(std::array<int16_t, half_table_size> table, size_t shape, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = sample_max * (1 - pow2(1 - tox(i, lo, hi), shape));
}

void PolyTable::make_linear(std::array<int16_t, half_table_size> table, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = sample_max * tox(i, lo, hi);
}

void PolyTable::make_convex(std::array<int16_t, half_table_size> table, size_t shape, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = sample_max * pow2(tox(i, lo, hi), shape);
}

void PolyTable::make_sine(std::array<int16_t, half_table_size> table, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = sample_max * sin(std::numbers::pi * tox(i, lo, hi));
}

void PolyTable::make_noise(std::array<int16_t, half_table_size> table, size_t lo, size_t hi) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(sample_min, sample_max);
  for (size_t i = lo; i < hi; i++) table.at(i) = sample_max * distrib(gen);
}

void PolyTable::make_square(std::array<int16_t, half_table_size> table, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = lo ? sample_min : sample_max;
}

void PolyTable::make_half(std::array<int16_t, half_table_size> table, size_t shape, size_t lo, size_t hi) {
  if (shape == noise) make_noise(table, lo, hi);
  else if (shape < linear) make_concave(table, linear - shape + 1, lo, hi);
  else if (shape == linear) make_linear(table, lo, hi);
  else if (shape < square) make_convex(table, shape - linear, lo, hi);
  else make_square(table, lo, hi);
}

