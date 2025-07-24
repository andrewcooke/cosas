
#ifndef COSAS_DNL_H
#define COSA_DNL_H

#include <cstdint>
#include <tuple>


int16_t fix_dnl(uint16_t adc);


// old code used in tests/dnl to understand/optimise the fixes

int16_t fix_dnl_ac_pxy(uint16_t adc, int x, int y);
int16_t fix_dnl_ac_pxyz(uint16_t adc, int x, int y, int z);

uint16_t fix_dnl_cj(uint16_t adc);
int16_t fix_dnl_cj_px(uint16_t adc, int x);
int16_t fix_dnl_cx_px(uint16_t adc, int x);


template <typename ...Args>
class ScaledDNL {
public:
  explicit ScaledDNL(int16_t(*correcn)(uint16_t, Args... args), int16_t lo, int16_t hi, Args... args);;
  int16_t operator()(uint16_t adc) const;
private:
  std::tuple<Args...> args;
  int16_t offset;
  uint32_t scale;
  int16_t(*correcn)(uint16_t, Args...);
};

template <typename... Args>
ScaledDNL<Args...>::ScaledDNL(int16_t(*correcn)(uint16_t, Args... args), int16_t lo, int16_t hi, Args... args)
  : args(args...), correcn(correcn) {
  if (correcn) {
    uint16_t one = correcn(0xfff, args...);
    scale = (0xfff << 19) / (one - hi);
    offset = lo;
  } else {
    scale = (0xfff << 19) / (0xfff - hi);
    offset = lo;
  }
}

template <typename... Args>
int16_t ScaledDNL<Args...>::operator()(uint16_t adc) const {
  if (! correcn) {
    return static_cast<int16_t>(((static_cast<int32_t>(adc) * scale) >> 19) + offset);
  } else {
    auto all_args = std::tuple_cat(std::make_tuple(adc), args);
    auto corr = static_cast<int16_t>(std::apply(correcn, all_args));
    return static_cast<int16_t>(((static_cast<int32_t>(corr) * scale) >> 19) + offset);
  }
}


#endif
