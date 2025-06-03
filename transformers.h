
#ifndef COSA_TRANSFORMERS_H
#define COSA_TRANSFORMERS_H

#include <vector>

#include "constants.h"
#include "params.h"
#include "node.h"


// these have one input (modulators have two)


class Transformer : public Node {};


class NodeTransformer : public Transformer {

protected:

  NodeTransformer(const Node& nd) : node(nd) {};
  const Node& node;
  
};


class Gain : public NodeTransformer {
    
public:
    
  Gain(const Node& nd, const Amplitude& amp);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  const Amplitude& amplitude;
  
};


// these need to be re-done with Inputs
class OneParFunc : public NodeTransformer {

public:

  OneParFunc(const Node& nd, const float k);
  int16_t next(int32_t tick, int32_t phi) const override;

protected:

  virtual float func(float k, float x) const = 0;
  
private:

  const float constant;
  
};


class Compander : public OneParFunc {

public:

  Compander(const Node& nd, float g);

private:
  
  float func(float k, float x) const override;
  
};


class Folder : public OneParFunc {

public:

  Folder(const Node& nd, float g);

private:
  
  float func(float k, float x) const override;
  
};


class MeanFilter : public NodeTransformer {

public:

  class Length : public Param {
  public:
    friend class MeanFilter;
    Length(size_t l);
    void set(float /* f */) override {};
  private:
    size_t len;
    MeanFilter* filter = nullptr;
  };

  class CircBuffer {
  public:
    CircBuffer(size_t l);
    int16_t next(int16_t cur);
  private:
    std::unique_ptr<std::vector<int32_t>> sums;
    mutable size_t circular_idx;
  };
       
  MeanFilter(const Node& nd, Length l);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  Length len;
  std::unique_ptr<CircBuffer> cbuf;
  
};


class BaseMerge : public Transformer {

public:

  class Weight : public Param {
  public:
    friend class BaseMerge;
    Weight(float w);
    void set(float /* f */) override {};
  private:
    float weight;
    BaseMerge* merge = nullptr;
  };

  BaseMerge(const Node& n, Weight w);
  void add_node(const Node& n, Weight w);
  int16_t next(int32_t tick, int32_t phi) const override;

protected:
  
  virtual void recalculate_weights() = 0;
  std::unique_ptr<std::vector<const Node*>> nodes;
  std::unique_ptr<std::vector<float>> float_weights;
  std::unique_ptr<std::vector<int16_t>> int16_weights;

private:

  void add_node_wout_recalc(const Node& n, Weight w);
  
  
};


class MultiMerge : public BaseMerge {

public:

  MultiMerge(const Node& n, BaseMerge::Weight w);

private:

  void recalculate_weights() override;
  
};


class PriorityMerge : public BaseMerge {

public:

  PriorityMerge(const Node& n, BaseMerge::Weight w);

private:

  void recalculate_weights() override;
  
};


#endif
