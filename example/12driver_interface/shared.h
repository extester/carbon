/*
 *	Carbon Framework Examples
 *  User/Kernel spaces shared definitions
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 27.01.2017 18:07:15
 *      Initial revision.
 */

#ifndef __EXAMPLE_SHARED_H_INCLUDED__
#define __EXAMPLE_SHARED_H_INCLUDED__

#ifdef __KERNEL__
#include  <linux/types.h>
#include "kern/types.h"
#else /* __KERNEL__ */
#include <sys/ioctl.h>
#include "shell/types.h"
#endif /* __KERNEL__ */


#define EX_DEVICE_NAME					"example"

/*
 * Message ident
 */

#define EX_MESSAGE_ID					0x7856

/*
 * Message format
 */
typedef struct
{
	uint16_t		ident;				/* Identificator, EX_MESSAGE_ID */
	uint32_t		data;				/* Payload data */
} __attribute__ ((packed)) ex_message_t;

/*
 * Driver IOCTL functions
 */

#define EX_IOC_MAGIC					'e'

#define EX_IOC_INTERVAL					_IOW(EX_IOC_MAGIC, 0, int)

#endif /* __EXAMPLE_SHARED_H_INCLUDED__ */
