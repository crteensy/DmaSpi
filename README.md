DmaSpi
======

DMA SPI for the Teensy 3.0/3.1/LC

teensyduino 1.21 test #6 branch
--
Discussion thread:
teensyduino 1.21 announcement thread: https://forum.pjrc.com/threads/27403-Teensyduino-1-21-Test-2-Available
Teensy LC beta testing thread: https://forum.pjrc.com/threads/27689-Teensy-LC-Beta-Testing
Old 1.20 Discussion thread: http://forum.pjrc.com/threads/26479-DmaSpi-for-teensyduino-1-20-RC2

The DmaSpi library makes use of the following teensyduino features:
- SPI transactions,
- dynamic DMA channel allocation
- interrupt vector table in RAM

Requirements
--
- The DMA SPI needs two DMA channels per SPI peripheral for operation.
- the new teensy-LC has two SPIs, thus four DMA channels are required if both SPIs are used in DMA mode.
  In that case, no DMA channels are left for other operations!
- buffers (data sources and sinks) must remain valid until the library has finished reading from/writing to them.

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

An example that shows a lot of the functionality is in the examples folder. This example only shows how to use SPI0; SPI1 (Teensy LC) is untested.

Some Notes
--
- The Teensy LC introduced a second working SPI. There is now an abstract base class (AbstractDmaSpi) which has all the code that is not
  chip-specific. Only the chip-specific code is in the derived classes DmaSpi0 and DmaSpi1. All methods and variables are static so that
  it's not dangerous to create multiple instances of the classes - they access the same static state.
- the first call to begin() initializes DmaSpi0 or DmaSpi1. Further calls have no effect until a matching number of calls to end()
  have been made. The last call to end() de-initializes DmaSpi0 or DmaSpi1.
- One instance of each DmaSpi class is created, they are called DMASPI0 (Teensy 3.0, 3.1 and LC)
  and DMASPI1 (Teensy LC only). If the DmaSpi header is included, these are visible.
- The Transfer class has been moved into a namespace called DmaSpi. The two DmaSpi classes import this type, so it's possible to use
  `DmaSpi::Transfer`, `DmaSpi0::Transfer`, `DMASPI0::Transfer`, `DmaSpi1::Transfer` and `DMASPI1::Transfer`.
- With a bit of trickery, it's possible to write code that uses either DmaSpi class through the base class `AbstractDmaSpi`.

Installation
--
Since arduino 1.0.6 it seems to be possible to just point arduino to the zip file containing this library.
Go to "Sketch"->"Import Library"->"Add Library" and navigate to the zip file.
Arduino should accept the underscores contained in the file name. This didn't work in previous versions, where you
had to rename the archive.

Other branches
--
- teensyduino_1.18 is the pre-1.20 branch. As DMA channel handling was introduced to teensyduino after 1.18,
  the 1.18 branch contains the "raw" DMA handling code
  and therefore might be interesting for those who don't want to/cannot use teensyduino.
