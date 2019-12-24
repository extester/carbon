/*
 *	Carbon Framework Examples
 *	Linux driver for example communication module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 27.01.2017 18:39:36
 *      Initial revision.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/timer.h>

#define DEBUG 		1

#include "kern/logger.h"
#include "kern/defines.h"
#include "kern/assert.h"
#include "../shared.h"

#define EX_CLASS_NAME				EX_DEVICE_NAME
#define EX_MESSAGE_INTERVAL			5 /* in seconds */

/*
 * A driver data
 */
typedef struct
{
	boolean_t				bOpen;				/* Open flag */
	struct spinlock			lock;				/* Access synchronisation */
	wait_queue_head_t		event;				/* Kernel notification event */
	struct list_head		messageList;		/* Messages (for user space) */
	struct timer_list		timer;				/* Message generation timer */
	int 					interval;			/* Message interval, seconds */
} ex_device_t;

static ex_device_t			exDevice;
static int					drvMajor = 0;
static struct class*		pDeviceClass = NULL;
static struct device*		pDevice = NULL;

typedef struct
{
	struct list_head			list;
	ex_message_t				message;
} __attribute__ ((packed)) 	ex_message_item_t ;


/*
 * Allocate an message container
 *
 * Return: allocated container address or NULL on error
 */
static ex_message_item_t* alloc_message_item(void)
{
	ex_message_item_t*	pMessageItem;

	pMessageItem = (ex_message_item_t*)kmalloc(sizeof(ex_message_item_t), GFP_KERNEL);
	if ( !pMessageItem )  {
		klog_error("[rf] failed to allocate notify item\n");
		return NULL;
	}

	bzero(pMessageItem, sizeof(*pMessageItem));
	INIT_LIST_HEAD(&pMessageItem->list);
	return pMessageItem;
}

/*
 * Queue message
 *
 * 		pMessageItem		message container to queue
 */
static void queue_message(ex_message_item_t* pMessageItem)
{
	unsigned long 	flags;

	spin_lock_irqsave(&exDevice.lock, flags);
	list_add(&pMessageItem->list, &exDevice.messageList);
	spin_unlock_irqrestore(&exDevice.lock, flags);
	wake_up_interruptible(&exDevice.event);
}

/*
 * Delete all outstanding messages
 */
static void clear_queue(void)
{
	unsigned long 	flags;

	spin_lock_irqsave(&exDevice.lock, flags);

	while ( !list_empty(&exDevice.messageList) ) {
		ex_message_item_t*	pMessageItem;

		pMessageItem = list_first_entry(&exDevice.messageList, ex_message_item_t, list);
		list_del(&pMessageItem->list);

		spin_unlock_irqrestore(&exDevice.lock, flags);
		kfree(pMessageItem);
		spin_lock_irqsave(&exDevice.lock, flags);
	}

	spin_unlock_irqrestore(&exDevice.lock, flags);
}

/*
 * Delete message generation timer
 */
static void timer_delete(void)
{
	del_timer(&exDevice.timer);
	init_timer(&exDevice.timer);
}

/*
 * Restart message generation timer
 */
static void timer_restart(void)
{
	exDevice.timer.expires = jiffies + msecs_to_jiffies(exDevice.interval*1000);
	add_timer(&exDevice.timer);
}

/*
 * Message generation timer function
 */
static void timer_callback(unsigned long data)
{
	static uint32_t		n = 1;
	ex_message_item_t*	pMessageItem;

	(void)data;
	klog_info("[ex_dev] timer callback\n");

	pMessageItem = alloc_message_item();
	if ( pMessageItem )  {
		pMessageItem->message.ident = EX_MESSAGE_ID;
		pMessageItem->message.data = n++;

		queue_message(pMessageItem);
	}

	timer_restart();
}

/*
 * Start message generation timer
 */
static void timer_start(void)
{
	timer_delete();

	exDevice.timer.function = timer_callback;
	timer_restart();
}

/*******************************************************************************/

/*
 * [Driver function]
 *
 * Open device (exclusive mode)
 */
static int ex_drv_open(struct inode* pInode, struct file* pFile)
{
	unsigned long 	flags;
	int				retVal;

	spin_lock_irqsave(&exDevice.lock, flags);
	if ( !exDevice.bOpen ) {
		exDevice.bOpen = TRUE;
		retVal = 0;

		klog_info("[ex_dev] device '%s' open successful\n", EX_DEVICE_NAME);
		timer_start();
	}
	else {
		klog_error("[ex_dev] device '%s' is already open\n", EX_DEVICE_NAME);
		retVal = -EBUSY;
	}

	spin_unlock_irqrestore(&exDevice.lock, flags);
	return retVal;
}

/*
 * [Driver function]
 *
 * Close device and release resources
 */
static int ex_drv_release(struct inode* pInode, struct file* pFile)
{
	unsigned long flags;

	klog_info("[ex_dev] closing device '%s'\n", EX_DEVICE_NAME);

	timer_delete();
	spin_lock_irqsave(&exDevice.lock, flags);
	shell_kassert(exDevice.bOpen);
	exDevice.bOpen = FALSE;
	spin_unlock_irqrestore(&exDevice.lock, flags);

	clear_queue();
	return 0;
}

/*
 * [Driver function]
 *
 * Read data
 */
