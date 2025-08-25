
#include <memory>
#include <stdexcept>

#include "cosas/modulators.h"
#include "cosas/engine.h"


constexpr int DEFAULT_BOXCAR = 1000;


Manager::Manager() : Manager(false) {};

Manager::Manager(const bool t)
  : wavelib(std::move(std::make_unique<Wavelib>())),
    current_sources(std::move(std::make_unique<std::vector<std::unique_ptr<RelSource>>>())),
    current_params(std::move(std::make_unique<std::vector<std::unique_ptr<Param>>>())),
    current_panes(std::move(std::make_unique<std::vector<std::unique_ptr<Pane>>>())),
    test(t) {};

RelSource& Manager::build(Manager::Engine engine) {
  current_sources->clear();
  current_params->clear();
  current_panes->clear();

  switch (engine)
  {
  case Manager::Engine::DEX:
    return build_dex();
  case Manager::Engine::POLY:
    return build_poly();
  case Manager::Engine::FM_SIMPLE:
    return build_fm_simple();
  case Manager::Engine::FM_LFO:
    return build_fm_lfo();
  case Manager::Engine::FM_ENV:
    return build_fm_env();
  case Manager::Engine::FM_FB:
    return build_fm_fb();
  case Manager::Engine::CHORD:
    return build_chord();
  default:
    throw std::domain_error("missing case in Manager::build?");
  }
}

const Pane& Manager::get_pane(size_t n) const {
  return *current_panes->at(n);
}

size_t Manager::n_panes() const {
  return current_panes->size();
}

size_t Manager::n_dex() const {
  return wavelib->size();
}


template <typename SourceType, typename... Args>
SourceType& Manager::add_source(Args&&... args) {
  std::unique_ptr<SourceType> source = std::make_unique<SourceType>(std::forward<Args>(args)...);
  current_sources->push_back(std::move(source));
  return dynamic_cast<SourceType&>(*current_sources->back());
}

template <typename ParamType, typename... Args>
ParamType& Manager::add_input(Args&&... args) {
  std::unique_ptr<ParamType> input = std::make_unique<ParamType>(std::forward<Args>(args)...);
  current_params->push_back(std::move(input));
  return dynamic_cast<ParamType&>(*current_params->back());
}

Pane& Manager::add_pane(Param& top, Param& left, Param& right) const {
  std::unique_ptr<Pane> pane = std::make_unique<Pane>(top, left, right);
  current_panes->push_back(std::move(pane));
  return *current_panes->back();
}

void Manager::swap_panes(size_t i, size_t j) const {
  std::unique_ptr<Pane> tmp = std::move(current_panes->at(i));
  current_panes->at(i) = std::move(current_panes->at(j));
  current_panes->at(j) = std::move(tmp);
}

// a is the one that jumps furthest
void Manager::rotate_panes(const size_t a, const size_t b) const {
  // ReSharper disable once CppDFAConstantConditions
  if (a < b) {
    // ReSharper disable once CppDFAUnreachableCode
    std::unique_ptr<Pane> tmp = std::move(current_panes->at(a));
    for (size_t i = a; i < b; i++) current_panes->at(i) = std::move(current_panes->at(i + 1));
    current_panes->at(b) = std::move(tmp);
  } else {
    std::unique_ptr<Pane> tmp = std::move(current_panes->at(a));
    for (size_t i = a; i > b; i--) current_panes->at(i) = std::move(current_panes->at(i - 1));
    current_panes->at(b) = std::move(tmp);
  }
}

// panes:
//   1 - freq/dex/arg
std::tuple<AbsFreqParam&, RelSource&> Manager::add_abs_dex_osc(float frq, size_t widx, Param& right) {
  auto& o = add_source<AbsDexOsc>(frq, *wavelib, widx);
  AbsFreqParam& f = o.get_freq_param();
  WavedexMixin::WavedexParam& w = o.get_dex_param();
  add_pane(f, w, right);
  return {f, o};
}

// panes:
//   1 - freq/dex/blk
std::tuple<AbsFreqParam&, RelSource&> Manager::add_abs_dex_osc(float frq, size_t widx) {
  return add_abs_dex_osc(frq, widx, add_input<Blank>());
}

// panes:
//   1 - freq/dex/gain
std::tuple<AbsFreqParam&, RelSource&> Manager::add_abs_dex_osc_w_gain(float frq, size_t widx, float amp) {
  auto& b = add_input<Blank>();
  auto [f, o] = add_abs_dex_osc(frq, widx, b);
  Gain& g = add_source<Gain>(o, amp, 100);  // todo - no idea if hi ok here
  b.unblank(&g.get_amp());
  return {f, g};
}

// panes:
//   1 - freq/dex/det
RelSource& Manager::add_rel_dex_osc(AbsFreqParam& root, size_t widx, float r, float d) {
  auto& o = add_source<RelDexOsc>(*wavelib, widx, root, r, d);
  RelFreqParam& f = o.get_freq_param();
  add_pane(root, o.get_dex_param(), f.get_det_param());
  return o;
}

// panes:
//   1 - off/shp/asym
//   (freq not mapped)
std::tuple<AbsFreqParam&, RelSource&> Manager::add_abs_poly_osc(float frq, size_t shp, size_t asym, size_t off) {
  auto& o = add_source<AbsPolyOsc>(frq, shp, asym, off);
  AbsFreqParam& f = o.get_freq_param();
  add_pane(o.get_off_param(), o.get_shp_param(), o.get_asym_param());
  return {f, o};
}

