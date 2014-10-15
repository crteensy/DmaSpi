DmaSpi
======

DMA SPI for the Teensy 3.0/3.1; initial release

Master branch
--
tested with teensyduino 1.20

Discussion thread: http://forum.pjrc.com/threads/26479-DmaSpi-for-teensyduino-1-20-RC2

The DmaSpi library makes use of the following teensyduino 1.20 features:
- SPI transactions,
- dynamic DMA channel allocation (**two channels** are required for operation),
- interrupt vector table in RAM

Features
--
- A data source is optional.
  If only dummy data has to be sent to a slave, that's possible;
- A sink for data received from a slave is optional.
  Slave data can be discarded;
- The maximum transfer length is 32767;
- Transfers are queued and can have an optional chip select object associated with them (see ChipSelect.h);
- The DmaSpi can be started and stopped if necessary.
  It can be used along with other drivers that use the SPI in non-DMA mode.

An example that shows a lot of the functionality is in the examples folder.

Installation
--

Other branches
--
- teensyduino_1.18 is the pre-1.20 branch. As DMA channel handling was introduced to teensyduino after 1.18,
  this branch contains the "raw" DMA handling code
  and therefore might be interesting for those who don't want to/cannot use teensyduino.
