
#include "doctest/doctest.h"

#include "cosas/ctrl.h"


TEST_CASE("CtrlQueue, pop") {
  CtrlQueue queue;
  queue.add(CtrlEvent(CtrlEvent::Main, 1, 0));
  queue.add(CtrlEvent(CtrlEvent::Main, 2, 1));
  queue.add(CtrlEvent(CtrlEvent::X, 1, 0));
  queue.add(CtrlEvent(CtrlEvent::Switch, 1, 0));
  queue.add(CtrlEvent(CtrlEvent::Main, 1, 0));
  CHECK(!queue.empty());
  CHECK(queue.pop() == CtrlEvent(CtrlEvent::Switch, 1, 0));
  CHECK(queue.empty());
  queue.add(CtrlEvent(CtrlEvent::Main, 1, 0));
  queue.add(CtrlEvent(CtrlEvent::Main, 2, 1));
  queue.add(CtrlEvent(CtrlEvent::Main, 4, 2));
  queue.add(CtrlEvent(CtrlEvent::Main, 3, 4));
  queue.add(CtrlEvent(CtrlEvent::X, 1, 0));
  CHECK(queue.pop() == CtrlEvent(CtrlEvent::X, 1, 0));
  CHECK(queue.pop() == CtrlEvent(CtrlEvent::Main, 3, 0));
}

TEST_CASE("CtrlEvent, pack/unpack") {
  auto round_trip = [](CtrlEvent event) {
    CHECK(event == CtrlEvent::unpack(event.pack()));
  };
  round_trip(CtrlEvent(1, 2, 3));
}