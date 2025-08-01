
# cosas

various utilities and applications for the Music Thing Modular
Workshop Computer (MTM WC).

the project **cosas** contains:

* the C++ library
  [cosas](https://github.com/andrewcooke/cosas/tree/main/cosas) which
  is hardware agnostic and currently focussed on audio synthesis
  (particularly fm).

* the C++ library
  [weas](https://github.com/andrewcooke/cosas/tree/main/weas) which is
  based on a refactoring of the ComputerCard.h library and only runs
  on the RP2040 based MTM WC.

* [executables](https://github.com/andrewcooke/cosas/tree/main/apps)
  built on the two libraries above.  these either test functionality
  in cosas or are applications that can be deployed to the MTM WC.
  not all executables will compile - some are historical artefacts.  a
  good starting example is
  [diag](https://github.com/andrewcooke/cosas/tree/main/apps/diag)
  which is useful to diagnose soldering issues.

* other scraps that were need along the way (currently some python
  code to explore DNL errors)

the plan is to have a unified UI via the weas library (input via three
knobs and a switch, display via 6 leds - what luxury!)

significant waypoints:
* 2025-07-31 - refactored codec to avoid template mess; LED based UI started
* 2025-07-26 - ComputerCard.h split into led, eeprom and codec
* 2025-07-18 - use ComputerCard.h more directly (refactor rather than recreate)
* 2025-07-03 - can flash LEDs on the pico
* 2025-06-30 - can generate waveforms on the laptop and deploy an empty program to the pico

all development is done inside debian (typically latest testing) with
the clion ide.  running install.sh should get you started.

*hecho en chile con amooooor*
