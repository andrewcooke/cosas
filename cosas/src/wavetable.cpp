
#include <algorithm>
#include <random>
#include <cmath>

#include "cosas/constants.h"
#include "cosas/maths.h"
#include "cosas/wavetable.h"


Square::Square(float duty) : duty_idx(duty * FULL_TABLE_SIZE) {}

int16_t Square::next(int32_t tick) const {
  size_t full_idx = tick2idx(tick);
  if (full_idx <= duty_idx) return SAMPLE_MAX;
  else return -SAMPLE_MAX;
}


QuarterWtable::QuarterWtable() : quarter_table() {}

// handle symmetry of triangular or sine wave
int16_t QuarterWtable::next(int32_t tick) const {
  size_t full_idx = tick2idx(tick);
  size_t quarter_idx = full_idx % QUARTER_TABLE_SIZE;
  if (full_idx < QUARTER_TABLE_SIZE) return quarter_table.at(quarter_idx);
  else if (full_idx < 2 * QUARTER_TABLE_SIZE) return quarter_table.at(QUARTER_TABLE_SIZE - 1 - quarter_idx);
  else if (full_idx < 3 * QUARTER_TABLE_SIZE) return -quarter_table.at(quarter_idx);
  else return -quarter_table.at(QUARTER_TABLE_SIZE - 1 - quarter_idx);
}


Sine::Sine(float gamma) {
  for (size_t i = 0; i < quarter_table.size(); i++) {
    float shape = sin(2 * M_PI * i / FULL_TABLE_SIZE);
    if (gamma != 1) { shape = pow(shape, gamma); }
    quarter_table.at(i) = clip_16(shape * SAMPLE_MAX);
  }
}

WTriangle::WTriangle() {
  for (size_t i = 0; i < QUARTER_TABLE_SIZE; i++) {
    quarter_table.at(i) = clip_16(SAMPLE_MAX * (i / static_cast<float>(QUARTER_TABLE_SIZE)));
  }
}


// constant for division with shift
const int64_t k = (static_cast<int64_t>(SAMPLE_MAX) << 32) / (SAMPLE_RATE / 4);

int16_t Triangle::next(int32_t tick) const {
  size_t full_idx = tick2idx(tick);
  size_t quarter_idx = full_idx % QUARTER_TABLE_SIZE;
  if (full_idx < QUARTER_TABLE_SIZE) return clip_16(static_cast<int64_t>(quarter_idx * k) >> 32);
  else if (full_idx < 2 * QUARTER_TABLE_SIZE) return clip_16(
    static_cast<int64_t>((QUARTER_TABLE_SIZE - 1 - quarter_idx) * k) >> 32);
  else if (full_idx < 3 * QUARTER_TABLE_SIZE) return clip_16(static_cast<int64_t>(-quarter_idx * k) >> 32);
  else return clip_16(static_cast<int64_t>(-(QUARTER_TABLE_SIZE - 1 - quarter_idx) * k) >> 32);
}


HalfWtable::HalfWtable() : half_table() {}

int16_t HalfWtable::next(int32_t tick) const {
  size_t full_idx = tick2idx(tick);
  size_t half_idx = full_idx % HALF_TABLE_SIZE;
  if (full_idx < HALF_TABLE_SIZE) return half_table.at(half_idx);
  else return -half_table.at(HALF_TABLE_SIZE - 1 - half_idx);
}


WSaw::WSaw(float offset) {
  size_t peak_index = HALF_TABLE_SIZE * (1 + offset) / 2;
  for (size_t i = 0; i < peak_index; i++) {
    half_table.at(i) = clip_16(SAMPLE_MAX * (i / static_cast<float>(peak_index)));
  }
  for (size_t i = peak_index; i < HALF_TABLE_SIZE; i++) {
    half_table.at(i) = clip_16(
      SAMPLE_MAX * ((HALF_TABLE_SIZE - i) / static_cast<float>(HALF_TABLE_SIZE - 1 - peak_index)));
  }
}


FullWtable::FullWtable() : full_table() {}

int16_t FullWtable::next(int32_t tick) const {
  size_t full_idx = tick2idx(tick);
  return full_table.at(full_idx);
}


