
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
  case Manager::Engine::FM:
    return build_fm();
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

template<typename ModType, typename... Args> ModType& Manager::add_modulator(const Node& nd1, const Node& nd2, Args... args) {
  std::unique_ptr<ModType> mod = std::make_unique<ModType>(nd1, nd2, std::forward<Args>(args)...);
  current_nodes->push_back(std::move(mod));
  return dynamic_cast<ModType&>(*current_nodes->back());
}

template<typename TranType, typename... Args> TranType& Manager::add_transformer(const Node& nd1, Args... args) {
  std::unique_ptr<TranType> tran = std::make_unique<TranType>(nd1, std::forward<Args>(args)...);
  current_nodes->push_back(std::move(tran));
  return dynamic_cast<TranType&>(*current_nodes->back());
}

template<typename NodeType, typename... Args> NodeType& Manager::add_node(Args... args) {
  std::unique_ptr<NodeType> node = std::make_unique<NodeType>(std::forward<Args>(args)...);
  current_nodes->push_back(std::move(node));
  return dynamic_cast<NodeType&>(*current_nodes->back());
}

Oscillator& Manager::add_oscillator(Wavedex& w, Frequency& f) {
  current_nodes->push_back(std::move(std::make_unique<Oscillator>(w, f)));
  return dynamic_cast<Oscillator&>(*current_nodes->back());
}

template<typename ParamType, typename... Args> ParamType& Manager::add_param(Args... args) {
  std::unique_ptr<ParamType> param = std::make_unique<ParamType>(std::forward<Args>(args)...);
  current_params->push_back(std::move(param));
  return dynamic_cast<ParamType&>(*current_params->back());
}

Wavedex& Manager::add_wavedex(size_t widx) {
  current_params->push_back(std::move(std::make_unique<Wavedex>(*wavelib, widx)));
  return dynamic_cast<Wavedex&>(*current_params->back());
}

template<typename InputType, typename... Args> InputType& Manager::add_input(Input& del, Args... args) {
  std::unique_ptr<InputType> input = std::make_unique<InputType>(del, std::forward<Args>(args)...);
  current_inputs->push_back(std::move(input));
  return dynamic_cast<InputType&>(*current_inputs->back());
}

Blank& Manager::add_blank() {
  current_inputs->push_back(std::move(std::make_unique<Blank>()));
  return dynamic_cast<Blank&>(*current_inputs->back());
}

Pane& Manager::add_pane(Input& top, Input& left, Input& right) {
  std::unique_ptr<Pane> pane = std::make_unique<Pane>(top, left, right);
  current_panes->push_back(std::move(pane));
  return *current_panes->back();
}

std::tuple<Wavedex&, AbsoluteFreq&, Oscillator&> Manager::build_abs_osc(size_t widx, float frq) {
  Wavedex& w = add_wavedex(widx);
  AbsoluteFreq& f = add_param<AbsoluteFreq>(frq);
  Oscillator& o = add_oscillator(w, f);
  Input& top = add_input<Change>(add_input<Sigmoid>(add_input<Multiplicative>(f, frq, 1.0 / (1 << subtick_bits), 0.5 * sample_rate), 0.5));
  Input& left = add_input<Change>(add_input<Sigmoid>(add_input<Additive>(w, wavelib->sine_gamma_1, 0, wavelib->size()), 0.5));
  add_pane(top, left, add_blank());
  return {w, f, o};
}

std::tuple<Wavedex&, RelativeFreq&, Oscillator&> Manager::build_rel_osc(size_t widx, AbsoluteFreq& root, float r, float d) {
  Wavedex& w = add_wavedex(widx);
  // has ref first arg so cannot use add_param
  current_params->push_back(std::move(std::make_unique<RelativeFreq>(root, r, d)));
  RelativeFreq& f = static_cast<RelativeFreq&>(*current_params->back());
  Oscillator& o = add_oscillator(w, f);
  return {w, f, o};
}

std::tuple<Amplitude&, Balance&, ModularFM&> Manager::build_fm(Node& c, Node& m, float amp) {
  Amplitude& a = add_param<Amplitude>(amp);
  Balance& b = add_param<Balance>();
  ModularFM& fm = add_modulator<ModularFM>(c, m, a, b);
  return {a, b, fm};
}
  
const Node& Manager::build_fm() {
  // i don't understand this 3.  is it subtick_bits?
  return build_fm(1.0 / (1 << (phi_fudge_bits - 3)));
}

const Node& Manager::build_fm(float amp) {
  auto [cw, cf, c] = build_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = build_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  auto [a, b, fm] = build_fm(c, m, amp);
  return fm;
}

TEST_CASE("BuildFM") {
  Manager m = Manager();
  int16_t amp0 = m.build_fm(0).next(50, 0);
  int16_t amp01 = m.build_fm(0.001).next(50, 0);
  CHECK(amp0 == amp01);
  amp0 = m.build_fm(0).next(51, 0);
  amp01 = m.build_fm(0.001).next(51, 0);
  CHECK(amp0 == amp01 - 18);  // exact diff not important, just should not be huge
}

const Node& Manager::build_fm_lfo() {
  return build_fm_lfo(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_lfo(float amp) {
  auto [cw, cf, c] = build_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = build_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  auto [lw, lf, l] = build_abs_osc(wavelib->sine_gamma_1, 1);
  // TODO - need gain for lfo?
  Node& am = add_modulator<AM>(l, m);
  auto [a, b, fm] = build_fm(c, am, amp);
  return fm;
}

const Node& Manager::build_fm_fb() {
  return build_fm_fb(1.0 / (1 << (phi_fudge_bits - 4)));
}

const Node& Manager::build_fm_fb(float amp) {
  auto [cw, cf, c] = build_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = build_rel_osc(wavelib->sine_gamma_1, cf, 1, 1);
  Latch& latch = add_node<Latch>();
  MeanFilter::Length l = add_param<MeanFilter::Length>(1);
  MeanFilter& flt = add_transformer<MeanFilter>(latch, l);
  Balance& mb = add_param<Balance>(0.5);
  Merge mrg = add_modulator<Merge>(flt, m, mb);
  auto [a, b, fm] = build_fm(c, mrg, amp);
  latch.set_source(&fm);
  return latch;
}

// this is via feedback which might be crazy
const Node& Manager::build_fm_fmnt() {
  auto [cw, cf, c] = build_abs_osc(wavelib->sine_gamma_1, 440);
  auto [mw, mf, m] = build_abs_osc(wavelib->sine_gamma_1, 100);
  Latch& latch = add_node<Latch>();
  MeanFilter::Length& l = add_param<MeanFilter::Length>(1);
  MeanFilter& flt = add_transformer<MeanFilter>(latch, l);
  PriorityMerge::Weight& w0 = add_param<PriorityMerge::Weight>(0.5);
  PriorityMerge& mrg = add_transformer<PriorityMerge>(flt, w0);
  auto [mw1, mf1, m1] = build_rel_osc(wavelib->sine_gamma_1, mf, 2, 1);
  PriorityMerge::Weight& w1 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(m1, w1);
  auto [mw2, mf2, m2] = build_rel_osc(wavelib->sine_gamma_1, mf, 3, 1);
  PriorityMerge::Weight& w2 = add_param<PriorityMerge::Weight>(0.1);
  mrg.add_node(m2, w2);
  // could add more in parallel here?
  auto [a, b, fm] = build_fm(c, mrg, 1);
  latch.set_source(&fm);
  return latch;
}

