
#ifndef WEAS_DEBUG_H
#define WEAS_DEBUG_H


#include <iostream>

#include "pico/stdio.h"
#include "pico/time.h"

#include "cosas/debug.h"


class Debug : public BaseDebug {

public:

  static Debug& get();
  void init(std::ostream& os, uint n);
  void init() override;
  template <typename... Args>
  static void log(const Args&... restArgs) {BaseDebug::log(restArgs...);}

private:

  Debug() = default;

};

#endif
