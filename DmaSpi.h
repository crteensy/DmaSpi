#ifndef DMASPI_H
#define DMASPI_H

#include <Arduino.h>
#include <util/atomic.h>

#if(!defined(__arm__) && defined(TEENSYDUINO))
  #error This library is for teensyduino 1.20 on Teensy 3.0 and 3.1 only.
#endif

#include <SPI.h>
#include "DMAChannel.h"
#include "ChipSelect.h"

/** \brief The main DMA SPI driver class
**/
class DmaSpi0
{
  public:
    /** \brief describes an SPI transfer
     *
     * Transfers are kept in a queue (intrusive linked list) until they are processed by the DmaSpi driver.
     *
    **/
    class Transfer
    {
      public:
        /** \brief The Transfer's current state.
        *
        **/
        enum State
        {
          idle, /**< The Transfer is idle, the DmaSpi has not seen it yet. **/
          eDone, /**< The Transfer is done. **/
          pending, /**< Queued, but not handled yet. **/
          inProgress, /**< The DmaSpi driver is currently busy executing this Transfer. **/
          error /**< An error occured. **/
        };

        /** \brief Creates a Transfer object.
        * \param pSource pointer to the data source. If this is nullptr, the fill value is used instead.
        * \param transferCount the number of SPI transfers to perform.
        * \param pDest pointer to the data sink. If this is nullptr, data received from the slave will be discarded.
        * \param fill if pSource is nullptr, this value is sent to the slave instead.
        * \param cs pointer to a chip select object.
        *   If not nullptr, cs->select() is called when the Transfer is started and cs->deselect() is called when the Transfer is finished.
        **/
        Transfer(const uint8_t* pSource = nullptr,
                    const uint16_t& transferCount = 0,
                    volatile uint8_t* pDest = nullptr,
                    const uint8_t& fill = 0,
                    AbstractChipSelect* cs = nullptr
        ) : m_state(State::idle),
          m_pSource(pSource),
          m_transferCount(transferCount),
          m_pDest(pDest),
          m_fill(fill),
          m_pNext(nullptr),
          m_pSelect(cs)
        {
//          Serial.printf("Transfer @ %p\n", this);
        };

        /** \brief Check if the Transfer is busy, i.e. may not be modified.
        **/
        bool busy() const {return ((m_state == State::pending) || (m_state == State::inProgress) || (m_state == State::error));}

        /** \brief Check if the Transfer is done.
        **/
        bool done() const {return (m_state == State::eDone);}

//      private:
        volatile State m_state;
        const uint8_t* m_pSource;
        uint16_t m_transferCount;
        volatile uint8_t* m_pDest;
        uint8_t m_fill;
        Transfer* m_pNext;
        AbstractChipSelect* m_pSelect;
    };

    /** \brief arduino-style initialization.
     *
     * During initialization, two DMA channels are allocated. If that fails, this function returns false.
     * If the channels could be allocated, those DMA channel fields that don't change during DMA SPI operation
     * are initialized to the values they will have at runtime.
     *
     * \return true if initialization was successful; false otherwise.
     * \see end()
    **/
    static bool begin()
    {
//      Serial.println("DmaSpi::begin() : ");
      // create DMA channels, might fail
      if (!createDmaChannels())
      {
//        Serial.println("could not create DMA channels");
        return false;
      }
      state_ = eStopped;
      // tx: known destination (SPI), no interrupt, finish silently
      txChannel_()->destination((volatile uint8_t&)SPI0_PUSHR);
      txChannel_()->disableOnCompletion();
      txChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
      if (txChannel_()->error())
      {
        destroyDmaChannels();
//        Serial.println("tx channel error");
        return false;
      }

      // rx: known source (SPI), interrupt on completion
      rxChannel_()->source((volatile uint8_t&)SPI0_POPR);
      rxChannel_()->disableOnCompletion();
      rxChannel_()->triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_RX);
      rxChannel_()->attachInterrupt(rxIsr_);
      rxChannel_()->interruptAtCompletion();
      if (rxChannel_()->error())
      {
        destroyDmaChannels();
//        Serial.println("rx channel error");
        return false;
      }

