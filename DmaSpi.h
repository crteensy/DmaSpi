#ifndef DMASPI_H
#define DMASPI_H

#include <stdint.h>
#include <util/atomic.h>

#include <WProgram.h>

#include "ChipSelect.h"

#define DMASPI0_TXCHAN 1
#define DMASPI0_RXCHAN 0
#define MAKE_DMA_CHAN_ISR(n) void dma_ch ## n ## _isr()
#define DMA_CHAN_ISR(n)  MAKE_DMA_CHAN_ISR(n)

class DmaSpi0
{
  public:
    class Transfer
    {
      public:
        enum State
        {
          idle,
          done,
          pending,
          inProgress
        };
        Transfer(const uint8_t* pSource = nullptr,
                    const uint16_t& size = 0,
                    volatile uint8_t* pDest = nullptr,
                    const uint8_t& fill = 0,
                    AbstractChipSelect* cb = nullptr
        ) : m_state(State::idle),
        m_pSource(pSource),
        m_size(size),
        m_pDest(pDest),
        m_fill(fill),
        m_pNext(nullptr),
        m_pSelect(cb) {};
        bool busy() const {return ((m_state == State::pending) || (m_state == State::inProgress));}
    //  private:
        volatile State m_state;
        const uint8_t* m_pSource;
        uint16_t m_size;
        volatile uint8_t* m_pDest;
        uint8_t m_fill;
        Transfer* m_pNext;
        AbstractChipSelect* m_pSelect;
    };

