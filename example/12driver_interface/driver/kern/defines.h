/*
 *  Kernel library
 *  Library wide defines
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.03.2016 14:33:56
 *      Initial revision.
 */

#ifndef __KERN_DEFINES_H_INCLUDED__
#define __KERN_DEFINES_H_INCLUDED__

#ifndef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#endif /* MAX */
#ifndef MIN
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif /* MIN */

#define BETWEEN(__p, __min, __max)		((__p) >= (__min) && (__p) <= (__max))
#define MEMBER_SIZE(type, member) 		sizeof(((type *)0)->member)

#ifndef offsetof
#define offsetof(type, member)			((size_t) &((type*)0)->member)
#endif /* offsetof */

#define bzero(__p, __length)			memset((__p), 0, (__length))
#define bzero_object(__obj)				bzero(&(__obj), sizeof(__obj))
#define nresult_join(__nresult, __nr)	(__nresult) = (__nresult) == 0 ? (__nr) : (__nresult)


#endif /* __KERN_DEFINES_H_INCLUDED__ */
