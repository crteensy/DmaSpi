#include "DmaSpi.h"

DmaSpi0 DMASPI0;
volatile DmaSpi0::EState DmaSpi0::state_ = DmaSpi0::EState::eError;
DmaSpi0::Transfer* volatile DmaSpi0::m_pCurrentTransfer = nullptr;
DmaSpi0::Transfer* volatile DmaSpi0::m_pNextTransfer = nullptr;
DmaSpi0::Transfer* volatile DmaSpi0::m_pLastTransfer = nullptr;
volatile uint8_t DmaSpi0::m_devNull;
