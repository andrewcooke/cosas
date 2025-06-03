
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
    FM,
    FM_LFO,
    FM_FB,
    FM_FMNT
  };

  Manager();
  const Node& build(Engine);
  const Pane& get_pane(size_t n);
  size_t n_panes();

  // should be private, but useful for testing
  const Node& build_fm();
  const Node& build_fm(float a);
  const Node& build_fm_lfo();
  const Node& build_fm_lfo(float a);
  const Node& build_fm_fb();
  const Node& build_fm_fb(float a);
  const Node& build_fm_fmnt();

private:

  template<typename NodeType, typename... Args> NodeType& add_node(Args&&... args);
  template<typename ParamType, typename... Args> ParamType& add_param(Args... args);
  Wavedex& add_wavedex(size_t widx);
  template<typename InputType, typename... Args> InputType& add_input(Input& del, Args... args);
  Pane& add_pane(Input& top, Input& left, Input& right);
  Blank& blank();
  Input& lin_control(Input& in, float c, float lo, float hi);
  Input& log_control(Input& in, float c, float lo, float hi);
  std::tuple<Wavedex&, AbsoluteFreq&, Oscillator&> add_abs_osc(size_t widx, float frq);
  std::tuple<Wavedex&, RelativeFreq&, Oscillator&> add_rel_osc(size_t widx, AbsoluteFreq& root, float r, float d);
  std::tuple<Amplitude&, Balance&, ModularFM&> add_fm(Node& c,  Node& m, float amp);

  std::unique_ptr<Wavelib> wavelib;
  std::unique_ptr<std::vector<std::unique_ptr<Node>>> current_nodes;
  std::unique_ptr<std::vector<std::unique_ptr<Param>>> current_params;
  std::unique_ptr<std::vector<std::unique_ptr<Input>>> current_inputs;
  std::unique_ptr<std::vector<std::unique_ptr<Pane>>> current_panes;
  
};

#endif