    static bool registerTransfer(Transfer& transfer)
    {
      if ((transfer.busy())
       || (transfer.m_size == 0) // no zero length transfers allowed
       || (transfer.m_size >= 0x8000)) // max CITER/BITER count with ELINK = 0 is 0x7FFF, so reject
      {
        return false;
      }
      transfer.m_state = Transfer::State::pending;
      transfer.m_pNext = nullptr;
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        if (m_pCurrentTransfer == nullptr)
        {
          /** no pending transfer **/
          m_pNextTransfer = &transfer;
          m_pLastTransfer = &transfer;
          beginNextTransfer();
        }
        else
        {
          /** add to list of pending transfers **/
          if (m_pNextTransfer == nullptr)
          {
            m_pNextTransfer = &transfer;
          }
          else
          {
            m_pLastTransfer->m_pNext = &transfer;
          }
          m_pLastTransfer = &transfer;
        }
      }
      return true;
    }

    static void begin()
    {
      // configure SPI0
      SIM_SCGC6 |= SIM_SCGC6_SPI0;

      // configure SPI pins: SCK, MOSI, CS0 as outputs, mux(2)
      pinMode(13, OUTPUT); // SCK
      PORTC_PCR5 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(2);
      pinMode(11, OUTPUT); // MOSI
      PORTC_PCR6 = PORT_PCR_SRE | PORT_PCR_DSE | PORT_PCR_MUX(2);
      pinMode(12, INPUT); // MISO
      PORTC_PCR7 = PORT_PCR_MUX(2);

      // clear status flags
      SPI0_SR = 0xFF0F0000;
      // disable requests
      SPI0_RSER = 0;
      // master mode, clear buffers
      SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_DCONF(0) | SPI_MCR_CLR_TXF | SPI_MCR_CLR_RXF;

      // configure default transfer attributes
//      SPI0_CTAR0 = SPI_CTAR_FMSZ(7) | SPI_CTAR_PBR(0) | SPI_CTAR_BR(0);

      // turn on DMAMUX and DMA
      SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
      SIM_SCGC7 |= SIM_SCGC7_DMA;

      // clear DMA error flags and channel configurations
      DMA_ERR = 0x0F;
      DMAMUX0_CHCFG1 = 0;
      DMAMUX0_CHCFG0 = 0;

      // clear Rx channel interrupt request, in case one is pending
      DMA_CINT = DMA_CINT_CINT(DMASPI0_RXCHAN);

      // enable requests
      // Tx, select SPI Tx FIFO
      DMAMUX0_CHCFG1 = DMAMUX_ENABLE | DMAMUX_SOURCE_SPI0_TX;
      // Rx, select SPI Rx FIFO
      DMAMUX0_CHCFG0 = DMAMUX_ENABLE | DMAMUX_SOURCE_SPI0_RX;

      // enable dma interrupt for rx channel
      NVIC_ENABLE_IRQ((IRQ_DMA_CH0 + DMASPI0_RXCHAN)); // double parantheses needed by macro
      // enable SPI requests (->dma)
      SPI0_RSER = SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS | SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS;

      // configure DMA mux, TX
      DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_8BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);
      DMA_TCD1_NBYTES_MLNO = 1;
      DMA_TCD1_SLAST = 0;
      DMA_TCD1_DADDR = (void*)&SPI0_PUSHR;
      DMA_TCD1_DOFF = 0;
      DMA_TCD1_DLASTSGA = 0;
      DMA_TCD1_CSR = DMA_TCD_CSR_DREQ;

      // configure DMA mux, RX
      DMA_TCD0_SADDR = (void*)&SPI0_POPR;
      DMA_TCD0_SOFF = 0;
      DMA_TCD0_ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_8BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_8BIT);
      DMA_TCD0_NBYTES_MLNO = 1;
      DMA_TCD0_SLAST = 0;
      DMA_TCD0_DLASTSGA = 0;
      DMA_TCD0_CSR = DMA_TCD_CSR_DREQ | DMA_TCD_CSR_INTMAJOR;
    }

    static bool busy()
    {
      return (m_pCurrentTransfer != nullptr);
    }

    static void pause()
    {
      pause_ = true;
    }

    static bool paused()
    {
      return (pause_ && (!busy()));
    }

    static void releaseSpi()
    {
      SPI0_RSER = 0;
    }

    static void resume()
    {
      // clear pending flags and re-enable requests
      SPI0_SR = 0xFF0F0000;
      SPI0_RSER = SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS | SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS;
      pause_ = false;
      beginNextTransfer();
    }

    static void backupSpi()
    {
    }

    static void m_rxIsr()
    {
      // transfer finished, start next one if available
      DMA_CINT = DMA_CINT_CINT(DMASPI0_RXCHAN);
      DMA_CERQ = DMA_CERQ_CERQ(DMASPI0_TXCHAN);

      /** TBD: RELEASE SPI HERE IF APP REQUESTED IT**/
      /** TBD: IF NOT, PROCEED **/

      if (m_pCurrentTransfer->m_pSelect != nullptr)
      {
        m_pCurrentTransfer->m_pSelect->deselect();
        SPI0_CTAR0 = m_ctarBackup;
      }
      m_pCurrentTransfer->m_state = Transfer::State::done;
      m_pCurrentTransfer = nullptr;
      beginNextTransfer();
    }

    static void write(const uint8_t& val)
    {
      Transfer transfer(nullptr, 1, nullptr, val, nullptr);
      registerTransfer(transfer);
      while(transfer.busy())
      ;
    }

    static uint8_t read(const uint32_t& flagsVal)
    {
      volatile uint8_t result;
      Transfer transfer(nullptr, 1, &result, flagsVal);
      registerTransfer(transfer);
      while(transfer.busy())
      ;
      return result;
    }

  private:
    static void beginNextTransfer()
    {
      if ((m_pNextTransfer == nullptr) || (pause_ == true))
      {
        /** TBD: UNLOCK SPI **/
        return;
      }
      /** TBD: Lock SPI **/

      m_pCurrentTransfer = m_pNextTransfer;
      m_pCurrentTransfer->m_state = Transfer::State::inProgress;
      m_pNextTransfer = m_pNextTransfer->m_pNext;
      if (m_pNextTransfer == nullptr)
      {
        m_pLastTransfer = nullptr;
      }

      // Select Chip
      if (m_pCurrentTransfer->m_pSelect != nullptr)
      {
        m_ctarBackup = SPI0_CTAR0;
        m_pCurrentTransfer->m_pSelect->select();
      }

      // clear SPI flags
      SPI0_SR = 0xFF0F0000;
      SPI0_MCR |= SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF;
      SPI0_TCR = 0;

      // configure Rx DMA
      if (m_pCurrentTransfer->m_pDest != nullptr)
      {
        // real data sink, offset after writing: 1
        DMA_TCD0_DADDR = (void*)(m_pCurrentTransfer->m_pDest);
        DMA_TCD0_DOFF = 1;
      }
      else
      {
        // use devNull, offset after writing: 0
        DMA_TCD0_DADDR = (void*)&m_devNull;
        DMA_TCD0_DOFF = 0;
      }
      // set transfer size
      DMA_TCD0_CITER_ELINKNO = DMA_TCD0_BITER_ELINKNO = m_pCurrentTransfer->m_size;
      // enable Rx request
      DMA_SERQ = DMASPI0_RXCHAN;

      // configure Tx DMA
      if (m_pCurrentTransfer->m_pSource != nullptr)
      {
        // real data source
        DMA_TCD1_SADDR = (void*)(m_pCurrentTransfer->m_pSource);
        DMA_TCD1_SOFF = 1;
      }
      else
      {
        // dummy source
        DMA_TCD1_SADDR = (void*)&(m_pCurrentTransfer->m_fill);
        DMA_TCD1_SOFF = 0;
      }
      // set transfer size
      DMA_TCD1_CITER_ELINKNO = DMA_TCD1_BITER_ELINKNO = m_pCurrentTransfer->m_size;
      DMA_SERQ = DMASPI0_TXCHAN;
    }

    static volatile uint32_t m_ctarBackup;
    static Transfer* volatile m_pCurrentTransfer;
    static Transfer* volatile m_pNextTransfer;
    static Transfer* volatile m_pLastTransfer;
    static volatile uint8_t m_devNull;
    static bool pause_;
};

extern DmaSpi0 DMASPI0;

#endif // DMASPI_H
