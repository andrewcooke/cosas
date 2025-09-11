
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
  PhaseSource& build(OldEngine);

private:

  std::tuple<AbsFreqParam&, PhaseSource&> add_abs_dex_osc(float frq, size_t widx, Param& right);
  std::tuple<AbsFreqParam&, PhaseSource&> add_abs_dex_osc(float frq, size_t widx);
  std::tuple<AbsFreqParam&, PhaseSource&> add_abs_dex_osc_w_gain(float frq, size_t widx, float amp);
  PhaseSource& add_rel_dex_osc(AbsFreqParam& root, size_t widx, float r, float d);

  PhaseSource& build_dex();
  PhaseSource& build_poly();
  PhaseSource& build_fm_simple();
  PhaseSource& build_fm_lfo();
  PhaseSource& build_fm_env();
  PhaseSource& build_fm_fb();
  PhaseSource& build_chord();

  std::unique_ptr<Wavelib> wavelib;
};


#endif