      return true;
    }

    /** \brief Allow the DMA SPI to start handling Transfers. This must be called after begin().
     * \see running()
     * \see busy()
     * \see stop()
     * \see stopping()
     * \see stopped()
    **/
    static void start()
    {
//      Serial.print("DmaSpi::start() : state_ = ");
      switch(state_)
      {
        case eStopped:
//          Serial.println("eStopped");
          state_ = eRunning;
          beginPendingTransfer();
          break;

        case eRunning:
//          Serial.println("eRunning");
          break;

        case eStopping:
//          Serial.println("eStopping");
          state_ = eRunning;
          break;

        default:
//          Serial.println("unknown");
          state_ = eError;
          break;
      }
    }

    /** \brief check if the DMA SPI is in running state.
     * \return true if the DMA SPI is in running state, false otherwise.
     * \see start()
     * \see busy()
     * \see stop()
     * \see stopping()
     * \see stopped()
    **/
    static bool running() {return state_ == eRunning;}

    /** \brief register a Transfer to be handled by the DMA SPI.
     * \return false if the Transfer had an invalid transfer count (zero or greater than 32767), true otherwise.
     * \post the Transfer state is Transfer::State::pending, or Transfer::State::error if the transfer count was invalid.
    **/
    static bool registerTransfer(Transfer& transfer)
    {
//      Serial.printf("DmaSpi::registerTransfer(%p)\n", &transfer);
      if ((transfer.busy())
       || (transfer.m_transferCount == 0) // no zero length transfers allowed
       || (transfer.m_transferCount >= 0x8000)) // max CITER/BITER count with ELINK = 0 is 0x7FFF, so reject
      {
//        Serial.printf("  Transfer is busy or invalid, dropped\n");
        transfer.m_state = Transfer::State::error;
        return false;
      }
      addTransferToQueue(transfer);
      if ((state_ == eRunning) && (!busy()))
      {
//        Serial.printf("  starting transfer\n");
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
          beginPendingTransfer();
        }
      }
      return true;
    }

    /** \brief Check if the DMA SPI is busy, which means that it is currently handling a Transfer.
     \return true if a Transfer is being handled.
     * \see start()
     * \see running()
     * \see stop()
     * \see stopping()
     * \see stopped()
    **/
    static bool busy()
    {
      return (m_pCurrentTransfer != nullptr);
    }

    /** \brief Request the DMA SPI to stop handling Transfers.
     *
     * The stopping driver may finish a current Transfer, but it will then not start a new, pending one.
     * \see start()
     * \see running()
     * \see busy()
     * \see stopping()
     * \see stopped()
    **/
    static void stop()
    {
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {
        switch(state_)
        {
          case eStopped:
            break;
          case eRunning:
            if (busy())
            {
              state_ = eStopping;
            }
            else
            {
              // this means that the DMA SPI simply has nothing to do
              state_ = eStopped;
            }
            break;
          case eStopping:
            break;
          default:
            state_ = eError;
            break;
        }
      }
    }

    /** \brief See if the DMA SPI is currently switching from running to stopped state
     * \return true if the DMA SPI is switching from running to stopped state
     * \see start()
     * \see running()
     * \see busy()
     * \see stop()
     * \see stopped()
    **/
    static bool stopping() { return (state_ == eStopping); }

    /** \brief See if the DMA SPI is stopped
    * \return true if the DMA SPI is in stopped state, i.e. not handling pending Transfers
     * \see start()
     * \see running()
     * \see busy()
     * \see stop()
     * \see stopping()
    **/
    static bool stopped() { return (state_ == eStopped); }

    /** \brief Shut down the DMA SPI
     *
     * Deallocates DMA channels and sets the internal state to error (this might not be an intelligent name for that)
     * \see begin()
    **/
    static void end()
    {
      destroyDmaChannels();
      state_ = eError;
    }

    /** \brief get the last value that was read from a slave, but discarded because the Transfer didn't specify a sink
    **/
    static uint8_t devNull()
    {
      return m_devNull;
    }

  private:
    enum EState
    {
      eStopped,
      eRunning,
      eStopping,
      eError
    };

    static void addTransferToQueue(Transfer& transfer)
    {
      transfer.m_state = Transfer::State::pending;
      transfer.m_pNext = nullptr;
//      Serial.println("  DmaSpi::addTransferToQueue() : queueing transfer");
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
      {

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

    static void finishCurrentTransfer()
    {
      if (m_pCurrentTransfer->m_pSelect != nullptr)
      {
        m_pCurrentTransfer->m_pSelect->deselect();
      }
      m_pCurrentTransfer->m_state = Transfer::State::eDone;
//      Serial.printf("  finishCurrentTransfer() @ %p\n", m_pCurrentTransfer);
      m_pCurrentTransfer = nullptr;
      disableSpiDmaRequests();
    }

    static void enableSpiDmaRequests()
    {
      SPI0_SR = 0xFF0F0000;
      SPI0_RSER = SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS | SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;
    }

    static void disableSpiDmaRequests()
    {
      SPI0_RSER = 0;
      SPI0_SR = 0xFF0F0000;
    }

    static bool createDmaChannels()
    {
      if (txChannel_() == nullptr)
      {
        return false;
      }
      if (rxChannel_() == nullptr)
      {
        delete txChannel_();
        return false;
      }
      return true;
    }

    static void destroyDmaChannels()
    {
      if (rxChannel_() != nullptr)
      {
        delete rxChannel_();
      }
      if (txChannel_() != nullptr)
      {
        delete txChannel_();
      }
    }

    static DMAChannel* rxChannel_()
    {
      static DMAChannel* pChannel = new DMAChannel();
      return pChannel;
    }

    static DMAChannel* txChannel_()
    {
      static DMAChannel* pChannel = new DMAChannel();
      return pChannel;
    }

    static void rxIsr_()
    {
//      Serial.print("DmaSpi::rxIsr_()\n");
      rxChannel_()->clearInterrupt();
      // end current transfer: deselect and mark as done
      finishCurrentTransfer();

//      Serial.print("  state = ");
      switch(state_)
      {
        case eStopped: // this should not happen!
//        Serial.println("eStopped");
          state_ = eError;
          break;
        case eRunning:
//          Serial.println("eRunning");
          beginPendingTransfer();
          break;
        case eStopping:
//          Serial.println("eStopping");
          state_ = eStopped;
          break;
        case eError:
//          Serial.println("eError");
          break;
        default:
//          Serial.println("eUnknown");
          state_ = eError;
          break;
      }
    }

    static void beginPendingTransfer()
    {
      if (m_pNextTransfer == nullptr)
      {
//        Serial.println("DmaSpi::beginNextTransfer: no pending transfer"); Serial.flush();
        return;
      }

      m_pCurrentTransfer = m_pNextTransfer;
//      Serial.printf("DmaSpi::beginNextTransfer: starting transfer @ %p\n", m_pCurrentTransfer); Serial.flush();
      m_pCurrentTransfer->m_state = Transfer::State::inProgress;
      m_pNextTransfer = m_pNextTransfer->m_pNext;
      if (m_pNextTransfer == nullptr)
      {
//        Serial.println("  this was the last in the queue"); Serial.flush();
        m_pLastTransfer = nullptr;
      }

      enableSpiDmaRequests();
      // Select Chip
      if (m_pCurrentTransfer->m_pSelect != nullptr)
      {
        m_pCurrentTransfer->m_pSelect->select();
      }

      // configure Rx DMA
      if (m_pCurrentTransfer->m_pDest != nullptr)
      {
        // real data sink
//        Serial.println("  real sink");
        rxChannel_()->destinationBuffer(m_pCurrentTransfer->m_pDest,
                                        m_pCurrentTransfer->m_transferCount);
      }
      else
      {
        // dummy data sink
//        Serial.println("  dummy sink");
        rxChannel_()->destination(m_devNull);
        rxChannel_()->transferCount(m_pCurrentTransfer->m_transferCount);
      }
      rxChannel_()->enable();

      // configure Tx DMA
      if (m_pCurrentTransfer->m_pSource != nullptr)
      {
        // real data source
//        Serial.println("  real source");
        txChannel_()->sourceBuffer(m_pCurrentTransfer->m_pSource,
                                   m_pCurrentTransfer->m_transferCount);
      }
      else
      {
        // dummy data source
//        Serial.println("  dummy source");
        txChannel_()->source(m_pCurrentTransfer->m_fill);
        txChannel_()->transferCount(m_pCurrentTransfer->m_transferCount);
      }
      txChannel_()->enable();
    }

    static volatile EState state_;
    static Transfer* volatile m_pCurrentTransfer;
    static Transfer* volatile m_pNextTransfer;
    static Transfer* volatile m_pLastTransfer;
    static volatile uint8_t m_devNull;
};

extern DmaSpi0 DMASPI0;

#endif // DMASPI_H
