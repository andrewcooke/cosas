
#include "weas/debug.h"


Debug& Debug::get() {
  static Debug debug;
  return debug;
}


void Debug::init(std::ostream& os, uint n) {
  stdio_init_all();
  BaseDebug::init();
  for (size_t i = 0; i < n; i++) {os << "debug startup " << (n - i) << std::endl; sleep_ms(300);}
}

void Debug::init() {
  init(std::cout, 5);
}
