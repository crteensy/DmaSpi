DmaSpi
======

DMA SPI for the Teensy 3.0/3.1

This is a library for teensyduino that facilitates the use of SPI0 with DMA. In its current state, it's dead simple and uses the following hardware resources:

- SPI0
- SPI0_CTAR0 for transfer attributes
- DMA channels 0 and 1 for Rx and Tx, respectively
- DMA channel 0 ISR

The library comes with a set of utility classes for selecting slaves. This is needed to overcome a bug in the chip's silicon, but also adds flexibility.
