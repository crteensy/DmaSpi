//#include <WProgram.h>

#include <SPI.h>
#include <DmaSpi.h>

/** Important
  The sketch waits for user input through Serial (USB) before it starts the setup.
  After any key was pressed, it should begin to work. It will ask for a second keypress later.
  
  This example cannot simply be modified to use SPI1 or SPI2 instead of SPI0 because ActiveLowChipSelect is
  hardcoded to use SPI (SPI0).
  If you want to use SPI1: see DMASpi_example2.
  If you want to use SPI2: adapt example and create a new chip select class.
**/

/** Hardware setup:
 Teensy 3.1, 3.2: DOUT (11) connected to DIN (12)
 Teensy LC: DOUT (11) connected to DIN (12)
 Teensy 3.5, 3.6: DOUT (11) connected to DIN (12)
 Pin 0 is used as a chip select pin, don't connect anything there.
**/

/** buffers to send from and to receive to **/
#define DMASIZE 100
uint8_t src[DMASIZE];
volatile uint8_t dest[DMASIZE];
volatile uint8_t dest1[DMASIZE];

/** Wait for and consume a keypress over USB **/
void waitForKeyPress()
{
  Serial.println("\nPress a key to continue\n");
  while(!Serial.available());
  while(Serial.available())
  {
    Serial.read();
  }
}

void dumpBuffer(const volatile uint8_t* buf, const char* prefix)
{
  Serial.print(prefix);
  for (size_t i = 0; i < DMASIZE; i++)
  {
    Serial.printf("0x%02x ", buf[i]);
  }
  Serial.print('\n');
}
/** Compare the buffers and print the destination contents if there's a mismatch **/
void compareBuffers(const uint8_t* src_, const uint8_t* dest_)
{
  int n = memcmp((const void*)src_, (const void*)dest_, DMASIZE);
  if (n == 0)
  {
    Serial.println("src and dest match");
  }
  else
  {
    Serial.println("src and dest don't match");
    dumpBuffer(src_, " src: " );
    dumpBuffer(dest_, "dest: ");
  }
}

void setSrc()
{
  for (size_t i = 0; i < DMASIZE; i++)
  {
    src[i] = i;
  }
}

void clrDest(uint8_t* dest_)
{
  memset((void*)dest_, 0x00, DMASIZE);
}

