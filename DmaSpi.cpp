#include "DmaSpi.h"

DmaSpi0 DMASPI0;

void dma_ch0_isr()
{
  DmaSpi0::m_rxIsr();
}

DmaSpi0::Transfer* volatile DmaSpi0::m_pCurrentTransfer = nullptr;
DmaSpi0::Transfer* volatile DmaSpi0::m_pNextTransfer = nullptr;
DmaSpi0::Transfer* volatile DmaSpi0::m_pLastTransfer = nullptr;
volatile uint32_t DmaSpi0::m_ctarBackup;
volatile uint8_t DmaSpi0::m_devNull;
bool DmaSpi0::pause_ = false;
