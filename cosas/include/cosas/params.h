
#ifndef COSAS_PARAMS_H
#define COSAS_PARAMS_H


#include <unistd.h>
#include <cmath>

class Param {

public:
  Param(float scale, float linearity, bool log, float lo, float hi)
  : scale(scale), linearity(linearity), log(log), lo(lo), hi(hi),
    clip_lo(log ? powf(10, lo) : lo), clip_hi(log ? powf(10, hi) : hi) {};
  Param() : valid(false) {};
  virtual ~Param() = default;
  virtual void set(float v) = 0;
  virtual float get() = 0;
  // these match Knob and make it easy for the app to connect a knob
  // ideally they would be const, but Blank needs to change them
  bool valid = true;
  float scale = 1;
  float linearity = 1;  // 0 flat, 1 linear
  bool log = false;
  float lo = 0;  // if log, these are log10 of final value
  float hi = 1;

protected:
  float clip(float v);

private:
  float clip_lo;
  float clip_hi;

};


class Blank final : public Param {

public:
  Blank();
  void set(float value) override;
  float get() override;
  void unblank(Param* del);

private:
  Param* delegate;

};


class DummyParam : public Param {
public:
  DummyParam(float scale, float linearity, bool log, float lo, float hi)
    : Param(scale, linearity, log, lo, hi) {};
  void set(float /* v */) override {};
  float get() override {return 0.5;}
};


#endif