void setup()
{
  waitForKeyPress();
  Serial.println("Hi!");

  /** Prepare source and destination **/
  setSrc();
  clrDest((uint8_t*)dest);
  Serial.println("Buffers are prepared");Serial.flush();

  /** set up SPI **/
  SPISettings spiSettings;
  SPI.begin();

  // transmit 10 bytes and measure time to get a feel of how long that takes
  SPI.beginTransaction(spiSettings);
  elapsedMicros us;
  for (size_t i = 0; i < DMASIZE; i++)
  {
    dest[i] = SPI.transfer(src[i]);
  }
  uint32_t t = us;
  Serial.print("Time for non-DMA transfer: ");Serial.print(t);Serial.println("us");
  SPI.endTransaction();
  compareBuffers(src, (const uint8_t*)dest);

  waitForKeyPress();

  DMASPI0.begin();
  DMASPI0.start();


  DmaSpi::Transfer trx(nullptr, 0, nullptr);

  Serial.println("Testing src -> dest, single transfer");
  Serial.println("--------------------------------------------------");
  trx = DmaSpi::Transfer(src, DMASIZE, dest);
  clrDest((uint8_t*)dest);
  DMASPI0.registerTransfer(trx);
  while(trx.busy())
  {
  }
  Serial.println("Finished DMA transfer");
  compareBuffers(src, (const uint8_t*)dest);
  Serial.println("==================================================\n\n");


  Serial.println("Testing src -> discard, single transfer");
  Serial.println("--------------------------------------------------");
  trx = DmaSpi::Transfer(src, DMASIZE, nullptr);
  DMASPI0.registerTransfer(trx);
  while(trx.busy())
  {
  }
  Serial.println("Finished DMA transfer");
  Serial.printf("last discarded value is 0x%02x\n", DMASPI0.devNull());
  if (DMASPI0.devNull() == src[DMASIZE-1])
  {
    Serial.println("That appears to be correct");
  }
  else
  {
    Serial.printf("That appears to be wrong, it should be src[DMASIZE-1] which is 0x%02x\n", src[DMASIZE-1]);
  }
  Serial.println("==================================================\n\n");


  Serial.println("Testing 0xFF dummy data -> dest, single transfer");
  Serial.println("--------------------------------------------------");
  trx = DmaSpi::Transfer(nullptr, DMASIZE, dest, 0xFF);
  memset((void*)src, 0xFF, DMASIZE); // we need this for checking the dest buffer
  clrDest((uint8_t*)dest);
  DMASPI0.registerTransfer(trx);
  while(trx.busy())
  {
  }
  Serial.println("Finished DMA transfer");
  compareBuffers(src, (const uint8_t*)dest);
  Serial.println("==================================================\n\n");


  Serial.println("Testing multiple queued transfers");
  Serial.println("--------------------------------------------------");
  trx = DmaSpi::Transfer(src, DMASIZE, dest, 0xFF);
  setSrc();
  clrDest((uint8_t*)dest);
  clrDest((uint8_t*)dest1);
  DmaSpi::Transfer trx1(src, DMASIZE, dest1);
  DMASPI0.registerTransfer(trx);
  DMASPI0.registerTransfer(trx1);
  while(trx.busy());
  Serial.println("Finished DMA transfer");
  while(trx1.busy());
  Serial.println("Finished DMA transfer1");
  compareBuffers(src, (const uint8_t*)dest);
  compareBuffers(src, (const uint8_t*)dest1);
  Serial.println("==================================================\n\n");


  Serial.println("Testing pause and restart");
  Serial.println("--------------------------------------------------");
  clrDest((uint8_t*)dest);
  clrDest((uint8_t*)dest1);
  DMASPI0.registerTransfer(trx);
  DMASPI0.registerTransfer(trx1);
  DMASPI0.stop();
  us = elapsedMicros();
  while(!DMASPI0.stopped());
  t = us;
  while(trx.busy());
  Serial.printf("Time until stopped: %lu us\n", t);
  Serial.println("Finished DMA transfer");

  if (DMASPI0.stopped())
  {
    Serial.println("DMA SPI appears to have stopped (this is good)\nrestarting");
  }
  else
  {
    Serial.println("DMA SPI does not report stopped state, but it should. (this is bad)");
  }

  DMASPI0.start();
  while(trx1.busy());
  Serial.println("Finished DMA transfer1");
  compareBuffers(src, (const uint8_t*)dest);
  compareBuffers(src, (const uint8_t*)dest1);
  Serial.println("==================================================\n\n");


  Serial.println("Testing src -> dest, with chip select object");
  Serial.println("--------------------------------------------------");
  ActiveLowChipSelect cs(0, SPISettings());
//  DebugChipSelect cs;
  trx = DmaSpi::Transfer(src, DMASIZE, dest, 0, &cs);
  clrDest((uint8_t*)dest);
  DMASPI0.registerTransfer(trx);
  while(trx.busy())
  {
  }
  Serial.println("Finished DMA transfer");
  compareBuffers(src, (const uint8_t*)dest);
  Serial.println("==================================================\n\n");


  DmaSpi::Transfer(src, DMASIZE, dest, 0, &cs);

  if (DMASPI0.stopped())
  {
    Serial.println("DMA SPI stopped.");
  }
  else
  {
    Serial.println("DMA SPI is still running");
  }
  DMASPI0.stop();
  if (DMASPI0.stopped())
  {
    Serial.println("DMA SPI stopped.");
  }
  else
  {
    Serial.println("DMA SPI is still running");
  }

  DMASPI0.end();
  SPI.end();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  digitalWriteFast(LED_BUILTIN, true);
  delay(500);
  digitalWriteFast(LED_BUILTIN, false);
  delay(500);
}
