
# weas

a low-level library for interfacing to the MTM computer.

applications that run on the MTM computer combine this with the cosas
library to do something useful.

some parts of the code are based on the ComputerCard (CC) library, but
the API has changed a lot, so this is not a pluggable replacement (or
vice versa).  but, still, it's fair to ask how they compare.  the
biggest difference is that CC is written for "everyone", while weas is
written just for me.  so i am playing around, learning C++ and
embedded programming, seeing how i can push things in interesting
directions, all while experimenting with user interfaces and sound
generation.  i don't care abut backwards compatibility, or supporting
other people, so use at your own risk!  especially since CC is much
more widely used and better tested (and likely to be faster and more
compact).

on the other hand, weas does have some interesting features:

* you can choose what frequency you want to generate output at.  so if
  you want to write very complex code that generates CV output, you
  could do that by reducing output frequency to 1kHz (a lower output
  frequency gives you more time to think between outputs).

* you can choose how much oversampling it does, which might be useful
  if you want to explore hardware limits.

* there is support for a UI running on core 1 (while core 0 handles
  the sounds generation and hardware).

* you can what frequency you want the UI to work at (no need to handle
  knob changes at audio frequencies!)
  