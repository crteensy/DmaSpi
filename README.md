DmaSpi
======

DMA SPI for the Teensy 3.0/3.1
Branch for teensyduino 1.20 RC2

This branch is currently in development and will change a lot in the coming weeks (starting somewhen after August 17th, 2014, but not earlier). It will make use of the following teensyduino 1.20 RC2 features:
- SPI transactions,
- dynamic DMA channel allocation,
- interrupt vector table in RAM

This is a library for teensyduino that facilitates the use of SPI0 with DMA. In its current state, it's dead simple and uses the following hardware resources:
- SPI0, using SPI transactions
- 2 dynamically allocated DMA channels

The library comes with a set of utility classes for selecting slaves, see ChipSelect.h

Examples will follow soon.
