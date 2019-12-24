/*
 *  Kernel library
 *  Library wide Types
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.03.2016 16:59:46
 *      Initial revision.
 */

#ifndef __KERN_LOGGER_H_INCLUDED__
#define __KERN_LOGGER_H_INCLUDED__

#include <linux/kernel.h>

#define klog_info(args...)			printk(KERN_INFO args)
#define klog_warning(args...)		printk(KERN_WARNING args)
#define klog_error(args...)			printk(KERN_ERR args)

#if DEBUG
#define klog_debug(args...)			printk(KERN_DEBUG args)
#else /* DEBUG */
#define klog_debug(args...)			do {} while(0)
#endif /* DEBUG */

#endif /* __KERN_LOGGER_H_INCLUDED__ */
