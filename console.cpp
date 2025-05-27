
#include <iostream>

#include "console.h"


void dump(const Source& source, int64_t n) {
  for (int64_t i = 0; i < n; i++) {
    int16_t amp = source.next(i);
    std::cout << i << " " << amp << " (dump)" << std::endl;
  }
}
