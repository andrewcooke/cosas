
#include <iostream>
#include <cmath>

#include "console.h"
#include "engine.h"


void dump(const Source& source, int64_t n) {
  for (int64_t i = 0; i < n; i++) {
    int16_t amp = source.next(i);
    std::cout << i << " " << amp << std::endl;
  }
}


void dump_fm(size_t n) {
  auto m = Manager();
  const Node& fm = m.build_fm();
  auto p = m.get_pane(m.n_panes());
  for (int64_t i = 0; i < n; i++) {
    p.top_knob.set(pow(i / (float)n, 2));
    int16_t amp = fm.next(i);
    std::cout << i << " " << amp << std::endl;
  }
}
