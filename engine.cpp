
#include <iostream>
#include <memory>
#include <stdexcept>

#include "modulators.h"
#include "engine.h"


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
  AbsoluteOsc& o = add_node<AbsoluteOsc>(*wavelib, widx, frq);
  AbsoluteFreq& f = o.get_param();
  Oscillator::Wavedex& w = o.get_wavedex();
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
  Amplitude& a = add_param<Amplitude>(amp);
  Input& right = lin_control(a, amp, 0, 1);
  auto [f, o] = add_abs_osc(widx, frq, right);
  Gain& g = add_node<Gain>(o, a);
  return {f, g};
}

Node& Manager::add_rel_osc(size_t widx, AbsoluteFreq& root, float r, float d) {
  RelativeOsc& o = add_node<RelativeOsc>(*wavelib, widx, root, r, d);
  RelativeFreq& f = o.get_param();
  Oscillator::Wavedex& w = o.get_wavedex();
  Input& top = log_control(f, 1, 1.0 / (root.get_frequency() << subtick_bits), 0.5 * sample_rate / root.get_frequency());
  Input& left = lin_control(w, widx, 0, wavelib->size());
  Input& right = log_control(f.get_detune(), 1, 0.9, 1.1);
  add_pane(top, left, right);
  std::cerr << "osc " << &o << " = " << o.next(0, 0) << std::endl;
  return o;
}

Node& Manager::add_fm(Node& c, Node& m, float bal, float amp) {
  Amplitude& a = add_param<Amplitude>(amp);
  std::cerr << "created " << &a << std::endl;
  Gain& g = add_node<Gain>(m, a);
  FM& fm = add_node<FM>(c, g);
  Balance& b = add_param<Balance>(bal);
  Merge& j = add_node<Merge>(fm, c, b);
  Input & top = lin_control(a, amp, 0, 1);
  Input& left = lin_control(b, bal, 0, 1);
  add_pane(top, left, add_input<Blank>());
  return j;
}
  
const Node& Manager::build_fm_simple() {
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  // i don't understand this 3.  is it subtick_bits?
  //  Node& fm = add_fm(c, m, 0.5, 1.0 / (1 << (phi_fudge_bits - 3)));
  Node& fm = add_fm(c, m, 0, 0);
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
  CHECK(amp == 32450);  // exact value not important
  CHECK(m.n_panes() == 4);  // carrier, modulator, lfo, fm gain/balance
}

const Node& Manager::build_fm_fb() {
  const int DEFAULT_LENGTH = 5;
  MeanFilter::Length l = add_param<MeanFilter::Length>(DEFAULT_LENGTH);
  Input& right = lin_control(l, DEFAULT_LENGTH, 0, 10);
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440, right);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  Latch& latch = add_node<Latch>();
  MeanFilter& flt = add_node<MeanFilter>(latch, l);
  Balance& mb = add_param<Balance>(0.5);
  Merge& mrg = add_node<Merge>(flt, m, mb);
  Node& fm = add_fm(c, mrg, 0.5, 1.0 / (1 << (phi_fudge_bits - 4)));
  latch.set_source(&fm);
  return latch;
}

TEST_CASE("BuildFM_FB") {
  Manager m = Manager();
  int32_t amp = m.build(m.FM_FB).next(666, 0);
  CHECK(amp == -21003);  // exact value not important
  CHECK(m.n_panes() == 3);  // carrier/filter, modulator, fm gain/balance
}

const Node& Manager::build_chord() {
  PriorityMerge::Weight& w0 = add_param<PriorityMerge::Weight>(0.5);
  auto [f0, o0] = add_abs_osc(wavelib->sine_gamma_1, 440, w0);
  Node& o1 = add_rel_osc(wavelib->sine_gamma_1, f0, 5/4.0, 1);
  Node& o2 = add_rel_osc(wavelib->sine_gamma_1, f0, 3/2.0, 1);
  Node& o3 = add_rel_osc(wavelib->sine_gamma_1, f0, 4/3.0, 1);
  PriorityMerge& mrg = add_node<PriorityMerge>(o0, w0);
  PriorityMerge::Weight& w1 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(o1, w1);
  PriorityMerge::Weight& w2 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(o2, w2);
  PriorityMerge::Weight& w3 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(o3, w3);
  add_pane(w1, w2, w3);
  return mrg;
}

