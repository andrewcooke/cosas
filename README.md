
# cosas

various utilities and applications for the Music Thing Modular
Workshop Computer (MTM WC):

* the C++ library
  [cosas](https://github.com/andrewcooke/cosas/tree/main/cosas) which
  is hardware agnostic and currently focussed on audio synthesis
  (particularly fm).

* the C++ library
  [weas](https://github.com/andrewcooke/cosas/tree/main/weas) which is
  based on a refactoring of the ComputerCard.h (CC) library and only
  runs on the RP2040 based MTM WC.

* [executables](https://github.com/andrewcooke/cosas/tree/main/apps)
  built on the two libraries above.  these either test functionality
  in cosas or are applications that can be deployed to the MTM WC.
  note - many are historical artefacts and may not currently compile.

* other scraps that were need along the way (currently some python
  code to explore DNL errors)

the plan is to have a unified UI via the weas library (input via three
knobs and a switch, display via 6 leds - what luxury!)

significant waypoints:
* 2925-09-03 - reasonable / usable signal generation
* 2025-08-31 - first signal generated
* 2025-08-25 - refactored audio code to interface to knobs in UI
* 2025-08-22 - finally have LEDs and knobs working cleanly
* 2025-08-02 - updated to include CC 0.2.6 changes (afaict)
* 2025-07-31 - refactored codec to avoid template mess; LED based UI started
* 2025-07-26 - CC split into led, eeprom and codec
* 2025-07-18 - use CC more directly (refactor rather than recreate)
* 2025-07-03 - can flash LEDs on the pico
* 2025-06-30 - can generate waveforms on the laptop and deploy an empty program to the pico

all development is done inside debian (typically latest testing) with
the clion IDE.  running install.sh should get you started.

*hecho en chile con amooooor*
