
#include "doctest/doctest.h"

#include "cosas/transformers.h"


TEST_CASE("Transformers, GainFloat") {
  Constant c = Constant(100);
  GainFloat g = GainFloat(c, 1);
  CHECK(g.next(123, 0) == 100);
  g.get_amp().set(0.1);
  CHECK(g.next(123, 0) == 10);
}


TEST_CASE("Transformers, Gain14") {
  Constant c = Constant(100);
  Gain14 g = Gain14(c, 1);
  CHECK(g.next(123, 0) == 100);
  g.get_amp().set(0.1);
  CHECK(g.next(123, 0) == 9);  // almost
}


TEST_CASE("Transformers, Folder") {
  Constant c1234 = Constant(1234);  // random +ve value
  Folder f0_12 = Folder(c1234, 0);
  CHECK(f0_12.next(0, 0) == 1234);
  f0_12.get_fold().set(1);
  CHECK(f0_12.next(0, 0) == 2421);  // not sure if correct, but more +ve
  f0_12.get_fold().set(2);
  CHECK(f0_12.next(0, 0) == 4750);  // not sure if correct
  Constant cm1234 = Constant(-1234);
  Folder f1_12x = Folder(cm1234, 1);
  CHECK(f1_12x.next(0, 0) == -2421);  // symmetrical
  
  Constant cmax = Constant(sample_max);  
  Folder f0_max = Folder(cmax, 0);
  CHECK(f0_max.next(0, 0) == sample_max);
  f0_max.get_fold().set(1);
  CHECK(f0_max.next(0, 0) == sample_max);
  f0_max.get_fold().set(2);
  CHECK(f0_max.next(0, 0) == 0);
}


TEST_CASE("Transformers, Boxcar") {
  Sequence s1 = Sequence({0, 0, 100});
  Boxcar b1 = Boxcar(s1, 3);
  CHECK(b1.next(0, 0) == 0);
  CHECK(b1.next(0, 0) == 0);
  CHECK(b1.next(0, 0) == 33);
  CHECK(b1.next(0, 0) == 33);
  CHECK(b1.next(0, 0) == 33);
  CHECK(b1.next(0, 0) == 0);
  CHECK(b1.next(0, 0) == 0);
  Sequence s2 = Sequence({0, 0, 100});
  Boxcar b2 = Boxcar(s2, 3);
  b2.get_len().set(1);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 100);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
  CHECK(b2.next(0, 0) == 0);
}


TEST_CASE("Transformers, MergeFloat") {
  Constant c1 = Constant(100);
  Constant c2 = Constant(30);
  Constant c3 = Constant(90);
  MergeFloat m = MergeFloat(c1, 0.5);
  m.add_node(c2, 0.1);
  m.add_node(c3, 0.2);
  CHECK(m.next(0, 0) == 50 + 5 + 30);
  m.get_weight(0).set(0);
  CHECK(m.next(0, 0) == 10 + 60);
}


TEST_CASE("Transformers, Merge14") {
  Constant c1 = Constant(100);
  Constant c2 = Constant(30);
  Constant c3 = Constant(90);
  Merge14 m = Merge14(c1, 0.5);
  m.add_node(c2, 0.1);
  m.add_node(c3, 0.2);
  CHECK(m.next(0, 0) == 50 + 5 + 30 - 2);  // close enough?
  m.get_weight(0).set(0);
  CHECK(m.next(0, 0) == 10 + 60 - 2);  // ditto
}

