#include "DmaSpi.h"

#if defined(__MK66FX1M0__)
DmaSpi0 DMASPI0;
#elif defined(KINETISK)
DmaSpi0 DMASPI0;
#elif defined (KINETISL)
DmaSpi0 DMASPI0;
DmaSpi1 DMASPI1;
#endif // defined
