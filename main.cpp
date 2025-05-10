
import std;
using namespace std;

#include "maths.h"


int main() {

  int n = 1000;
  SimpleRatio sr1 = SimpleRatio(0);
  for (int i = 0; i < n; ++i) {
    float f = i / (float)n;
    SimpleRatio sr2 = SimpleRatio(f);
    if (sr1 != sr2) {
      cout << f << " " << sr2 << endl;
      sr1 = sr2;
    }
  }
  for (int i = 0; i < n; ++i) {
    float f = 1 + i / (float)n;
    SimpleRatio sr2 = SimpleRatio(f);
    if (sr1 != sr2) {
      cout << f << " " << sr2 << endl;
      sr1 = sr2;
    }
  }
  for (int i = 0; i < 20000; ++i) {
    float f = i;
    SimpleRatio sr2 = SimpleRatio(f);
    if (sr1 != sr2) {
      cout << f << " " << sr2 << endl;
      sr1 = sr2;
    }
  }
  
}
