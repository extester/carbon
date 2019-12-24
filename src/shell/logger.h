
#if __unix__
#include "unix/logger.h"
#elif __embed__
#include "embed/logger.h"
#endif