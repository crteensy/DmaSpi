DmaSpi
======

DMA SPI for the Teensy family of ARM microcontroller boards.

Current teensyduino version: 1.39 release

What works:

| Teensy | SPI0 | SPI1 | SPI2 |
| -      | -    | -    | -    |
| 3.6    | yes  | yes  | n/i  |
| 3.5    | exp  | no   | n/i  |
| 3.2    | exp  | --   | --   |
| 3.1    | exp  | --   | --   |
| 3.0    | exp  | --   | --   |
| LC     | exp  | exp  | --   |

where
- yes: tested and works
- no: tested and doesn't work or doesn't seem feasible
- exp: should work, but not sure
- n/i: not implemented yet
- --: SPI doesn't exist

Current development discussion thread: https://forum.pjrc.com/threads/35760-DMASPI-library-needs-some-(probably-breaking)-changes-to-really-support-multiple-SPIs

The DmaSpi library makes use of the following teensyduino features:
- SPI transactions,
- dynamic DMA channel allocation
- interrupt vector table in RAM

Requirements
--
- The DMA SPI needs two DMA channels per SPI peripheral for operation.
- Teensy LC: If both SPIs are used with DMA, no DMA channels are left for other purposes because the chip only has four of them.
- buffers (data sources and sinks) must remain valid until the library has finished reading from/writing to them.

Features
--
- A data source is optional.
  If only dummy data has to be sent to a slave, that's possible without a data source buffer;
- A sink for data received from a slave is optional.
  Slave data can be discarded;
- The maximum transfer length is 32767 bytes;
- Transfers are queued and can have an optional chip select object associated with them (see ChipSelect.h);
- The DmaSpi can be started and stopped if necessary.
  It can be used along with other drivers that use the SPI in non-DMA mode.

An example that shows a lot of the functionality is in the examples folder. This example only shows how to use SPI0; SPI1 and SPI2 (if present) are not used.

Some Notes
--
- The Teensy LC introduced a second working SPI, Teensy 3.5 and 3.6 even have three. There is now an abstract base class (AbstractDmaSpi) which has all the code that is not
  chip-specific. Only the chip-specific code is in the derived classes DmaSpi0, DmaSpi1 and DmaSpi2. All methods and variables are static so that
  it's not dangerous to create multiple instances of the classes - they access the same static state.
- The ActiveLowChipSelect class is meant to be an example to be used with SPI. It will not work with SPI1 or SPI2 because it is hard-coded to use that one SPI only. You can copy its source code and adapt it accordingly, see ChipSelect.h.
- the first call to begin() initializes DmaSpi. Further calls have no effect until a matching number of calls to end()
  have been made. The last call to end() de-initializes DmaSpi.
- One instance of each DmaSpi class is created, they are called DMASPI0 (Teensy 3.0, 3.1, 3.2, 3.6 and LC) and
  DMASPI1 (Teensy 3.6 and LC). If the DmaSpi header is included, these are visible. DMASPI2 is commented out but should work with Teensy  3.6.
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
