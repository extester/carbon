/*
 *  Shell library
 *  Console output functions (embedded version)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 23.03.2017 10:44::44
 *      Initial revision.
 *
 */

#ifndef __SHELL_CONSOLE_OUTPUT_H_INCLUDED__
#define __SHELL_CONSOLE_OUTPUT_H_INCLUDED__

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int console_putc(char c);
extern void console_printf(const char* strFormat, ...);
extern void console_vprintf(const char* strFormat, va_list args);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __SHELL_CONSOLE_OUTPUT_H_INCLUDED__ */
