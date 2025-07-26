
# cosas

various utilities and applications for the Music Thing Modular
Workshop Computer (MTM WC).

the project **cosas** contains:

* the C++ library **cosas** which is hardware agnostic and currently
  focussed on audio synthesis (particularly fm).

* the C++ library **weas** which is based on a refactoring of the
  CoomputerCard.h librray and only runs on the RP2040 based MTM WC.

* various executables built on the two libraries above.  these either
  test functionality in cosas or are applications that can be deployed
  to the MTM WC.  not all executables will compile - some are
  historical artefacts.

* various other scraps that were need along the way (currently some
  python code to explore DNL errors)

the plan is to have a unified UI via the weas library.

significant waypoints:
* 2025-07-26 - ComputerCard.h split into led, eeprom and codec
* 2025-07-18 - use ComputerCard.h more directly (refactor rather than recreate)
* 2025-07-03 - can flash LEDs on the pico
* 2025-06-30 - can generate waveforms on the laptop and deploy an empty program to the pico

*made in chile with love*
