
import std;
using namespace std;

#include "maths.h"


int main() {

  int n = 1000;
  SimpleRatio sr1 = SimpleRatio(0);
  for (int decade = -2; decade < 4; decade++) {
    for (int centi = 0; centi < 100; ++centi) {
      float f = pow(10, decade + centi / 100.0);
      SimpleRatio sr2 = SimpleRatio(f);
      if (sr1 != sr2) {
	cout << f << " " << sr2 << endl;
	sr1 = sr2;
      }
    }
  }
  
}