// panes:
//   1 - freq/blk/gain
//   2 - off/shp/asym
std::tuple<AbsFreqParam&, RelSource&> Manager::add_abs_poly_osc_w_gain(const float frq, size_t shp, size_t asym,
                                                                  const size_t off, float amp) {
  auto& top = add_input<Blank>();
  auto& left = add_input<Blank>();
  auto& right = add_input<Blank>();
  add_pane(top, left, right);
  auto [f, o] = add_abs_poly_osc(frq, shp, asym, off);
  top.unblank(&f);
  Gain& g = add_source<Gain>(o, amp, 100);  // todo - hi?
  right.unblank(&g.get_amp());
  return {f, g}; // TODO - currently unused.  why are we returning these?
}

Merge& Manager::add_balance(RelSource& a, RelSource& b, float bal) {
  auto& m = add_source<Merge>(a, bal);
  m.add_source(b, 1);
  return m;
}

// panes:
//   1 - gain/wet/blk
RelSource& Manager::add_fm(RelSource& c, RelSource& m, float bal, float amp) {
  return add_fm(c, m, bal, amp, add_input<Blank>());
}

// panes:
//   1 - gain/wet/arg
RelSource& Manager::add_fm(RelSource& c, RelSource& m, float bal, float amp, Param& right) {
  Gain& g = add_source<Gain>(m, amp, 100);  // todo - hi
  FM& fm = add_source<FM>(c, g);
  Merge& b = add_balance(fm, c, bal);
  add_pane(g.get_amp(), b.get_weight(0), right);
  return b;
}

// panes:
//   1 - freq/dex/blk
RelSource& Manager::build_dex() {
  auto [f, o] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  return o;
}

// panes:
//   1 - freq/blank/blk
//   2 - off/shp/asym
RelSource& Manager::build_poly() {
  auto [f, o] = add_abs_poly_osc(10, PolyTable::LINEAR - 1, 0,
                                                   static_cast<size_t>(0.1f * QUARTER_TABLE_SIZE));
  return o;
}

// panes:
//   1 - freq/dex/blk
//   2 - freq/dex/det
//   3 - gain/wet/blk
RelSource& Manager::build_fm_simple() {
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  RelSource& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  // i don't understand this 3.  is it subtick_bits?
  RelSource& fm = add_fm(c, m, 0.5, 1.0 / (1 << (PHI_FUDGE_BITS - 3)));
  return fm;
}

// panes:
//   1 - freq/dex/blk
//   2 - freq/dex/det
//   3 - freq/dex/blk
//   4 - gain/wet/blk
RelSource& Manager::build_fm_lfo() {
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  RelSource& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  auto [lf, l] = add_abs_dex_osc_w_gain(1, wavelib->sine_gamma_1, 1);
  RelSource& am = add_source<AM>(l, m);
  RelSource& fm = add_fm(c, am, 0.5, 1.0 / (1 << (PHI_FUDGE_BITS - 4)));
  return fm;
}

// panes:
//   1 - freq/dex/freq
//   2 - freq/dex/det
//   3 - gain/wet/blk
//   4 - off/shp/asym
RelSource& Manager::build_fm_env() {
  RelSource& fm = build_fm_simple();
  auto [ef, e] = add_abs_poly_osc(1, PolyTable::LINEAR - 1, 0,
                                                      static_cast<size_t>(0.1f * QUARTER_TABLE_SIZE));
  RelSource& am = add_source<AM>(e, fm);
  dynamic_cast<Blank&>(get_pane(0).right).unblank(&ef);
  return am;
}

// panes:
//   1 - freq/dex/len
//   2 - freq/dex/det
//   3 - gain/wet/fb
RelSource& Manager::build_fm_fb() {
  auto& latch = add_source<Latch>();
  auto& flt = add_source<Boxcar>(latch, DEFAULT_BOXCAR);
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1, flt.get_len());
  RelSource& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  Merge& mrg = add_balance(flt, m, 0.5);
  RelSource& fm = add_fm(c, mrg, 0.5, 1.0 / (1 << (PHI_FUDGE_BITS - 4)), mrg.get_weight(0));
  latch.set_source(&fm);
  return latch;
}

// panes:
//   1 - freq/dex/wt0
//   2 - wt1/wt2/wt3  (rotated to posn)
//   3 - freq/dex/det
//   4 - freq/dex/det
//   5 - freq/dex/det
RelSource& Manager::build_chord() {
  auto& b = add_input<Blank>();
  auto [f0, o0] = add_abs_dex_osc(440, wavelib->sine_gamma_1, b);
  RelSource& o1 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 5 / 4.0f, 1);
  RelSource& o2 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 3 / 2.0f, 1);
  RelSource& o3 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 4 / 3.0f, 1);
  auto& m = add_source<Merge>(o0, 0.5);
  b.unblank(&m.get_weight(0));
  m.add_source(o1, 1);
  m.add_source(o2, 1);
  m.add_source(o3, 1);
  add_pane(m.get_weight(1), m.get_weight(2), m.get_weight(3));
  rotate_panes(4, 1);
  return m;
}

