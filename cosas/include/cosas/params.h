
#ifndef COSAS_PARAMS_H
#define COSAS_PARAMS_H


class Param {

public:
  Param(float scale, float linearity, bool log, float lo, float hi)
  : scale(scale), linearity(linearity), log(log), lo(lo), hi(hi) {};
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
  float lo = 0;
  float hi = 1;

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


#endif
