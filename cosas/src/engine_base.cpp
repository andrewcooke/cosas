
#include <memory>

#include "cosas/engine_base.h"
#include "cosas/modulators.h"
#include "cosas/debug.h"


BaseManager::BaseManager()
  : current_sources(std::move(std::make_unique<std::vector<std::unique_ptr<RelSource>>>())),
    current_params(std::move(std::make_unique<std::vector<std::unique_ptr<Param>>>())),
    current_panes(std::move(std::make_unique<std::vector<std::unique_ptr<Pane>>>())) {};

void BaseManager::clear_all() {
  current_sources->clear();
  current_params->clear();
  current_panes->clear();
}

Pane& BaseManager::get_pane(size_t n) const {
  return *current_panes->at(n);
}

size_t BaseManager::n_panes() const {
  return current_panes->size();
}

Pane& BaseManager::add_pane(Param& main, Param& x, Param& y) const {
  std::unique_ptr<Pane> pane = std::make_unique<Pane>(main, x, y);
  current_panes->push_back(std::move(pane));
  return *current_panes->back();
}

void BaseManager::swap_panes(size_t i, size_t j) const {
  std::unique_ptr<Pane> tmp = std::move(current_panes->at(i));
  current_panes->at(i) = std::move(current_panes->at(j));
  current_panes->at(j) = std::move(tmp);
}

// a is the one that jumps furthest
void BaseManager::rotate_panes(const size_t a, const size_t b) const {
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
//   1 - freq/blk/off
//   2 - freq/shp/asym
AbsPolyOsc& BaseManager::add_abs_poly_osc(float frq, size_t shp, size_t asym, size_t off) {
  auto& o = add_source<AbsPolyOsc>(frq, shp, asym, off);
  AbsFreqParam& f = o.get_freq_param();
  add_pane(f, add_param<Blank>(), o.get_off_param());
  add_pane(f, o.get_shp_param(), o.get_asym_param());
  return o;
}

// panes:
//   1 - freq/blk/off
//   2 - freq/shp/asym
RelPolyOsc& BaseManager::add_rel_poly_osc(AbsFreqParam& frq, size_t shp, size_t asym, size_t off) {
  auto& o = add_source<RelPolyOsc>(shp, asym, off, frq, 10.0f, 1.0f);
  RelFreqParam& f = o.get_freq_param();
  add_pane(f, add_param<Blank>(), o.get_off_param());
  add_pane(f, o.get_shp_param(), o.get_asym_param());
  return o;
}

// panes:
//   1 - freq/gain/off
//   2 - freq/shp/asym
std::tuple<Gain&, AbsPolyOsc&>
BaseManager::add_abs_poly_osc_w_gain(const float frq, size_t shp, size_t asym, const size_t off, float amp) {
  size_t n = n_panes();
  AbsPolyOsc& o = add_abs_poly_osc(frq, shp, asym, off);
  Gain& g = add_source<Gain>(o, amp, false);  // abs poly gain is for volume
  static_cast<Blank&>(get_pane(n).x).unblank(&g.get_amp());
  return {g, o};
}

// panes:
//   1 - freq/gain/off
//   2 - freq/shp/asym
std::tuple<Gain&, RelPolyOsc&>
BaseManager::add_rel_poly_osc_w_gain(AbsFreqParam& frq, size_t shp, size_t asym, const size_t off, float amp) {
  size_t n = n_panes();
  RelPolyOsc& o = add_rel_poly_osc(frq, shp, asym, off);
  Gain& g = add_source<Gain>(o, amp, true);  // rel poly gain is for phase
  static_cast<Blank&>(get_pane(n).x).unblank(&g.get_amp());
  return {g, o};
}

Merge& BaseManager::add_balance(RelSource& a, RelSource& b, float bal) {
  auto& m = add_source<Merge>(a, bal);
  m.add_source(b, 1);
  return m;
}

// TODO _ pane
RelSource& BaseManager::add_fm(RelSource& c, RelSource& m, float bal) {
  FM& fm = add_source<FM>(c, m);
  Merge& b = add_balance(fm, c, bal);
  return b;
}

// panes:
//   1 - gain/wet/blk
RelSource& BaseManager::add_fm(RelSource& c, RelSource& m, float bal, float amp) {
  return add_fm(c, m, bal, amp, add_param<Blank>());
}

// panes:
//   1 - gain/wet/arg
RelSource& BaseManager::add_fm(RelSource& c, RelSource& m, float bal, float amp, Param& right) {
  Gain& g = add_source<Gain>(m, amp, 100);  // todo - hi
  FM& fm = add_source<FM>(c, g);
  Merge& b = add_balance(fm, c, bal);
  add_pane(g.get_amp(), b.get_weight(0), right);
  return b;
}

