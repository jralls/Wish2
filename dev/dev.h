#ifndef __X10_DEV_H__
#define __X10_DEV_H__

/*
 *
 * $Id: x10_dev.h,v 1.4 2004/01/01 21:34:09 whiles Exp whiles $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * X10 header for device abstraction module
 */

/*
   This module is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as publised by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This module is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A
   PARTICULAR PURPOSE.  See the GNU Library General Public License
   for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA

   Should you need to contact the author, you can do so by email to
   <wsh@sprintmail.com>.
*/

#define DEBUG

#include <linux/devfs_fs_kernel.h>

// define a standard format for messages to syslogd
#ifdef DEBUG
#ifdef dbg
#undef dbg
#endif
#define ANNOUNCE if (debug) printk(KERN_NOTICE "%s/%d/%s() called\n", __FILE__, __LINE__, __FUNCTION__)
#define dbg(format, arg...) do { if (debug) {printk(KERN_NOTICE "%s/%d/%s(): " format "\n", __FILE__, __LINE__, __FUNCTION__,## arg); }} while(0)
#else
#define dbg(format, arg...) do {} while (0)
#endif

#ifdef err
#undef err
#endif
#define err(format, arg...) printk(KERN_ERR __FILE__ "%s/%d/%s(): " format "\n", __FILE__, __LINE__, __FUNCTION__, ## arg)

#ifdef info
#undef info
#endif
#define info(format, arg...) printk(KERN_INFO "x10: " format "\n", ## arg)

#ifdef warn
#undef warn
#endif
#define warn(format, arg...) printk(KERN_WARNING "x10: " format "\n", ## arg)

#ifdef log
#undef log
#endif
#define log(format, arg...) do { if (syslogtraffic) printk(KERN_NOTICE "X10: " format "\n", ## arg); } while(0)

/* Some earlier 2.4.x kernels did not create min()/max() in kernel.h */
/* This version is overly simplified but is all we need for the x10 code */
#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MODULE_LICENSE
#define MODULE_LICENSE(a) static char __module_license=(a);
#endif

#define MAX_HOUSECODES 16
#define MAX_UNITS 16

#define X10_CONTROL_WITHOUTHDRS         0x0
#define X10_CONTROL_WITHHDRS            0x1
#define X10_CONTROL_READCHANGED         0x2

#define X10_CONTROL_HCWHDRS             0x0       /* 0 */
#define X10_CONTROL_HCWOHDRS            0x1       /* 16 */
#define X10_CONTROL_STATUS              0x2       /* 32 */
#define X10_CONTROL_DUMPLOG             0x3       /* 48 */
#define X10_CONTROL_4			0x4	/* 64 */
#define X10_CONTROL_5			0x5	/* 80 */
#define X10_CONTROL_6			0x6	/* 96 */
#define X10_CONTROL_7			0x7	/* 112 */
#define X10_CONTROL_8			0x8	/* 128 */
#define X10_CONTROL_9			0x9	/* 144 */
#define X10_CONTROL_10			0xa	/* 160 */
#define X10_CONTROL_11			0xb	/* 176 */
#define X10_CONTROL_12			0xc	/* 192 */
#define X10_CONTROL_13			0xd	/* 208 */
#define X10_CONTROL_14			0xe	/* 224 */
#define X10_CONTROL_API			0xf	/* 240 */

#define HOUSECODE(a) (((a)&0xf0)>>4)
#define UNITCODE(a) ((a)&0x0f)

typedef struct x10mqueue {
  atomic_t	head;
  atomic_t	tail;
  spinlock_t    spinlock;
  wait_queue_head_t wq;
  x10_message_t	queue[MESSAGE_QUEUE_SIZE];
} x10mqueue_t;

typedef struct x10dataio {
  atomic_t	changed;
  atomic_t	value;
} x10dataio_t;

typedef struct x10controlio {
  atomic_t	changed;
} x10controlio_t;

#endif				// #ifdef __X10_MODULE__

