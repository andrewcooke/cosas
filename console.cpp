
#include <iostream>

#include "console.h"


void dump(Source& source, int64_t n) {
  for (int64_t i = 0; i < n; i++) {
    std::cout << i << " " << source.next(i) << std::endl;
  }
}
