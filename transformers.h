
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


// warning: this relies on being called once per sample

const size_t MAX_BOXCAR = 10000;

class Boxcar : public SingleNode {

public:

  class Length : public Param {
  public:
    Length(Boxcar* p);
    void set(float v) override;
  private:
    Boxcar* parent = nullptr;
  };

  class CircBuffer {
  public:
    CircBuffer(size_t l);
    int16_t next(int16_t cur);
  private:
    std::unique_ptr<std::vector<int32_t>> sums;
    mutable size_t circular_idx;
  };

  friend class Length;
  Boxcar(const Node& nd, size_t l);
  int16_t next(int32_t tick, int32_t phi) const override;
  Length& get_param();

private:

  std::unique_ptr<CircBuffer> cbuf;
  Length param;
  
};


// the smenatics here are maybe unexpected, but useful.  the first
// node is weighted explicitly.  the weights for subsequent nodes are
// then taken as relative to the first.

// for use as balance between two nodes this means that the second
// weight is ignored - it is simply the "remaining" weight, as
// expected.

// for use with a long list of nodes, it means that the first node is
// dominant, and the rest fill in as required.

class MergeFloat : public Node {

public:

  class Weight : public Param {
  public:
    Weight(MergeFloat* m, size_t i);
    void set(float w) override;
  private:
    MergeFloat* merge = nullptr;
    size_t idx;
  };

  friend class Weight;
  MergeFloat(const Node& n, float w);
  void add_node(const Node& n, float w);
  Weight& get_param(size_t i);
  int16_t next(int32_t tick, int32_t phi) const override;

protected:
  
  virtual void normalize();
  void add_node(const Node& n, Weight w);
  std::unique_ptr<std::vector<Weight>> params;
  std::unique_ptr<std::vector<const Node*>> nodes;
  std::unique_ptr<std::vector<float>> given_weights;
  std::unique_ptr<std::vector<float>> norm_weights;

};


class Merge14 : public MergeFloat {

public:
  
  friend class MergeFloat::Weight;
  Merge14(const Node& n, float w);
  int16_t next(int32_t tick, int32_t phi) const override;
  
protected:
  
  void normalize();
  std::unique_ptr<std::vector<uint16_t>> uint16_weights;
  
};


// forward to Gain14 on assumption this is faster
class Merge : public Merge14 {
  
public:
  
  Merge(const Node& nd, float wo);

};


#endif
