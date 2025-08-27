
#include <memory>

#include "cosas/engine_base.h"
#include "cosas/modulators.h"


BaseManager::BaseManager()
  : current_sources(std::move(std::make_unique<std::vector<std::unique_ptr<RelSource>>>())),
    current_params(std::move(std::make_unique<std::vector<std::unique_ptr<Param>>>())),
    current_panes(std::move(std::make_unique<std::vector<std::unique_ptr<Pane>>>())) {};

Pane& BaseManager::get_pane(size_t n) const {
  return *current_panes->at(n);
}

size_t BaseManager::n_panes() const {
  return current_panes->size();
}

Pane& BaseManager::add_pane(Param& top, Param& left, Param& right) const {
  std::unique_ptr<Pane> pane = std::make_unique<Pane>(top, left, right);
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
//   1 - off/shp/asym
//   (freq not mapped)
std::tuple<AbsFreqParam&, RelSource&>
BaseManager::add_abs_poly_osc(float frq, size_t shp, size_t asym, size_t off) {
  auto& o = add_source<AbsPolyOsc>(frq, shp, asym, off);
  AbsFreqParam& f = o.get_freq_param();
  add_pane(o.get_off_param(), o.get_shp_param(), o.get_asym_param());
  return {f, o};
}

// panes:
//   1 - freq/blk/gain
//   2 - off/shp/asym
std::tuple<AbsFreqParam&, RelSource&>
BaseManager::add_abs_poly_osc_w_gain(const float frq, size_t shp, size_t asym,
                                     const size_t off, float amp) {
  auto& top = add_param<Blank>();
  auto& left = add_param<Blank>();
  auto& right = add_param<Blank>();
  add_pane(top, left, right);
  auto [f, o] = add_abs_poly_osc(frq, shp, asym, off);
  top.unblank(&f);
  Gain& g = add_source<Gain>(o, amp, 100);  // todo - hi?
  right.unblank(&g.get_amp());
  return {f, g}; // TODO - currently unused.  why are we returning these?
}

Merge& BaseManager::add_balance(RelSource& a, RelSource& b, float bal) {
  auto& m = add_source<Merge>(a, bal);
  m.add_source(b, 1);
  return m;
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

