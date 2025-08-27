
#ifndef COSAS_ENGINE_H
#define COSAS_ENGINE_H

#include <tuple>
#include <vector>
#include <memory>

#include "cosas/transformers.h"
#include "cosas/wavelib.h"
#include "cosas/oscillator.h"
#include "cosas/params.h"
#include "cosas/pane.h"


// initial manager/engines that used too much memory


class OldManager {

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
  explicit OldManager(bool t);
  RelSource& build(OldEngine);
  [[nodiscard]] Pane& get_pane(size_t n) const;
  [[nodiscard]] size_t n_panes() const;
  [[nodiscard]] size_t n_dex() const;

private:

  template<typename SourceType, typename... Args> SourceType& add_source(Args&&... args);
  template<typename ParamType, typename... Args> ParamType& add_param(Args&&... args);
  template<typename ParamType, typename... Args> ParamType& add_input(Args&&... args);
  Pane& add_pane(Param& top, Param& left, Param& right) const;
  void swap_panes(size_t i, size_t j) const;
  void rotate_panes(size_t a, size_t b) const;
  std::tuple<AbsFreqParam&, RelSource&> add_abs_dex_osc(float frq, size_t widx, Param& right);
  std::tuple<AbsFreqParam&, RelSource&> add_abs_dex_osc(float frq, size_t widx);
  std::tuple<AbsFreqParam&, RelSource&> add_abs_dex_osc_w_gain(float frq, size_t widx, float amp);
  std::tuple<AbsFreqParam&, RelSource&> add_abs_poly_osc(float frq, size_t shp, size_t asym, size_t off);
  std::tuple<AbsFreqParam&, RelSource&> add_abs_poly_osc_w_gain(float frq, size_t shp, size_t asym, size_t off, float amp);
  RelSource& add_rel_dex_osc(AbsFreqParam& root, size_t widx, float r, float d);
  Merge& add_balance(RelSource& a, RelSource& b, float bal);
  RelSource& add_fm(RelSource& c,  RelSource& m, float bal, float amp);
  RelSource& add_fm(RelSource& c,  RelSource& m, float bal, float amp, Param& right);

  RelSource& build_dex();
  RelSource& build_poly();
  RelSource& build_fm_simple();
  RelSource& build_fm_lfo();
  RelSource& build_fm_env();
  RelSource& build_fm_fb();
  RelSource& build_chord();

  std::unique_ptr<Wavelib> wavelib;
  std::unique_ptr<std::vector<std::unique_ptr<RelSource>>> current_sources;
  std::unique_ptr<std::vector<std::unique_ptr<Param>>> current_params;
  std::unique_ptr<std::vector<std::unique_ptr<Pane>>> current_panes;
  bool test;
};


#endif
