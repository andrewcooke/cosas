
#include <iostream>
#include <memory>
#include <stdexcept>

#include "modulators.h"
#include "engine.h"


Manager::Manager()
  : wavelib(std::move(std::make_unique<Wavelib>())),
    current_nodes(std::move(std::make_unique<std::vector<std::unique_ptr<Node>>>())),
    current_params(std::move(std::make_unique<std::vector<std::unique_ptr<Param>>>())),
    current_inputs(std::move(std::make_unique<std::vector<std::unique_ptr<Input>>>())),
    current_panes(std::move(std::make_unique<std::vector<std::unique_ptr<Pane>>>())) {};

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
  case Manager::Engine::FM_FMNT:
    return build_fm_fmnt();
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

template<typename InputType, typename... Args> InputType& Manager::add_input(Input& del, Args... args) {
  std::unique_ptr<InputType> input = std::make_unique<InputType>(del, std::forward<Args>(args)...);
  current_inputs->push_back(std::move(input));
  return dynamic_cast<InputType&>(*current_inputs->back());
}

Pane& Manager::add_pane(Input& top, Input& left, Input& right) {
  std::unique_ptr<Pane> pane = std::make_unique<Pane>(top, left, right);
  current_panes->push_back(std::move(pane));
  return *current_panes->back();
}

Blank& Manager::blank() {
  current_inputs->push_back(std::move(std::make_unique<Blank>()));
  return dynamic_cast<Blank&>(*current_inputs->back());
}

Input& Manager::lin_control(Input& in, float c, float lo, float hi) {
  return add_input<Change>(
	   add_input<Sigmoid>(
			      add_input<Additive>(in, c, lo, hi),
	                      0.5));
}

Input& Manager::log_control(Input& in, float c, float lo, float hi) {
  return add_input<Change>(
	   add_input<Sigmoid>(
			      add_input<Multiplicative>(in, c, lo, hi),
	                      0.5));
}

std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_osc(size_t widx, float frq) {
  Wavedex& w = add_param<Wavedex>(*wavelib, widx);
  AbsoluteFreq& f = add_param<AbsoluteFreq>(frq);
  Oscillator& o = add_node<Oscillator>(w, f);
  Input & top = log_control(f, frq, 1.0 / (1 << subtick_bits), 0.5 * sample_rate);
  Input& left = log_control(w, wavelib->sine_gamma_1, 0, wavelib->size());
  add_pane(top, left, blank());
  return {f, o};
}

// this uses all the knobs!
std::tuple<AbsoluteFreq&, Node&> Manager::add_abs_osc_w_gain(size_t widx, float frq, float amp) {
  auto [f, o] = add_abs_osc(widx, frq);
  Amplitude& a = add_param<Amplitude>(amp);
  Gain& g = add_node<Gain>(o, a);
  Pane& p = *current_panes->back();
  p.right_knob = lin_control(a, amp, 0, 1);
  return {f, g};
}

Node& Manager::add_rel_osc(size_t widx, AbsoluteFreq& root, float r, float d) {
  Wavedex& w = add_param<Wavedex>(*wavelib, widx);
  // has ref first arg so cannot use add_param
  current_params->push_back(std::move(std::make_unique<RelativeFreq>(root, r, d)));
  RelativeFreq& f = static_cast<RelativeFreq&>(*current_params->back());
  Oscillator& o = add_node<Oscillator>(w, f);
  Input& top = log_control(f, 1, 1.0 / (root.get_frequency() << subtick_bits), 0.5 * sample_rate / root.get_frequency());
  Input& left = log_control(w, wavelib->sine_gamma_1, 0, wavelib->size());
  current_inputs->push_back(std::move(f.get_detune()));  // TODO - ok?
  Input& right = log_control(*current_inputs->back(), 1, 0.9, 1.1);
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
  add_pane(top, left, blank());
  return j;
}
  
const Node& Manager::build_fm_simple() {
  // i don't understand this 3.  is it subtick_bits?
  return build_fm_simple(1.0 / (1 << (phi_fudge_bits - 3)));
}

const Node& Manager::build_fm_simple(float amp) {
  // TODO - are these return tuple vals used?
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  Node& fm = add_fm(c, m, 0.5, amp);
  return fm;
}

TEST_CASE("BuildFMSimple") {
  Manager m = Manager();
  int16_t amp0 = m.build_fm_simple(0).next(50, 0);
  CHECK(m.n_panes() == 3);  // carrier, modulator, fm gain/balance
  int16_t amp01 = m.build_fm_simple(0.001).next(50, 0);
  CHECK(amp0 == amp01);
  amp0 = m.build_fm_simple(0).next(51, 0);
  amp01 = m.build_fm_simple(0.001).next(51, 0);
  CHECK(abs(amp0 - amp01) < 10);  // exact diff not important, just should not be huge
}

const Node& Manager::build_fm_lfo() {
  return build_fm_lfo(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_lfo(float amp) {
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  auto [lf, l] = add_abs_osc_w_gain(wavelib->sine_gamma_1, 1, 1);
  Node& am = add_node<AM>(l, m);
  Node& fm = add_fm(c, am, 0.5, amp);
  return fm;
}

TEST_CASE("BuildFMLFO") {
  Manager m = Manager();
  int32_t amp = m.build(m.FM_LFO).next(123, 0);
  CHECK(amp == 32450);  // exact value not important
  CHECK(m.n_panes() == 4);  // carrier, modulator, lfo, fm gain/balance
}

const Node& Manager::build_fm_fb() {
  return build_fm_fb(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_fb(float amp) {
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  Node& m = add_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  Latch& latch = add_node<Latch>();
  MeanFilter::Length l = add_param<MeanFilter::Length>(1);
  MeanFilter& flt = add_node<MeanFilter>(latch, l);
  Balance& mb = add_param<Balance>(0.5);
  Merge& mrg = add_node<Merge>(flt, m, mb);
  Node& fm = add_fm(c, mrg, 0.5, amp);
  latch.set_source(&fm);
  return latch;
}

// this is via feedback which might be crazy
const Node& Manager::build_fm_fmnt() {
  auto [cf, c] = add_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mf, m] = add_abs_osc(wavelib->sine_gamma_1, 100);
  Latch& latch = add_node<Latch>();
  MeanFilter::Length& l = add_param<MeanFilter::Length>(1);
  MeanFilter& flt = add_node<MeanFilter>(latch, l);
  PriorityMerge::Weight& w0 = add_param<PriorityMerge::Weight>(0.5);
  PriorityMerge& mrg = add_node<PriorityMerge>(flt, w0);
  Node& m1 = add_rel_osc(wavelib->sine_gamma_1, mf, 2, 1);
  PriorityMerge::Weight& w1 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(m1, w1);
  Node& m2 = add_rel_osc(wavelib->sine_gamma_1, mf, 3, 1);
  PriorityMerge::Weight& w2 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(m2, w2);
  // could add more in parallel here?
  Node& fm = add_fm(c, mrg, 0.5, 1);
  latch.set_source(&fm);
  return latch;
}

