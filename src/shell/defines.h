/*
 *  Shell library
 *  Library wide definitions
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013 15:28:37
 *      Initial revision.
 *
 *  Revision 1.1, 26.01.2017 18:28:16
 *  	Added UNUSED macro.
 *
 *  Revision 1.2, 09.01.2018 10:26:57
 *  	Added 'interface' macro.
 */

#ifndef __SHELL_DEFINES_H_INCLUDED__
#define __SHELL_DEFINES_H_INCLUDED__


#ifndef MAX
#define MAX(a,b)    					((a) > (b) ? (a) : (b))
#endif /* MAX */
#ifndef MIN
#define MIN(a,b)    					((a) < (b) ? (a) : (b))
#endif /* MIN */

#define ABS(__n)						((__n) >= 0 ? (__n) : (-(__n)) )

#define UNUSED(__expr)					((void)(__expr))

#define BETWEEN(__p, __min, __max)		((__p) >= (__min) && (__p) <= (__max))
#define ARRAY_SIZE(__a)         		(sizeof(__a)/(sizeof((__a)[0])))
#define MEMBER_SIZE(type, member) 		sizeof(((type *)0)->member)

#define A(__pointer)					((unatural_t)(__pointer))

#ifndef offsetof
#define offsetof(type, member)			((size_t) &((type*)0)->member)
#endif /* offsetof */

#define SAFE_DELETE(__p)	\
	do{ if ( (__p) != 0 ) delete __p;  (__p) = 0; }while(0)

#define SAFE_REFERENCE(__p)	\
	do{ if ( (__p) != 0 ) (__p)->reference(); }while(0)

#define SAFE_RELEASE(__p)	\
	do{ if ( (__p) != 0 ) (__p)->release();  (__p) = 0; }while(0)


#ifndef PAGE_SIZE
#define PAGE_SIZE		4096
#endif /* PAGE_SIZE */

#define ALIGN(__value, __n)				( ((__value)+(__n)-1) & (~((__n)-1)) )
#define PAGE_ALIGN(__value)				ALIGN(__value, PAGE_SIZE)

#define interface 		class

#define get_bit(__value, __bit)			( ((__value)>>(__bit))&1 )
#define set_bit(__value, __bit)			( (__value) |= (1<<(__bit)) )
#define clear_bit(__value, __bit)		( (__value) &= (~(1<<(__bit))) )

#define IS_NETWORK_CONNECTION_CLOSED(__nr)      \
    ((__nr) == ENETDOWN || (__nr) == ENETRESET || (__nr) == ECONNRESET || \
    (__nr) == ENOTCONN || (__nr) == ESHUTDOWN || (__nr) == EPIPE)

#define DAYSEC		(60*60*24)


#endif /* __SHELL_DEFINES_H_INCLUDED__ */
