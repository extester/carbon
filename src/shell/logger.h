
#if __unix__
//#include "unix/logger.h"
#include "logger/logger_compat.h"
#elif __embed__
#include "embed/logger.h"
#endif