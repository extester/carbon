
#if __unix__
#include "unix/lock.h"
#elif __embed__
#if __tneo__
#include "embed/tneo/lock.h"
#elif __freertos__
#include "embed/freertos/lock.h"
#endif
#endif