static ssize_t ex_drv_read(struct file* pFile, char* pBuffer, size_t size, loff_t* pOffset)
{
	unsigned long 		flags;
	ex_message_item_t*	pMessageItem;
	ssize_t			retVal;
	int 			res;
	size_t			rsize;

	spin_lock_irqsave(&exDevice.lock, flags);
	if ( !exDevice.bOpen )  {
		spin_unlock_irqrestore(&exDevice.lock, flags);
		return -EINVAL;
	}

	retVal = -EAGAIN;
	pMessageItem = list_first_entry_or_null(&exDevice.messageList, ex_message_item_t, list);
	if ( !pMessageItem )  {
		if ( (pFile->f_flags&O_NONBLOCK) == 0 ) {
			spin_unlock_irqrestore(&exDevice.lock, flags);
			retVal = wait_event_interruptible(exDevice.event, !list_empty(&exDevice.messageList));
			spin_lock_irqsave(&exDevice.lock, flags);
			if ( retVal == 0 )  {
				pMessageItem = list_first_entry_or_null(&exDevice.messageList, ex_message_item_t, list);
			}
		}
	}

	if ( pMessageItem ) {
		list_del(&pMessageItem->list);
		spin_unlock_irqrestore(&exDevice.lock, flags);

		rsize = sizeof(ex_message_t);
		rsize = MIN(rsize, size);

		res = (int)copy_to_user(pBuffer, &pMessageItem->message, rsize);
		kfree(pMessageItem);
		if ( res == 0 ) {
			retVal = rsize;
		}
		else {
			klog_error("[rfm] copy to user failed, result: %d\n", -res);
			retVal = res;
		}
	}
	else {
		spin_unlock_irqrestore(&exDevice.lock, flags);
	}

	return retVal;
}

/*
 * [Driver function]
 *
 * Write data (unsupported)
 */
static ssize_t ex_drv_write(struct file* pFile, const char* pBuffer, size_t size, loff_t* pOffset)
{
	return -ENOSYS;
}

/*
 * [Driver function]
 *
 * Execute specific driver function
 */
static long ex_drv_ioctl(struct file* pFile, unsigned int cmd, unsigned long arg)
{
	void __user*			pUser = (void __user*)arg;
	int 					retVal = 0;

	if ( _IOC_TYPE(cmd) != EX_IOC_MAGIC )  {
		return -ENOTTY;
	}

	if ( _IOC_DIR(cmd)&_IOC_READ )  {
		retVal = !access_ok(VERIFY_WRITE, pUser, _IOC_SIZE(cmd));
	}
	if ( retVal == 0 && (_IOC_DIR(cmd)&_IOC_WRITE) ) {
		retVal = !access_ok(VERIFY_READ, pUser, _IOC_SIZE(cmd));
	}
	if ( retVal )  {
		return -EFAULT;
	}

	switch ( cmd )  {
		case EX_IOC_INTERVAL:
			exDevice.interval =(uint32_t)arg;
			timer_delete();
			timer_restart();
			retVal = 0;
			break;

		default:
			retVal = -ENOTTY;
			break;
	}

	return retVal;
}

/*
 * Driver functions
 */
static struct file_operations rfm_fops = {
		.owner = THIS_MODULE,
		.open = ex_drv_open,
		.release = ex_drv_release,
		.read = ex_drv_read,
		.write = ex_drv_write,
		.unlocked_ioctl = ex_drv_ioctl
};

/*
 * Driver entry point
 */
static int __init ex_init(void)
{
	bzero_object(exDevice);
	spin_lock_init(&exDevice.lock);
	INIT_LIST_HEAD(&exDevice.messageList);
	init_waitqueue_head(&exDevice.event);
	exDevice.interval = EX_MESSAGE_INTERVAL;

	klog_info("[ex_dev] Loading driver '%s', version 1.0\n", EX_DEVICE_NAME);

	/* Try to dynamically allocate a major number for the device */
	drvMajor = register_chrdev(0, EX_DEVICE_NAME, &rfm_fops);
	if ( drvMajor < 0 ) {
		klog_error("[ex_dev] failed to register a major number, result %d\n", -drvMajor);
		return drvMajor;
	}

	/* Register the device class */
	pDeviceClass = class_create(THIS_MODULE, EX_CLASS_NAME);
	if ( IS_ERR(pDeviceClass) ) {
		unregister_chrdev(drvMajor, EX_DEVICE_NAME);
		klog_error("[ex_dev] failed to register device class\n");
		return PTR_ERR(pDeviceClass);
	}

	/* Register the device driver */
	pDevice = device_create(pDeviceClass, NULL, MKDEV(drvMajor, 0), NULL, EX_DEVICE_NAME);
	if ( IS_ERR(pDevice) ) {
		class_destroy(pDeviceClass);
		unregister_chrdev(drvMajor, EX_DEVICE_NAME);
		klog_error("[ex_dev] Failed to create the device\n");
		return PTR_ERR(pDeviceClass);
	}

	return ESUCCESS;
}

/*
 * Driver exit point
 */
static void __exit ex_exit(void)
{
	klog_info("[ex_dev] Unloading driver '%s'\n", EX_DEVICE_NAME);

	device_destroy(pDeviceClass, MKDEV(drvMajor, 0));
	class_unregister(pDeviceClass);
	class_destroy(pDeviceClass);
	unregister_chrdev(drvMajor, EX_DEVICE_NAME);
}

module_init(ex_init);
module_exit(ex_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Softland");
MODULE_DESCRIPTION("Carbon Example Driver");
