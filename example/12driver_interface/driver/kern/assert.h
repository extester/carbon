/*
 *  Kernel library
 *  Kernel assertion
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.03.2016 13:01:41
 *      Initial revision.
 */

#ifndef __KERN_ASSERT_H_INCLUDED__
#define __KERN_ASSERT_H_INCLUDED__

#include <linux/kernel.h>

#if DEBUG
#define shell_kassert(__b)			\
	do {  \
		if ( !(__b) )  {  \
			printk(KERN_EMERG "Assertion failed %s:%d: "	\
			       #__b "\n", __FILE__, __LINE__);  \
			BUG();   \
		}  \
	} while(0)
#else /* DEBUG */
#define shell_kassert(args...)			do {} while(0)
#endif /* DEBUG */

#endif /* __KERN_ASSERT_H_INCLUDED__ */