Saw::Saw(float offset) :
  peak_idx(QUARTER_TABLE_SIZE + offset * QUARTER_TABLE_SIZE),
  k1((static_cast<int64_t>(SAMPLE_MAX) << 32) / static_cast<int64_t>((1 + offset) * SAMPLE_RATE / 4)),
  k2((static_cast<int64_t>(SAMPLE_MAX) << 32) / static_cast<int64_t>((1 - offset) * SAMPLE_RATE / 4)) {}

int16_t Saw::next(int32_t tick) const {
  size_t full_idx = tick2idx(tick);
  if (full_idx < peak_idx) return clip_16((static_cast<int64_t>(full_idx) * k1) >> 32);
  else if (full_idx < HALF_TABLE_SIZE) return clip_16((static_cast<int64_t>(HALF_TABLE_SIZE - full_idx) * k2) >> 32);
  else if (full_idx < FULL_TABLE_SIZE - peak_idx) return clip_16(
    (-static_cast<int64_t>(full_idx - HALF_TABLE_SIZE) * k2) >> 32);
  else return clip_16((-static_cast<int64_t>(FULL_TABLE_SIZE - full_idx) * k1) >> 32);
}


Noise::Noise(size_t smooth) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(SAMPLE_MIN, SAMPLE_MAX);
  for (size_t i = 0; i < FULL_TABLE_SIZE; i++) {
    full_table.at(i) = distrib(gen);
  }
  if (smooth > 1) {
    std::array<float, FULL_TABLE_SIZE> smoothed;
    std::fill(smoothed.begin(), smoothed.end(), 0);
    for (size_t i = 0; i < FULL_TABLE_SIZE; i++) {
      for (size_t j = 0; j < smooth; j++) {
        smoothed.at(i) += full_table.at((i + j) % FULL_TABLE_SIZE);
      }
    }
    float norm = std::max(*std::max_element(smoothed.begin(), smoothed.end()),
                          -1 * *std::min_element(smoothed.begin(), smoothed.end()));
    for (size_t i = 0; i < FULL_TABLE_SIZE; i++) {
      full_table.at(i) = clip_16(smoothed.at(i) * SAMPLE_MAX / norm);
    }
  }
}


PolyTable::PolyTable(size_t shape, size_t asym, size_t offset) {
  make_half(half_table, shape % N_SHAPES, 0, offset);
  make_half(half_table, (shape + asym) % N_SHAPES, offset, HALF_TABLE_SIZE);
}

float PolyTable::pow2(float x, size_t n) {
  while (n-- > 0) x = x * x;
  return x;
}

float PolyTable::tox(size_t i, size_t lo, size_t hi) {
  return lo ? static_cast<float>(hi - i) / static_cast<float>(hi - lo)
            : static_cast<float>(i - lo) / static_cast<float>(hi - lo);
}

void PolyTable::make_concave(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t shape, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = static_cast<int16_t>(SAMPLE_MAX * pow2(tox(i, lo, hi), shape));
}

void PolyTable::make_linear(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = static_cast<int16_t>(SAMPLE_MAX * tox(i, lo, hi));
}

void PolyTable::make_convex(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t shape, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = static_cast<int16_t>(SAMPLE_MAX * (1 - pow2(1 - tox(i, lo, hi), shape)));
}

void PolyTable::make_sine(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = static_cast<int16_t>(SAMPLE_MAX * sin(M_PI * tox(i, lo, hi) / 2.0f));
}

void PolyTable::make_noise(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(SAMPLE_MIN, SAMPLE_MAX);
  for (size_t i = lo; i < hi; i++) table.at(i) = static_cast<int16_t>(SAMPLE_MAX * distrib(gen));
}

void PolyTable::make_square(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi) {
  for (size_t i = lo; i < hi; i++) table.at(i) = static_cast<int16_t>(lo ? SAMPLE_MAX : SAMPLE_MIN);
}

void PolyTable::make_half(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t shape, size_t lo, size_t hi) {
  shape = shape % N_SHAPES;
  if (shape == NOISE) make_noise(table, lo, hi);
  else if (shape < LINEAR) make_concave(table, LINEAR - shape + 1, lo, hi);
  else if (shape == LINEAR) make_linear(table, lo, hi);
  else if (shape == SINE) make_sine(table, lo, hi);
  else if (shape < SQUARE) make_convex(table, shape - SINE + 1, lo, hi);
  else make_square(table, lo, hi);
}

