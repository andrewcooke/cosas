
#ifndef COSA_TRANSFORMERS_H
#define COSA_TRANSFORMERS_H

#include <vector>

#include "cosas/params.h"
#include "cosas/node.h"


// these have one input (modulators have two)


class SingleSource : public RelSource {
protected:
  explicit SingleSource(const RelSource& src) : src(src) {};
  const RelSource& src;
};


class SingleFloat : public SingleSource {
public:
  class Value final : public Param {
  public:
    explicit Value(SingleFloat* p);
    void set(float v) override;
  private:
    SingleFloat* parent;
  };
  friend class Value;
protected:
  SingleFloat(const RelSource& src, float v);
  float value;
  Value param;
};


class GainFloat final : public SingleFloat {
public:
  GainFloat(const RelSource& src, float amp);
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) const override;
  Value& get_amp();
};


class Single14 : public SingleSource {
public:
  class Value final : public Param {
  public:
    explicit Value(Single14* p);
    void set(float v) override;
  private:
    Single14* parent;
  };
  friend class Value;
protected:
  Single14(const RelSource& src, float v);
  uint16_t value;
  Value param;
  const float v;
};


class Gain14 : public Single14 {
public:
  Gain14(const RelSource& src, float amp);
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) const override;
  Value& get_amp();
};


// forward to Gain14 on assumption this is faster
class Gain : public Gain14 {
public:
  Gain(const RelSource& src, float amp);
};


class FloatFunc : public SingleFloat {
public:
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) const override;
protected:
  FloatFunc(const RelSource& src, float v);
  // x is normalised 0-1 and this can (will) use value
  [[nodiscard]] virtual float func(float x) const = 0;
};


class Compander final : public FloatFunc {
public:
  Compander(const RelSource& src, float gamma);
private:
  [[nodiscard]] float func(float x) const override;
};


class Folder final : public FloatFunc {
public:
  // k is progrgessive, 0-1 expands and 1-2 folds
  Folder(const RelSource& src, float k);
  Value& get_fold();
private:
  [[nodiscard]] float func(float x) const override;
};


// warning: this relies on being called once per sample

constexpr size_t MAX_BOXCAR = 10000;

class Boxcar final : public SingleSource {
public:
  class Length final : public Param {
  public:
    explicit Length(Boxcar* p);
    void set(float v) override;
  private:
    Boxcar* parent = nullptr;
  };
  class CircBuffer {
  public:
    explicit CircBuffer(size_t l);
    int16_t next(int16_t cur) const;
  private:
    std::unique_ptr<std::vector<int32_t>> sums;
    mutable size_t circular_idx;
  };
  friend class Length;
  Boxcar(const RelSource& src, size_t l);
  [[nodiscard]] int16_t next(int32_t delta, int32_t phi) const override;
  Length& get_len();
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

class MergeFloat : public RelSource {
public:
  class Weight final : public Param {
  public:
    Weight(MergeFloat* m, size_t i);
    void set(float w) override;
  private:
    MergeFloat* merge = nullptr;
    size_t idx;
  };
  friend class Weight;
  MergeFloat(const RelSource& src, float w);
  void add_source(const RelSource& src, float w);
  [[nodiscard]] Weight& get_weight(size_t i) const;
  [[nodiscard]] int16_t next(int32_t tick, int32_t phi) const override;
protected:
  virtual void normalize();
  std::unique_ptr<std::vector<Weight>> weights;
  std::unique_ptr<std::vector<const RelSource*>> sources;
  std::unique_ptr<std::vector<float>> given_weights;
  std::unique_ptr<std::vector<float>> norm_weights;
};


class Merge14 : public MergeFloat {
public:
  friend class Weight;
  Merge14(const RelSource& src, float w);
  [[nodiscard]] int16_t next(int32_t tick, int32_t phi) const override;
protected:
  void normalize() override;
  std::unique_ptr<std::vector<uint16_t>> uint16_weights;
};


// forward to Gain14 on assumption this is faster
class Merge final : public Merge14 {
public:
  Merge(const RelSource& src, float w);
};


#endif
