
#ifndef COSA_WAVETABLE_H
#define COSA_WAVETABLE_H

#include <cstdint>
#include <array>

#include "cosas/constants.h"
#include "cosas/source.h"


// since fp is slow we generally use lookup tables for waveforms.
// note that we can use fp to generate tables because it's done
// up-front (even, we could load from disc).

class Wavetable : public Source {};


class Square : public Wavetable {

public:

  Square() : Square(0.5) {};
  Square(float duty);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  size_t duty_idx;
  
};


class QuarterWtable : public Wavetable {

public:
  
  int16_t next(int32_t tick, int32_t phi) const override;
  
protected:

  std::array<int16_t, quarter_table_size> quarter_table;

};


class Sine : public QuarterWtable {

public:

  Sine() : Sine(1) {};
  Sine(float gamma);

};


// this uses a wavetable and a lot more memory
class WTriangle : public QuarterWtable {

public:

  WTriangle();

};


// this uses division by multiplication and a lot less memory
class Triangle : public Wavetable {

public:

  Triangle() = default;
  int16_t next(int32_t tick, int32_t phi) const override;

};
  

class HalfWtable : public Wavetable {

public:
  
  int16_t next(int32_t tick, int32_t phi) const override;
  
protected:

  std::array<int16_t, half_table_size> half_table;

};


class WSaw : public HalfWtable {

public:

  // a regular saw can be done with a quarter wavetable, but with a
  // different unpacking logic.  for full generalility, however, we
  // need half (and even then it's complex).  offset from -1 to 1.
  WSaw(float offset);

};


// this uses division by multiplication and a lot less memory
class Saw : public Wavetable {

public:

  Saw(float offset);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  size_t peak_idx;
  int64_t k1;
  int64_t k2;
  
};
  

class FullWtable : public Wavetable {

public:
  
  int16_t next(int32_t tick, int32_t phi) const override;
  
protected:

  std::array<int16_t, full_table_size> full_table;

};


class Noise : public FullWtable {

public:

  Noise() : Noise(1) {};
  Noise(size_t smooth);

};


class PolyTable : public HalfWtable {

public:

  PolyTable(size_t shape, size_t asym, size_t offset);
  static const size_t n_concave = 4;
  static const size_t n_convex = 4;
  static const size_t noise = 0;
  static const size_t linear = n_concave + 1;
  static const size_t sine = n_concave + 2;
  static const size_t square = n_concave + n_convex + 3;
  static const size_t n_shapes = square + 1;

private:
  
  float pow2(float x, size_t n);
  float tox(size_t i, size_t lo, size_t hi);
  void make_concave(std::array<int16_t, half_table_size>& table, size_t shape, size_t lo, size_t hi);
  void make_linear(std::array<int16_t, half_table_size>& table, size_t lo, size_t hi);
  void make_convex(std::array<int16_t, half_table_size>& table, size_t shape, size_t lo, size_t hi);
  void make_sine(std::array<int16_t, half_table_size>& table, size_t lo, size_t hi);
  void make_noise(std::array<int16_t, half_table_size>& table, size_t lo, size_t hi);
  void make_square(std::array<int16_t, half_table_size>& table, size_t lo, size_t hi);
  void make_half(std::array<int16_t, half_table_size>& table, size_t shape, size_t lo, size_t hi);

};


#endif
