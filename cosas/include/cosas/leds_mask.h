
#ifndef COSAS_LEDS_MASK_H
#define COSAS_LEDS_MASK_H


#include <stddef.h> // size_t
#include <cstdint>


// a compact description of leds that we can build on for animation
// TODO - should most of these be static?!

class BaseLEDsMask {

public:
  static constexpr size_t N = 6;
  explicit BaseLEDsMask(uint8_t bits);
  virtual ~BaseLEDsMask() = default;
  uint32_t reverse(uint32_t mask);
  uint32_t vinterp(size_t off, uint32_t a, uint32_t b);
  uint32_t hinterp(size_t off, uint32_t a, uint32_t b);
  uint32_t constant(uint8_t fg);
  uint32_t ring(float normalized, bool highlight);
  uint32_t square(bool bottom, uint8_t amplitude);
  uint32_t vbar(bool right, uint8_t amplitude);
  uint32_t rot2dot(size_t off, uint8_t fg, uint8_t bg);
  uint32_t wiggle19(size_t off, uint8_t fg, uint8_t bg);
  uint32_t rotate(uint32_t mask, size_t n);
  virtual void show(uint32_t mask) = 0;
  const uint8_t BITS;
  const uint32_t FULL_MASK;
  const uint32_t BITS_MASK;

private:
  const uint32_t SIDE_MASK;
  const size_t ring_order[N] = {4, 2, 0, 1, 3, 5};
  uint32_t overwrite(uint32_t mask, float centre, float width, uint8_t amplitude);
  const uint8_t wiggle[19] = {
    0b101010, 0b001010, 0b001110, 0b000110, 0b000111,
    0b000011, 0b001011, 0b001001, 0b001101, 0b001100,
    0b101100, 0b100100, 0b110100, 0b110000, 0b111000,
    0b011000, 0b011100, 0b010100, 0b010101
  };

};


#endif
