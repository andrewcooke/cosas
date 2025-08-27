
#ifndef COSAS_ENGINE_OLD_H
#define COSAS_ENGINE_OLD_H


#include <tuple>
#include <memory>

#include "cosas/engine_base.h"
#include "cosas/wavelib.h"
#include "cosas/oscillator.h"
#include "cosas/params.h"


// initial manager/engines that used too much memory


class OldManager : public BaseManager{

public:

  enum OldEngine {
    DEX,
    POLY,
    FM_SIMPLE,
    FM_LFO,
    FM_ENV,
    FM_FB,
    CHORD,
  };
  static constexpr size_t N_ENGINE = CHORD + 1;

  OldManager();
  RelSource& build(OldEngine);

private:

  std::tuple<AbsFreqParam&, RelSource&> add_abs_dex_osc(float frq, size_t widx, Param& right);
  std::tuple<AbsFreqParam&, RelSource&> add_abs_dex_osc(float frq, size_t widx);
  std::tuple<AbsFreqParam&, RelSource&> add_abs_dex_osc_w_gain(float frq, size_t widx, float amp);
  RelSource& add_rel_dex_osc(AbsFreqParam& root, size_t widx, float r, float d);

  RelSource& build_dex();
  RelSource& build_poly();
  RelSource& build_fm_simple();
  RelSource& build_fm_lfo();
  RelSource& build_fm_env();
  RelSource& build_fm_fb();
  RelSource& build_chord();

  std::unique_ptr<Wavelib> wavelib;
};


#endif
