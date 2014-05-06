#include <ChipSelect.h>
#include <DmaSpi.h>

// create a chip select object. This one uses pin 14.
ActiveLowChipSelect<14> cs;

void setup() {
  DMASPI0.begin();
  // create a transfer object
  DmaSpi0::Transfer trx(nullptr, 100, nullptr, 0xFF, &cs);

  // and register it. If the DMA SPI is idle, it will immediately start transmitting. Otherwise the transfer is added to a queue.
  DMASPI0.registerTransfer(trx);

  /** FREE CPU TIME YOU DIDN'T HAVE BEFORE! DO SOMETHING USEFUL! **/

  // wait for the transfer to finish
  while(trx.busy());
}

void loop() {

}

