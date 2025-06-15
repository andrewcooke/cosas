
#ifndef COSA_ENGINE_H
#define COSA_ENGINE_H

#include <tuple>
#include <vector>
#include <memory>

#include "modulators.h"
#include "wavelib.h"
#include "node.h"
#include "oscillator.h"
#include "params.h"
#include "pane.h"


// forward decl
class Oscillator;


class Manager {

public:

  enum Engine {
    DEX,
    POLY,
    FM_SIMPLE,
    FM_LFO,
    FM_ENV,
    FM_FB,
    CHORD,
  };

  Manager();
  Manager(bool t);
  const Node& build(Engine);
  const Pane& get_pane(size_t n);
  size_t n_panes();
  size_t n_wforms();

private:

  template<typename NodeType, typename... Args> NodeType& add_node(Args&&... args);
  template<typename ParamType, typename... Args> ParamType& add_param(Args&&... args);
  template<typename InputType, typename... Args> InputType& add_input(Args&&... args);
  Pane& add_pane(Input& top, Input& left, Input& right);
  Input& lin_control(Input& in, float c, float lo, float hi);
  Input& log_control(Input& in, float c, float lo, float hi);
  std::tuple<AbsoluteFreq&, Node&> add_abs_dex_osc(float frq, size_t widx, Input& right);
  std::tuple<AbsoluteFreq&, Node&> add_abs_dex_osc(float frq, size_t widx);
  std::tuple<AbsoluteFreq&, Node&> add_abs_dex_osc_w_gain(float frq, size_t widx, float amp);
  std::tuple<AbsoluteFreq&, Node&> add_abs_poly_osc(float frq, size_t shp, size_t asyn, size_t off, Input& right);
  std::tuple<AbsoluteFreq&, Node&> add_abs_poly_osc(float frq, size_t shp, size_t asym, size_t off);
  std::tuple<AbsoluteFreq&, Node&> add_abs_poly_osc_w_gain(float frq, size_t shp, size_t asym, size_t off, float amp);
  Node& add_rel_dex_osc(AbsoluteFreq& root, size_t widx, float r, float d);
  Merge& add_balance(Node& a, Node& b, float bal);
  Node& add_fm(Node& c,  Node& m, float bal, float amp);
  Node& add_fm(Node& c,  Node& m, float bal, float amp, Input& right);

  const Node& build_dex();
  const Node& build_poly();
  const Node& build_fm_simple();
  const Node& build_fm_lfo();
  const Node& build_fm_env();
  const Node& build_fm_fb();
  const Node& build_chord();

  std::unique_ptr<Wavelib> wavelib;
  std::unique_ptr<std::vector<std::unique_ptr<Node>>> current_nodes;
  std::unique_ptr<std::vector<std::unique_ptr<Param>>> current_params;
  std::unique_ptr<std::vector<std::unique_ptr<Input>>> current_inputs;
  std::unique_ptr<std::vector<std::unique_ptr<Pane>>> current_panes;
  bool test;
  
};

#endif
