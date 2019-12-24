
#if __unix__
#include "unix/thread.h"
#elif __embed__
#if __tneo__
#include "embed/tneo/thread.h"
#elif __freertos__
#include "embed/freertos/thread.h"
#endif
#endif