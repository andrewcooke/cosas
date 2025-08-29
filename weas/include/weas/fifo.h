
#ifndef WEAS_FIFO_H
#define WEAS_FIFO_H


#include <atomic>
#include <queue>

#include "cosas/filter.h"

#include "weas/codec.h"


class FIFO;

class Stalled {
public:
  explicit Stalled(FIFO& fifo);
  ~Stalled();
  Stalled(const Stalled&) = delete;
  Stalled& operator=(const Stalled&) = delete;
private:
  FIFO& fifo;
};


// responsible for launching code on core1 and connecting fifos
// that send knob change events.

// this is the basis for a UI using the knobs.

class FIFO final : public CtrlHandler /*, public ConnectedHandler */ {

public:

  friend class Stalled;

  static constexpr uint DUMP_MASK = (1 << 10) - 1;

  FIFO(const FIFO&) = delete;
  FIFO& operator=(const FIFO&) = delete;
  static FIFO& get();

  void set_ctrl_changes(CtrlHandler* k) {ctrl_changes = k;};
  void handle_ctrl_change(CtrlEvent event) override;
  // void set_connected_changes(ConnectedHandler* c) {connected_changes = c;};
  // void handle_connected_change(uint8_t socket_in, bool connected) override;
  void start(Codec& cc);

private:

  FIFO() = default;
  CtrlHandler* ctrl_changes = nullptr;
  // ConnectedHandler* connected_changes = nullptr;
  void push(CtrlEvent);
  static void core1_marshaller();
  static constexpr uint TIMEOUT_US = 0;
  std::queue<uint32_t> overflow;
  std::atomic<bool> stalled = false;
};


#endif
