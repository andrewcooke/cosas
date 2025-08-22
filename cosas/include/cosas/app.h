
#ifndef COSAS_APP_H
#define COSAS_APP_H


#include <cstdint>

#include "source.h"



// this is the interface that the weas ui expects


class KnobSpec {

public:

  enum Knob { Main, X, Y };
  static constexpr uint8_t N_KNOBS = Y + 1;
  KnobSpec(float lo, float hi, bool log, void (*callback)(float), float scale, float linearity);
  const float lo;
  const float hi;
  const bool log;
  void (*callback)(float v);
  const float scale;
  const float linearity;

};


class App {

public:

  virtual ~App() = default;
  virtual uint8_t get_n_sources() = 0;
  virtual RelSource& get_source(uint8_t source) = 0;
  virtual uint8_t get_n_pages(uint8_t source) = 0;
  virtual KnobSpec get_knob_spec(uint8_t page, KnobSpec::Knob knob) = 0;
  virtual void set_source(uint8_t source) = 0;

};


class DummyApp : public App {

public:

  uint8_t get_n_sources() override;
  RelSource& get_source(uint8_t source) override;
  uint8_t get_n_pages(uint8_t source) override;
  KnobSpec get_knob_spec(uint8_t page, KnobSpec::Knob knob) override;
  void set_source(uint8_t /* source */) override {};

};


#endif
