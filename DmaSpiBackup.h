#ifndef DMASPIBACKUP_H
#define DMASPIBACKUP_H

struct SpiBackup
{
  SpiBackup() = default;

  void create()
  {
    MCR = SPI0_MCR;
    CTAR0 = SPI0_CTAR0;
    CTAR1 = SPI0_CTAR1;
    RSER = SPI0_RSER;
  }
  uint32_t MCR;
  uint32_t CTAR0;
  uint32_t CTAR1;
  uint32_t RSER;

  void restore()
  {
    // halt SPI, clear all mode bits
    SPI0_MCR = SPI_MCR_HALT;
    // clear all pending flags
    SPI0_SR = 0xFFFFFFFF;
    // restore settings
    SPI0_RSER = RSER;
    SPI0_CTAR0 = CTAR0;
    SPI0_CTAR1 = CTAR1;
    // clear
    SPI0_MCR = MCR | SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF;
  }
};

#endif // DMASPIBACKUP_H

