
#include <memory>
#include <stdexcept>

#include "cosas/engine_old.h"
#include "cosas/modulators.h"

constexpr int DEFAULT_BOXCAR = 1000;


OldManager::OldManager() : wavelib(std::move(std::make_unique<Wavelib>())) {};

PhaseSource& OldManager::build(OldManager::OldEngine engine) {

  clear_all();

  switch (engine)
  {
  case OldManager::OldEngine::DEX:
    return build_dex();
  case OldManager::OldEngine::POLY:
    return build_poly();
  case OldManager::OldEngine::FM_SIMPLE:
    return build_fm_simple();
  case OldManager::OldEngine::FM_LFO:
    return build_fm_lfo();
  case OldManager::OldEngine::FM_ENV:
    return build_fm_env();
  case OldManager::OldEngine::FM_FB:
    return build_fm_fb();
  case OldManager::OldEngine::CHORD:
    return build_chord();
  default:
    throw std::domain_error("missing case in Manager::build?");
  }
}

// panes:
//   1 - freq/dex/arg
std::tuple<AbsFreqParam&, PhaseSource&> OldManager::add_abs_dex_osc(float frq, size_t widx, Param& right) {
  auto& o = add_source<AbsDexOsc>(frq, *wavelib, widx);
  AbsFreqParam& f = o.get_freq_param();
  WavedexMixin::WavedexParam& w = o.get_dex_param();
  add_pane(f, w, right);
  return {f, o};
}

// panes:
//   1 - freq/dex/blk
std::tuple<AbsFreqParam&, PhaseSource&> OldManager::add_abs_dex_osc(float frq, size_t widx) {
  return add_abs_dex_osc(frq, widx, add_param<Blank>());
}

// panes:
//   1 - freq/dex/gain
std::tuple<AbsFreqParam&, PhaseSource&> OldManager::add_abs_dex_osc_w_gain(float frq, size_t widx, float amp) {
  auto& b = add_param<Blank>();
  auto [f, o] = add_abs_dex_osc(frq, widx, b);
  Gain& g = add_source<Gain>(o, amp, 100);  // todo - no idea if hi ok here
  b.unblank(&g.get_amp());
  return {f, g};
}

// panes:
//   1 - freq/dex/det
PhaseSource& OldManager::add_rel_dex_osc(AbsFreqParam& root, size_t widx, float r, float d) {
  auto& o = add_source<RelDexOsc>(*wavelib, widx, root, r, d);
  RelFreqParam& f = o.get_freq_param();
  add_pane(root, o.get_dex_param(), f.get_det_param());
  return o;
}

// panes:
//   1 - freq/dex/blk
PhaseSource& OldManager::build_dex() {
  auto [f, o] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  return o;
}

// panes:
//   1 - freq/blank/blk
//   2 - off/shp/asym
PhaseSource& OldManager::build_poly() {
  return add_abs_poly_osc(10, PolyTable::LINEAR - 1, 0, static_cast<size_t>(0.1f * QUARTER_TABLE_SIZE));
}

// panes:
//   1 - freq/dex/blk
//   2 - freq/dex/det
//   3 - gain/wet/blk
PhaseSource& OldManager::build_fm_simple() {
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  PhaseSource& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  // i don't understand this 3.  is it subtick_bits?
  PhaseSource& fm = add_fm(c, m, 0.5, 1.0 / (1 << (PHI_FUDGE_BITS - 3)));
  return fm;
}

// panes:
//   1 - freq/dex/blk
//   2 - freq/dex/det
//   3 - freq/dex/blk
//   4 - gain/wet/blk
PhaseSource& OldManager::build_fm_lfo() {
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  PhaseSource& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  auto [lf, l] = add_abs_dex_osc_w_gain(1, wavelib->sine_gamma_1, 1);
  PhaseSource& am = add_source<AM>(l, m);
  PhaseSource& fm = add_fm(c, am, 0.5, 1.0 / (1 << (PHI_FUDGE_BITS - 4)));
  return fm;
}

// panes:
//   1 - freq/dex/freq
//   2 - freq/dex/det
//   3 - gain/wet/blk
//   4 - off/shp/asym
PhaseSource& OldManager::build_fm_env() {
  PhaseSource& fm = build_fm_simple();
  AbsPolyOsc& e = add_abs_poly_osc(1, PolyTable::LINEAR - 1, 0, static_cast<size_t>(0.1f * QUARTER_TABLE_SIZE));
  PhaseSource& am = add_source<AM>(e, fm);
  dynamic_cast<Blank&>(get_pane(0).y).unblank(&e.get_freq_param());
  return am;
}

// panes:
//   1 - freq/dex/len
//   2 - freq/dex/det
//   3 - gain/wet/fb
PhaseSource& OldManager::build_fm_fb() {
  auto& latch = add_source<Latch>();
  auto& flt = add_source<Boxcar>(latch, DEFAULT_BOXCAR);
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1, flt.get_len());
  PhaseSource& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  Merge& mrg = add_balance(flt, m, 0.5);
  PhaseSource& fm = add_fm(c, mrg, 0.5, 1.0 / (1 << (PHI_FUDGE_BITS - 4)), mrg.get_weight(0));
  latch.set_source(&fm);
  return latch;
}

// panes:
//   1 - freq/dex/wt0
//   2 - wt1/wt2/wt3  (rotated to posn)
//   3 - freq/dex/det
//   4 - freq/dex/det
//   5 - freq/dex/det
PhaseSource& OldManager::build_chord() {
  auto& b = add_param<Blank>();
  auto [f0, o0] = add_abs_dex_osc(440, wavelib->sine_gamma_1, b);
  PhaseSource& o1 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 5 / 4.0f, 1);
  PhaseSource& o2 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 3 / 2.0f, 1);
  PhaseSource& o3 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 4 / 3.0f, 1);
  auto& m = add_source<Merge>(o0, 0.5);
  b.unblank(&m.get_weight(0));
  m.add_source(o1, 1);
  m.add_source(o2, 1);
  m.add_source(o3, 1);
  add_pane(m.get_weight(1), m.get_weight(2), m.get_weight(3));
  rotate_panes(4, 1);
  return m;
}

