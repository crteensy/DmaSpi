DmaSpi
======

DMA SPI for the Teensy 3.0/3.1/LC/ 3.5 and 3.6 (experimental)

Current development discussion thread: https://forum.pjrc.com/threads/35760-DMASPI-library-needs-some-(probably-breaking)-changes-to-really-support-multiple-SPIs

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
  If only dummy data has to be sent to a slave, that's possible without a data source buffer;
- A sink for data received from a slave is optional.
  Slave data can be discarded;
- The maximum transfer length is 32767;
- Transfers are queued and can have an optional chip select object associated with them (see ChipSelect.h);
- The DmaSpi can be started and stopped if necessary.
  It can be used along with other drivers that use the SPI in non-DMA mode.

An example that shows a lot of the functionality is in the examples folder. This example only shows how to use SPI0; SPI1 and SPI2 (if present) are not used.

Some Notes
--
- The Teensy LC introduced a second working SPI, Teensy 3.5 nd 3.6 even have a third. There is now an abstract base class (AbstractDmaSpi) which has all the code that is not
  chip-specific. Only the chip-specific code is in the derived classes DmaSpi0, DmaSpi1 and DmaSpi2. All methods and variables are static so that
  it's not dangerous to create multiple instances of the classes - they access the same static state.
- the first call to begin() initializes DmaSpi. Further calls have no effect until a matching number of calls to end()
  have been made. The last call to end() de-initializes DmaSpi.
- One instance of each DmaSpi class is created, they are called DMASPI0 (Teensy 3.0, 3.1 and LC)
  DMASPI1 (Teensy LC, 3.5, 3.6). If the DmaSpi header is included, these are visible.
- The Transfer class has been moved into a namespace called DmaSpi. The two DmaSpi classes import this type, so it's possible to use
  `DmaSpi::Transfer`, `DmaSpi0::Transfer`, `DMASPI0::Transfer`, `DmaSpi1::Transfer`, `DMASPI1::Transfer`, `DmaSpi2::Transfer`, `DMASPI2::Transfer`.
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
