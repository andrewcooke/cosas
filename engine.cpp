
#include <iostream>
#include <memory>
#include <stdexcept>

#include "modulators.h"
#include "engine.h"


const int DEFAULT_BOXCAR = 1000;


Manager::Manager() : Manager(false) {};

Manager::Manager(bool t)
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
  case Manager::Engine::FM_SIMPLE:
    return build_fm_simple();
  case Manager::Engine::FM_LFO:
    return build_fm_lfo();
  case Manager::Engine::FM_FB:
    return build_fm_fb();
  case Manager::Engine::CHORD:
    return build_chord();
  default:
    throw std::domain_error("missing case in Manager::build?");
  }
}

const Pane& Manager::get_pane(size_t n) {
  return *current_panes->at(n);
}

size_t Manager::n_panes() {
  return current_panes->size();
}

size_t Manager::n_wforms() {
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

std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_osc(size_t widx, float frq, Input& right) {
  AbsDexOsc& o = add_node<AbsDexOsc>(*wavelib, widx, frq);
  AbsoluteFreq& f = o.get_param();
  WavedexMixin::Wavedex& w = o.get_wavedex();
  Input& top = log_control(f, frq, 1.0 / (1 << subtick_bits), 0.5 * sample_rate);
  Input& left = lin_control(w, widx, 0, wavelib->size() - 1);
  std::cerr << "sq " <<  wavelib->square_duty_05 << " sin " << wavelib->sine_gamma_1 << std::endl;
  add_pane(top, left, right);
  return {f, o};
}

std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_osc(size_t widx, float frq) {
  return add_abs_osc(widx, frq, add_input<Blank>());
}

std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_osc_w_gain(size_t widx, float frq, float amp) {
  Blank& b = add_input<Blank>();
  auto [f, o] = add_abs_osc(widx, frq, b);
  Gain& g = add_node<Gain>(o, amp);
  b.unblank(&lin_control(g.get_param(), amp, 0, 1));
  return {f, g};
}

Node& Manager::add_rel_osc(size_t widx, AbsoluteFreq& root, float r, float d) {
  RelDexOsc& o = add_node<RelDexOsc>(*wavelib, widx, root, r, d);
  RelativeFreq& f = o.get_param();
  WavedexMixin::Wavedex& w = o.get_wavedex();
  Input& top = log_control(f, 1, 1.0 / (root.get_frequency() << subtick_bits), 0.5 * sample_rate / root.get_frequency());
  Input& left = lin_control(w, widx, 0, wavelib->size());
  Input& right = log_control(f.get_detune(), 1, 0.9, 1.1);
  add_pane(top, left, right);
  std::cerr << "osc " << &o << " = " << o.next(0, 0) << std::endl;
  return o;
}

Merge& Manager::add_balance(Node& a, Node& b, float bal) {
  Merge& m = add_node<Merge>(a, bal);
  m.add_node(b, 1);
  return m;
}

Node& Manager::add_fm(Node& c, Node& m, float bal, float amp) {
  return add_fm(c, m, bal, amp, add_input<Blank>());
}

Node& Manager::add_fm(Node& c, Node& m, float bal, float amp, Input& right) {
  Gain& g = add_node<Gain>(m, amp);
  FM& fm = add_node<FM>(c, g);
  Merge& b = add_balance(fm, c, bal);
  Input & top = lin_control(g.get_param(), amp, 0, 1);
  Input& left = lin_control(b.get_param(0), bal, 0, 1);
  add_pane(top, left, right);
  return b;
}
  
const Node& Manager::build_fm_simple() {
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  // i don't understand this 3.  is it subtick_bits?
  Node& fm = add_fm(c, m, 0.5, 1.0 / (1 << (phi_fudge_bits - 3)));
  return fm;
}

TEST_CASE("BuildFM_SIMPLE") {
  Manager m = Manager();
  CHECK(m.build(m.FM_SIMPLE).next(50, 0) == 172);
  CHECK(m.n_panes() == 3);  // carrier, modulator, fm gain/balance
}

const Node& Manager::build_fm_lfo() {
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  auto [lf, l] = add_abs_osc_w_gain(wavelib->sine_gamma_1, 1, 1);
  Node& am = add_node<AM>(l, m);
  Node& fm = add_fm(c, am, 0.5, 1.0 / (1 << (phi_fudge_bits - 4)));
  return fm;
}

TEST_CASE("BuildFM_LFO") {
  Manager m = Manager();
  int32_t amp = m.build(m.FM_LFO).next(123, 0);
  CHECK(amp == 32449);  // exact value not important
  CHECK(m.n_panes() == 4);  // carrier, modulator, lfo, fm gain/balance
}

const Node& Manager::build_fm_fb() {
  Latch& latch = add_node<Latch>();
  Boxcar& flt = add_node<Boxcar>(latch, DEFAULT_BOXCAR);
  Input& right = lin_control(flt.get_param(), DEFAULT_BOXCAR, 1, MAX_BOXCAR);
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440, right);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  Merge& mrg = add_balance(flt, m, 0.5);
  Node& fm = add_fm(c, mrg, 0.5, 1.0 / (1 << (phi_fudge_bits - 4)), mrg.get_param(0));
  latch.set_source(&fm);
  return latch;
}

TEST_CASE("BuildFM_FB") {
  Manager m = Manager();
  int32_t amp = m.build(m.FM_FB).next(666, 0);
  CHECK(amp == -21003);  // exact value not important
  CHECK(m.n_panes() == 3);  // carrier/filter, modulator, fm gain/balance/flt balance
}

const Node& Manager::build_chord() {
  Blank& b = add_input<Blank>();
  auto [f0, o0] = add_abs_osc(wavelib->sine_gamma_1, 440, b);
  Node& o1 = add_rel_osc(wavelib->sine_gamma_1, f0, 5/4.0, 1);
  Node& o2 = add_rel_osc(wavelib->sine_gamma_1, f0, 3/2.0, 1);
  Node& o3 = add_rel_osc(wavelib->sine_gamma_1, f0, 4/3.0, 1);
  Merge& m = add_node<Merge>(o0, 0.5);
  b.unblank(&m.get_param(0));
  m.add_node(o1, 1);
  m.add_node(o2, 1);
  m.add_node(o3, 1);
  add_pane(m.get_param(1), m.get_param(2), m.get_param(3));
  return m;
}

