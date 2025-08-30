
#ifndef COSAS_CTRL_H
#define COSAS_CTRL_H

#include "common.h"

#include <cstddef>
#include <cstdint>

enum Header {Ctrl = 0x0 << 30, Connected = 0x1 << 30, Overflow = 0x1 << 31};


class CtrlEvent {

  public:

  enum Ctrl {Main, X, Y, Switch, Dummy};
  static constexpr size_t N_CTRLS = Switch + 1;
  enum SwitchPosition { Up, Middle, Down };  // order matches leds_direct.h2

  CtrlEvent(Ctrl ctrl, uint16_t now, uint16_t prev) : ctrl(ctrl), now(now), prev(prev) {};
  CtrlEvent(uint8_t ctrl, uint16_t now, uint16_t prev) : CtrlEvent(static_cast<CtrlEvent::Ctrl>(ctrl), now, prev) {};
  CtrlEvent() : CtrlEvent(Dummy, 0, 0) {};

  Ctrl ctrl;
  uint16_t now;
  uint16_t prev;
  uint32_t pack();

  static CtrlEvent unpack(uint32_t packed);
};


class CtrlHandler {
public:
  virtual void handle_ctrl_change(CtrlEvent /* event */) {};
  virtual ~CtrlHandler() = default;
};


class ConnectedHandler {
public:  // TODO - event class
  virtual void handle_connected_change(uint8_t /* socket_in */, bool /* connected */) {};
  virtual ~ConnectedHandler() = default;

};


class CtrlQueue {

public:
  void add(CtrlEvent event);
  CtrlEvent pop();
  bool empty();

private:
  bool empty_ = true;
  size_t offset = 0;
  CtrlEvent queue[N_KNOBS] = {CtrlEvent(), CtrlEvent(), CtrlEvent()};

};


#endif

