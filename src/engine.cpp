
#include <iostream>
#include <memory>
#include <stdexcept>

#include "cosas/modulators.h"
#include "cosas/engine.h"


const int DEFAULT_BOXCAR = 1000;


Manager::Manager() : Manager(false) {};

Manager::Manager(const bool t)
  : wavelib(std::move(std::make_unique<Wavelib>())),
    current_nodes(std::move(std::make_unique<std::vector<std::unique_ptr<Node>>>())),
    current_params(std::move(std::make_unique<std::vector<std::unique_ptr<Param>>>())),
    current_inputs(std::move(std::make_unique<std::vector<std::unique_ptr<Input>>>())),
    current_panes(std::move(std::make_unique<std::vector<std::unique_ptr<Pane>>>())),
    test(t) {};

const Node& Manager::build(Manager::Engine engine) {
  
  current_nodes->clear();
  current_params->clear();
  current_inputs->clear();
  current_panes->clear();

  switch(engine) {
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


template<typename NodeType, typename... Args> NodeType& Manager::add_node(Args&&... args) {
  std::unique_ptr<NodeType> node = std::make_unique<NodeType>(std::forward<Args>(args)...);
  current_nodes->push_back(std::move(node));
  return dynamic_cast<NodeType&>(*current_nodes->back());
}

template<typename ParamType, typename... Args> ParamType& Manager::add_param(Args&&... args) {
  std::unique_ptr<ParamType> param = std::make_unique<ParamType>(std::forward<Args>(args)...);
  current_params->push_back(std::move(param));
  return dynamic_cast<ParamType&>(*current_params->back());
}

template<typename InputType, typename... Args> InputType& Manager::add_input(Args&&... args) {
  std::unique_ptr<InputType> input = std::make_unique<InputType>(std::forward<Args>(args)...);
  current_inputs->push_back(std::move(input));
  return dynamic_cast<InputType&>(*current_inputs->back());
}

Pane& Manager::add_pane(Input& top, Input& left, Input& right) {
  std::unique_ptr<Pane> pane = std::make_unique<Pane>(top, left, right);
  current_panes->push_back(std::move(pane));
  return *current_panes->back();
}

void Manager::swap_panes(size_t i, size_t j) {
  std::unique_ptr<Pane> tmp = std::move(current_panes->at(i));
  current_panes->at(i) = std::move(current_panes->at(j));
  current_panes->at(j) = std::move(tmp);  
}

// a is the one that jumps furthest
void Manager::rotate_panes(size_t a, size_t b) {
  if (a < b) {
    std::unique_ptr<Pane> tmp = std::move(current_panes->at(a));
    for (size_t i = a; i < b; i++) current_panes->at(i) = std::move(current_panes->at(i+1));
    current_panes->at(b) = std::move(tmp);
  } else {
    std::unique_ptr<Pane> tmp = std::move(current_panes->at(a));
    for (size_t i = a; i > b; i--) current_panes->at(i) = std::move(current_panes->at(i-1));
    current_panes->at(b) = std::move(tmp);
  }
}

Input& Manager::lin_control(Input& in, float c, float lo, float hi) {
  return add_input<Change>(
	   add_input<Sigmoid>(
			      add_input<Additive>(in, test ? lo : c, lo, hi),
	                      test ? 1 : 0.5));
}

Input& Manager::log_control(Input& in, float c, float lo, float hi) {
  return add_input<Change>(
	   add_input<Sigmoid>(
			      add_input<Multiplicative>(in, test ? lo : c, lo, hi),
	                      test ? 1 : 0.5));
}

// panes:
//   1 - freq/dex/arg
std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_dex_osc(float frq, size_t widx, Input& right) {
  AbsDexOsc& o = add_node<AbsDexOsc>(*wavelib, widx, frq);
  AbsoluteFreq& f = o.get_freq();
  WavedexMixin::Wavedex& w = o.get_dex();
  Input& top = log_control(f, frq, 1.0 / (1 << subtick_bits), 0.5 * sample_rate);
  Input& left = lin_control(w, widx, 0, wavelib->size() - 1);
  add_pane(top, left, right);
  return {f, o};
}

// panes:
//   1 - freq/dex/blk
std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_dex_osc(float frq, size_t widx) {
  return add_abs_dex_osc(frq, widx, add_input<Blank>());
}

// panes:
//   1 - freq/dex/gain
std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_dex_osc_w_gain(float frq, size_t widx, float amp) {
  Blank& b = add_input<Blank>();
  auto [f, o] = add_abs_dex_osc(frq, widx, b);
  Gain& g = add_node<Gain>(o, amp);
  b.unblank(&lin_control(g.get_amp(), amp, 0, 1));
  return {f, g};
}

// panes:
//   1 - freq/dex/det
Node& Manager::add_rel_dex_osc(AbsoluteFreq& root, size_t widx, float r, float d) {
  RelDexOsc& o = add_node<RelDexOsc>(*wavelib, widx, root, r, d);
  RelativeFreq& f = o.get_freq();
  Input& top = log_control(f, 1, 1.0 / (root.get_frequency() << subtick_bits), 0.5 * sample_rate / root.get_frequency());
  Input& left = lin_control(o.get_dex(), widx, 0, wavelib->size());
  Input& right = log_control(f.get_det(), 1, 0.9, 1.1);
  add_pane(top, left, right);
  return o;
}

// panes:
//   1 - off/shp/asym
//   (freq not mapped)
std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_poly_osc(float frq, size_t shp, size_t asym, size_t off) {
  AbsPolyOsc& o = add_node<AbsPolyOsc>(frq, shp, asym, off);
  AbsoluteFreq& f = o.get_freq();
  Input& top = lin_control(o.get_off(), quarter_table_size, 0, half_table_size);
  Input& left = lin_control(o.get_shp(), PolyTable::sine, 0, PolyTable::n_shapes);
  int n2 = PolyTable::n_shapes >> 1;
  Input& right = lin_control(o.get_asym(), 0, -n2, static_cast<int>(PolyTable::n_shapes) - n2); 
  add_pane(top, left, right);
  return {f, o};
}

// panes:
//   1 - freq/blk/gain
//   2 - off/shp/asym
std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_poly_osc_w_gain(float frq, size_t shp, size_t asym, size_t off, float amp) {
  Blank& top = add_input<Blank>();
  Blank& left = add_input<Blank>();
  Blank& right = add_input<Blank>();
  add_pane(top, left, right);
  auto [f, o] = add_abs_poly_osc(frq, shp, asym, off);
  top.unblank(&log_control(f, frq, 1.0 / (1 << subtick_bits), 0.5 * sample_rate));
  Gain& g = add_node<Gain>(o, amp);
  right.unblank(&lin_control(g.get_amp(), amp, 0, 1));
  return {f, g};  // TODO - currently unsued.  why are we returning these?
}

Merge& Manager::add_balance(Node& a, Node& b, float bal) {
  Merge& m = add_node<Merge>(a, bal);
  m.add_node(b, 1);
  return m;
}

// panes:
//   1 - gain/wet/blk
Node& Manager::add_fm(Node& c, Node& m, float bal, float amp) {
  return add_fm(c, m, bal, amp, add_input<Blank>());
}

// panes:
//   1 - gain/wet/arg
Node& Manager::add_fm(Node& c, Node& m, float bal, float amp, Input& right) {
  Gain& g = add_node<Gain>(m, amp);
  FM& fm = add_node<FM>(c, g);
  Merge& b = add_balance(fm, c, bal);
  Input & top = lin_control(g.get_amp(), amp, 0, 1);
  Input& left = lin_control(b.get_weight(0), bal, 0, 1);
  add_pane(top, left, right);
  return b;
}

// panes:
//   1 - freq/dex/blk
const Node& Manager::build_dex() {
  auto [f, o] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  return o;
}

// panes:
//   1 - freq/blank/blk
//   2 - off/shp/asym
const Node& Manager::build_poly() {
  auto [f, o] = add_abs_poly_osc(10, PolyTable::linear - 1, 0, 0.1 * quarter_table_size);
  return o;
}

// panes:
//   1 - freq/dex/blk
//   2 - freq/dex/det
//   3 - gain/wet/blk
const Node& Manager::build_fm_simple() {
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  Node& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  // i don't understand this 3.  is it subtick_bits?
  Node& fm = add_fm(c, m, 0.5, 1.0 / (1 << (phi_fudge_bits - 3)));
  return fm;
}

// panes:
//   1 - freq/dex/blk
//   2 - freq/dex/det
//   3 - freq/dex/blk
//   4 - gain/wet/blk
const Node& Manager::build_fm_lfo() {
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1);
  Node& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  auto [lf, l] = add_abs_dex_osc_w_gain(1, wavelib->sine_gamma_1, 1);
  Node& am = add_node<AM>(l, m);
  Node& fm = add_fm(c, am, 0.5, 1.0 / (1 << (phi_fudge_bits - 4)));
  return fm;
}

// panes:
//   1 - freq/dex/freq
//   2 - freq/dex/det
//   3 - gain/wet/blk
//   4 - off/shp/asym
const Node& Manager::build_fm_env() {
  const Node& fm = build_fm_simple();
  auto [ef, e] = add_abs_poly_osc(1, PolyTable::linear - 1, 0, 0.1 * quarter_table_size);
  Node& am = add_node<AM>(e, fm);
  static_cast<Blank&>(get_pane(0).right).unblank(&log_control(ef, 1, 1.0 / (1 << subtick_bits), 0.5 * sample_rate));
  return am;
}

// panes:
//   1 - freq/dex/len
//   2 - freq/dex/det
//   3 - gain/wet/fb
const Node& Manager::build_fm_fb() {
  Latch& latch = add_node<Latch>();
  Boxcar& flt = add_node<Boxcar>(latch, DEFAULT_BOXCAR);
  Input& right = lin_control(flt.get_len(), DEFAULT_BOXCAR, 1, MAX_BOXCAR);
  auto [cf, c] = add_abs_dex_osc(440, wavelib->sine_gamma_1, right);
  Node& m = add_rel_dex_osc(cf, wavelib->sine_gamma_1, 1, 1);
  Merge& mrg = add_balance(flt, m, 0.5);
  Node& fm = add_fm(c, mrg, 0.5, 1.0 / (1 << (phi_fudge_bits - 4)), mrg.get_weight(0));
  latch.set_source(&fm);
  return latch;
}

// panes:
//   1 - freq/dex/wt0
//   2 - wt1/wt2/wt3  (rotated to posn)
//   3 - freq/dex/det
//   4 - freq/dex/det
//   5 - freq/dex/det
const Node& Manager::build_chord() {
  Blank& b = add_input<Blank>();
  auto [f0, o0] = add_abs_dex_osc(440, wavelib->sine_gamma_1, b);
  Node& o1 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 5/4.0, 1);
  Node& o2 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 3/2.0, 1);
  Node& o3 = add_rel_dex_osc(f0, wavelib->sine_gamma_1, 4/3.0, 1);
  Merge& m = add_node<Merge>(o0, 0.5);
  b.unblank(&m.get_weight(0));
  m.add_node(o1, 1);
  m.add_node(o2, 1);
  m.add_node(o3, 1);
  add_pane(m.get_weight(1), m.get_weight(2), m.get_weight(3));
  rotate_panes(4, 1);
  return m;
}

