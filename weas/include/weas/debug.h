
#ifndef WEAS_DEBUG_H
#define WEAS_DEBUG_H


#include <iostream>

#include "pico/stdio.h"
#include "pico/time.h"


class Debug {

private:

  static bool used;

public:

  static void init(std::ostream& os, uint n) {
    stdio_init_all();
    used = true;
    for (uint i = 0; i < n; i++) {os << "debug startup " << (n - i) << std::endl; sleep_ms(300);}
  }
  static void init() {init(std::cout, 5);}

  static void log(std::ostream& os) {os << std::endl;}
  template <typename T, typename... Args>
  static void log(std::ostream& os, const T& firstArg, const Args&... restArgs) {
    if (used) {
      os << firstArg << " ";
      log(os, restArgs...);
    }
  }

  template <typename... Args>
  static void log(const Args&... restArgs) {if (used) log(std::cout, restArgs...);}

};

#endif
