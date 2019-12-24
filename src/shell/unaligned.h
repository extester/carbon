/*
 *  Shell library
 *  Unaligned data access
 *
 *  Copyright (c) 2013 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013 18:32:53
 *      Initial revision.
 */

#ifndef __SHELL_UNALIGNED_H_INCLUDED__
#define __SHELL_UNALIGNED_H_INCLUDED__

#include "shell/types.h"
#include "shell/tstring.h"

typedef struct __una_uint16 { uint16_t value __attribute__((packed)); } __una_uint16_t;
typedef struct __una_uint32 { uint32_t value __attribute__((packed)); } __una_uint32_t;
typedef struct __una_uint64 { uint64_t value __attribute__((packed)); } __una_uint64_t;

#define UNALIGNED16             			(const __una_uint16_t*)
#define UNALIGNED32             			(const __una_uint32_t*)
#define UNALIGNED64             			(const __una_uint64_t*)

#define GET_UNALIGNED16(__p)    			((UNALIGNED16((__p)))->value)
#define GET_UNALIGNED32(__p)    			((UNALIGNED32((__p)))->value)
#define GET_UNALIGNED64(__p)    			((UNALIGNED64((__p)))->value)

#define PUT_UNALIGNED16(__addr, __value)	( ((__una_uint16_t*)(__addr))->value = (__value) )
#define PUT_UNALIGNED32(__addr, __value)	( ((__una_uint32_t*)(__addr))->value = (__value) )
#define PUT_UNALIGNED64(__addr, __value)	( ((__una_uint64_t*)(__addr))->value = (__value) )

#define UNALIGNED_MEMCPY(__to, __from, __len)       _tmemmove(__to, __from, __len)

#endif /* __SHELL_UNALIGNED_H_INCLUDED__ */
