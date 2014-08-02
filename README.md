DmaSpi
======

DMA SPI for the Teensy 3.0/3.1
Branch for teensyduino 1.20 RC2

This branch is currently in development and will change a lot in the coming weeks (starting somewhen after August 17th, 2014, but not earlier). It will make use of the following teensyduino 1.20 RC2 features:
- SPI transactions,
- dynamic DMA channel allocation,
- interrupt vector table in RAM

This is a library for teensyduino that facilitates the use of SPI0 with DMA. In its current state, it's dead simple and uses the following hardware resources:
- SPI0
- SPI0_CTAR0 for transfer attributes
- DMA channels 0 and 1 for Rx and Tx, respectively
- DMA channel 0 ISR

The library comes with a set of utility classes for selecting slaves. This is needed to overcome a bug in the chip's silicon, but also adds flexibility.

Installation instructions for the arduino IDE
---------------------------------------------
If you have downloaded this library from github (https://github.com/crteensy/DmaSpi), you will need to install the library manually, because the automatically generated zip filename breaks arduino compatibility.

Here's how:
- download zip file, the filename should be DmaSpi-teensyduino_1.20_RC2.zip
- unzip where you like, and rename the resulting folder (DmaSpi-master) to DmaSpi
- move that folder to your folder for contributed arduino libraries; see http://arduino.cc/en/Guide/Libraries -> Manual Installation for instructions
- restart your arduino IDE if it was running

