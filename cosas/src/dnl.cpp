
#include "cosas/dnl.h"


// see discussion on discord
uint16_t fix_dnl(uint16_t adc) {
  uint16_t bdc = adc + (((adc + 0x200) >> 10) << 3);
  if (!((adc - 0x01ff) & 0x01ff)) bdc += 4;  // TODO - is this ok?  -ve value but promoted to int
  return bdc;
}


// see https://acooke.org/cute/RaspberryP1.html
uint16_t fix_dnl_old(const uint16_t adc) {
  uint16_t bdc = adc + (((adc + 0x200) >> 10) << 3);
  if ((adc & 0x600) && !(adc & 0x800)) bdc += 3;
  if (!((adc + 0x200) & 0x3ff)) bdc -= 10;
  return bdc;
}


// old code used in tests/dnl to understand/optimise the fixes

inline uint16_t scale_adc(uint16_t adc) {
  return static_cast<uint16_t>((520222 * static_cast<uint32_t>(adc)) >> 19);
}

int16_t fix_dnl_ac_pxy(const uint16_t adc, const int x, const int y) {
  auto bdc = static_cast<int16_t>(adc + (((adc + 0x200) >> 10) << 3));
  if ((adc & 0x600) && !(adc & 0x800)) bdc += y;
  if ((adc + 0x200) % 0x400 == 0) bdc += x;
  return bdc;
}

int16_t fix_dnl_ac_pxyz(const uint16_t adc, const int x, const int y, const int z) {
  auto bdc = static_cast<int16_t>(adc + (((adc + 0x200) >> 10) << 3));
  if ((adc & 0x600) && !(adc & 0x800)) bdc += y;
  if ((adc + 0x200) % 0x400 == 0) bdc += x;
  if (adc & 1) bdc += z;
  return bdc;
}

uint16_t fix_dnl_cj(uint16_t adc) {
  uint16_t adc512 = adc + 512;
  if (!(adc512 % 0x01ff)) adc += 4;
  return scale_adc(adc + ((adc512>>10) << 3));
}

int16_t fix_dnl_cj_px(uint16_t adc, int x) {
  int16_t bdc = static_cast<int16_t>(adc);
  uint16_t adc512 = adc + 0x200;
  if (!(adc512 % 0x01ff)) bdc += x;
  return bdc + ((adc512>>10) << 3);
}

int16_t fix_dnl_cx_px(uint16_t adc, int x) {
  int16_t bdc = static_cast<int16_t>(adc);
  uint16_t adc512 = adc + 0x200;
  if (!(adc512 % 0x0200)) bdc += x;
  return bdc + ((adc512>>10) << 3);
}

int16_t fix_dnl_cj2_pxy(uint16_t adc, int x, int y) {
  auto zz = static_cast<int16_t>(adc + (((adc + 0x200) >> 10) << 3));
  auto delta = static_cast<int16_t>(adc - (0x01ff + x));
  if (! (delta & 0x01ff)) zz += y;
  return zz;
}

int16_t fix_dnl_cj2_pxyz(uint16_t adc, int x, int y, int z) {
  auto zz = static_cast<int16_t>(adc + (((adc + 0x200) >> 10) << 3));
  auto delta = static_cast<int16_t>(adc - (0x01ff + x));
  if (! (delta & 0x01ff)) zz += y;
  if ((adc & 0x600) && !(adc & 0x800)) zz += z;
  return zz;
}

