
#ifndef COSAS_WAVETABLE_H
#define COSAS_WAVETABLE_H


#include <array>

#include "cosas/constants.h"
#include "cosas/source.h"


// since fp is slow we generally use lookup tables for waveforms.
// note that we can use fp to generate tables because it's done
// up-front (even, we could load from disc).

class Wavetable : public AbsSource {};


class Square final : public Wavetable {
public:
  Square() : Square(0.5) {};
  explicit Square(float duty);
  [[nodiscard]] int16_t next(int32_t tick) const override;
private:
  size_t duty_idx;
};


class QuarterWtable : public Wavetable {
public:
  QuarterWtable();
  [[nodiscard]] int16_t next(int32_t tick) const override;
protected:
  std::array<int16_t, QUARTER_TABLE_SIZE> quarter_table;
};


class Sine final : public QuarterWtable {
public:
  Sine() : Sine(1) {};
  explicit Sine(float gamma);
};


// this uses a wavetable and a lot more memory
class WTriangle final : public QuarterWtable {
public:
  WTriangle();
};


// this uses division by multiplication and a lot less memory
class Triangle final : public Wavetable {
public:
  Triangle() = default;
  [[nodiscard]] int16_t next(int32_t tick) const override;
};


class HalfWtable : public Wavetable {
public:
  HalfWtable();
  [[nodiscard]] int16_t next(int32_t tick) const override;
protected:
  std::array<int16_t, HALF_TABLE_SIZE> half_table;
};


class WSaw final : public HalfWtable {
public:
  // a regular saw can be done with a quarter wavetable, but with a
  // different unpacking logic.  for full generalility, however, we
  // need half (and even then it's complex).  offset from -1 to 1.
  explicit WSaw(float offset);
};


// this uses division by multiplication and a lot less memory
class Saw final : public Wavetable {
public:
  explicit Saw(float offset);
  [[nodiscard]] int16_t next(int32_t tick) const override;
private:
  size_t peak_idx;
  int64_t k1;
  int64_t k2;
};


class FullWtable : public Wavetable {
public:
  FullWtable();
  [[nodiscard]] int16_t next(int32_t tick) const override;
protected:
  std::array<int16_t, FULL_TABLE_SIZE> full_table;
};


class Noise final : public FullWtable {
public:
  Noise() : Noise(1) {};
  explicit Noise(size_t smooth);
};


class PolyTable final : public HalfWtable {
public:
  PolyTable(size_t shape, size_t asym, int offset);
  static constexpr size_t N_CONCAVE = 4;
  static constexpr size_t N_CONVEX = 4;
  static constexpr size_t N_NOISE = 2;
  static constexpr size_t NOISE = 0;
  static constexpr size_t CONCAVE = N_NOISE;
  static constexpr size_t LINEAR = CONCAVE + N_CONCAVE;
  static constexpr size_t SINE = LINEAR + 1;
  static constexpr size_t CONVEX = SINE + 1;
  static constexpr size_t SQUARE = CONVEX + N_CONVEX;
  static constexpr size_t ZERO = SQUARE + 1;
  static constexpr size_t N_SHAPES = ZERO + 1;
private:
  static float pow2(float x, size_t n);
  static float tox(size_t i, size_t lo, size_t hi);
  static void make_concave(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t shape, size_t lo, size_t hi);
  static void make_linear(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi);
  static void make_convex(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t shape, size_t lo, size_t hi);
  static void make_sine(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi);
  static void make_noise_std(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi);
  static void make_noise_32(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi);
  static void make_noise_bit(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi);
  static void make_square(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t lo, size_t hi);
  static void make_constant(std::array<int16_t, HALF_TABLE_SIZE>& table, int16_t value, size_t lo, size_t hi);
  static void make_half(std::array<int16_t, HALF_TABLE_SIZE>& table, size_t shape, size_t lo, size_t hi);
};


#endif
