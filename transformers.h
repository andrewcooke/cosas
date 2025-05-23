
#ifndef COSA_TRANSFORMERS_H
#define COSA_TRANSFORMERS_H

#include <vector>

#include "constants.h"
#include "params.h"
#include "node.h"
#include "lookup.h"


// these have one input (modulators have two)


class Transformer : public Node {

protected:

  Transformer(const Node& nd) : node(nd) {};
  const Node& node;
  
};


class Gain : public Transformer {
    
public:
    
  Gain(const Node& nd, const Amplitude amp);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  const Amplitude amplitude;
  
};


class OneParFunc : public Transformer {

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


class MeanFilter : public Transformer {

public:
    
  MeanFilter(const Node& nd, int len);
  int16_t next(int32_t tick, int32_t phi) const override;

private:

  std::unique_ptr<std::vector<int32_t>> sums;
  mutable size_t circular_idx;
  
};


#endif
