#include "DmaSpi.h"

#if defined(KINETISK)
DmaSpi0 DMASPI0;
#elif defined (KINETISL)
DmaSpi0 DMASPI0;
DmaSpi1 DMASPI1;
#endif // defined
