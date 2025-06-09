
#ifndef COSA_TRANSFORMERS_H
#define COSA_TRANSFORMERS_H

#include <vector>

#include "constants.h"
#include "params.h"
#include "node.h"


// these have one input (modulators have two)


class SingleNode : public Node {

protected:

  SingleNode(const Node& nd) : node(nd) {};
  const Node& node;
  
};


class SingleFloat : public SingleNode {

public:

  class Value : public Param {
  public:
    Value(SingleFloat* p);
    void set(float v) override;
  private:
    SingleFloat* parent;
  };

  friend class Value;
  Value& get_param();
    
protected:

  SingleFloat(const Node& nd, float v);
  float value;

private:

  Value param;
  
};


class GainFloat : public SingleFloat {

public:
  
  GainFloat(const Node& nd, float amp);
  int16_t next(int32_t tick, int32_t phi) const override;
  
};


class Single14 : public SingleNode {

public:

  class Value : public Param {
  public:
    Value(Single14* p);
    void set(float v) override;
  private:
    Single14* parent;
  };

  friend class Value;
  Value& get_param();
    
protected:

  Single14(const Node& nd, float v);
  uint16_t value;

private:

  Value param;
  
};


class Gain14 : public Single14 {

public:
  
  Gain14(const Node& nd, float amp);
  int16_t next(int32_t tick, int32_t phi) const override;
  
};


// forward to Gain14 on assumption this is faster
class Gain : public Gain14 {
  
public:
  
  Gain(const Node& nd, float amp);

};


class FloatFunc : public SingleFloat {

public:
  
  int16_t next(int32_t tick, int32_t phi) const override;

protected:

  FloatFunc(const Node& nd, float v);
  // x is normalised 0-1 and this can (will) use value
  virtual float func(float x) const = 0;  

};


class Compander : public FloatFunc {

public:

  Compander(const Node& nd, float gamma);

private:
  
  float func(float x) const override;
  
};


class Folder : public FloatFunc {

public:

  // k is progrgessive, 0-1 expands and 1-2 folds
  Folder(const Node& nd, float k);

private:
  
  float func(float x) const override;
  
};


class MeanFilter : public SingleNode {

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


class BaseMerge : public Node {

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
  std::unique_ptr<std::vector<uint16_t>> uint16_weights;

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
