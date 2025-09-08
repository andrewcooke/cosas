
#ifndef COSAS_ENGINE_BASE_H
#define COSAS_ENGINE_BASE_H

#include <tuple>
#include <vector>
#include <memory>

#include "cosas/transformers.h"
#include "cosas/wavelib.h"
#include "cosas/oscillator.h"
#include "cosas/params.h"
#include "cosas/pane.h"


// UNUSED - may be broken

class BaseManager {

public:

  BaseManager();
  [[nodiscard]] Pane& get_pane(size_t n) const;
  [[nodiscard]] size_t n_panes() const;

protected:

  template <typename SourceType, typename... Args>
  SourceType& add_source(Args&&... args) {
    std::unique_ptr<SourceType> source = std::make_unique<SourceType>(std::forward<Args>(args)...);
    current_sources->push_back(std::move(source));
    return dynamic_cast<SourceType&>(*current_sources->back());
  }

  template <typename ParamType, typename... Args>
  ParamType& add_param(Args&&... args) {
    std::unique_ptr<ParamType> input = std::make_unique<ParamType>(std::forward<Args>(args)...);
    current_params->push_back(std::move(input));
    return dynamic_cast<ParamType&>(*current_params->back());
  }

  void clear_all();
  Pane& add_pane(Param& main, Param& x, Param& y) const;
  void swap_panes(size_t i, size_t j) const;
  void rotate_panes(size_t a, size_t b) const;
  AbsPolyOsc& add_abs_poly_osc(float frq, size_t shp, size_t asym, size_t off);
  std::tuple<Gain&, AbsPolyOsc&> add_abs_poly_osc_w_gain(float frq, size_t shp, size_t asym, size_t off, float amp);
  RelPolyOsc& add_rel_poly_osc(AbsFreqParam& frq, size_t shp, size_t asym, size_t off);
  std::tuple<Gain&, RelPolyOsc&> add_rel_poly_osc_w_gain(AbsFreqParam& frq, size_t shp, size_t asym, size_t off, float amp);
  Merge& add_balance(RelSource& a, RelSource& b, float bal);
  RelSource& add_fm(RelSource& c, RelSource& m, float bal);
  RelSource& add_fm(RelSource& c,  RelSource& m, float bal, float amp);
  RelSource& add_fm(RelSource& c,  RelSource& m, float bal, float amp, Param& right);

private:

  std::unique_ptr<std::vector<std::unique_ptr<RelSource>>> current_sources;
  std::unique_ptr<std::vector<std::unique_ptr<Param>>> current_params;
  std::unique_ptr<std::vector<std::unique_ptr<Pane>>> current_panes;
};


#endif

