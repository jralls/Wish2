
/*
 *
 * $Id: x10_dev.c,v 1.4 2004/01/01 21:01:32 whiles Exp whiles $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * X10 device abstraction module
 *
 */

#define DISTRIBUTION_VERSION "X10 DEV module v2.0.0 (wsh@sprintmail.com)"

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

#define EXPORT_SYMTAB
#include <linux/config.h>

#ifdef CONFIG_SMP
#define __SMP__
#endif

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/poll.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/time.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/interrupt.h>

#include <linux/version.h>

#define __X10_MODULE__
#include "x10.h"
#include "x10_dev.h"

#define DATA_DEVICE_NAME "x10d"
#define CONTROL_DEVICE_NAME "x10c"
#define API_BUFFER 1024

/* Here are some module parameters and definitions */
int debug = 1;
MODULE_PARM (debug, "i");
MODULE_PARM_DESC (debug, "turns on debug information (default=0)");

int syslogtraffic = 0;
MODULE_PARM (syslogtraffic, "i");
MODULE_PARM_DESC (syslogtraffic, "If set to 1, all X10 traffic is written to kern.notice in the syslog (default=0)");

static int data_major = 120;
static int control_major = 121;
MODULE_PARM(data_major, "i");
MODULE_PARM_DESC(data_major, "Major character device for communicating with individual x10 units (default=120)");
MODULE_PARM(control_major, "i");
MODULE_PARM_DESC(control_major, "Major character device for communicating with raw x10 transceiver (default=121)");

#define DRIVER_VERSION "$Id: x10_dev.c,v 1.4 2004/01/01 21:01:32 whiles Exp whiles $"
char *version = DRIVER_VERSION;

static __inline__ int XMAJOR (struct file *a)
{
  return MAJOR ((a)->f_dentry->d_inode->i_rdev);
}

static __inline__ int XMINOR (struct file *a)
{
  return MINOR ((a)->f_dentry->d_inode->i_rdev);
}

/* prototypes for character device registration */
static int __init x10_init (void);
static void __exit x10_exit (void);
static int x10_open (struct inode *inode, struct file *file);
static int x10_release (struct inode *inode, struct file *file);
static unsigned int x10_poll (struct file *, poll_table *);
static ssize_t x10_read (struct file *file, char *buffer,
			      size_t length, loff_t * offset);
static ssize_t x10_write (struct file *file, const char *buffer,
			       size_t length, loff_t * offset);
static loff_t x10_llseek (struct file *file, loff_t offset, int origin);
static int x10mqueue_get(x10mqueue_t *q,x10_message_t *m);
static int x10mqueue_add(x10mqueue_t *q,__u8 type,__u8 hc, __u8 uc, __u8 cmd, __u32 f);
static ssize_t x10_api_write(x10_message_t *message,x10mqueue_t *q);

struct file_operations device_fops = {
	owner:		THIS_MODULE,
	read:		x10_read,
	write:		x10_write,
	open:		x10_open,
	poll:		x10_poll,
	release:	x10_release,
	llseek:		x10_llseek
};

struct x10api {
        // storage of system information
        int data_major;         // the target data device major
        int control_major;      // the target control device major
        int data;               // the actual data device major
        int control;            // the actual control device major

	int us_connected;	// UserSpace driver connected flag

        void *private;
};

static struct x10api x10api;

const char hex[] = "0123456789abcdef";
static char hexbuffer[256];
inline char *
dumphex (void *data, int len)
{
  int i;
  unsigned char c, *pbuf;

  pbuf = hexbuffer;
  for (i = 0; i < min (len, (int) sizeof (hexbuffer) - 6); i++) {
    c = ((unsigned char *) data)[i];
    *pbuf++ = hex[(c >> 4) & 0x0f];
    *pbuf++ = hex[c & 0xf];
    *pbuf++ = ' ';
  }
  *pbuf++ = '\0';
  return hexbuffer;
}

/*
   Initialization code:  When the module is loaded, it attempts to open the 
   serial port specified in "serial".  

   Note also that it is possible to set major_device and major_unit to a value
   of zero to have the system allocate major device numbers.  If this is done, 
   the user must look in /proc/devices to find the device numbers.
*/
int __init
x10_init (void)
{
  int res;

  x10api.data = -1;
  x10api.control = -1;
  x10api.data_major = data_major;
  x10api.control_major = control_major;
  x10api.us_connected = 0;

  info (DISTRIBUTION_VERSION);
  info ("%s", version);
  dbg ("%s", "starting initialization");

  // the private data for each device identifies the minor device info

  dbg ("registering data_major device %d for %s",x10api.data_major,DATA_DEVICE_NAME);
  res = register_chrdev (x10api.data_major, DATA_DEVICE_NAME, &device_fops);
  if (res < 0) {
    err ("unable to register major device %d for %s", x10api.data_major, DATA_DEVICE_NAME);
    x10_exit ();
    return x10api.data;
  }
  if (x10api.data_major == 0)
    x10api.data_major = x10api.data = res;
  else
    x10api.data = x10api.data_major;

  dbg ("registering control_major unit %d for %s", x10api.control_major, CONTROL_DEVICE_NAME);
  res = register_chrdev (x10api.control_major, CONTROL_DEVICE_NAME, &device_fops);
  if (res < 0) {
    err ("unable to register unit device %d for %s", x10api.control_major, CONTROL_DEVICE_NAME);
    x10_exit ();
    return res;
  }
  if (x10api.control_major == 0)
    x10api.control_major = x10api.control = res;
  else
    x10api.control = x10api.control_major;
  dbg ("%s", "X10 /dev devices registered");
  info ("X10 driver successfully loaded");
  return 0;
}

void __exit
x10_exit (void)
{
  dbg ("%s", "Unloading X10 driver");

  if (x10api.data >= 0) {
    dbg ("unregistering %d:%s", x10api.data, DATA_DEVICE_NAME);
    if (unregister_chrdev (x10api.data, DATA_DEVICE_NAME) == -EINVAL)
      dbg ("error unregistering %s", DATA_DEVICE_NAME);
  }
  if (x10api.control >= 0) {
    dbg ("unregistering %d:%s", x10api.control, CONTROL_DEVICE_NAME);
    if (unregister_chrdev (x10api.control, CONTROL_DEVICE_NAME) == -EINVAL)
      dbg ("error unregistering %s", CONTROL_DEVICE_NAME);
    }
  info ("X10 driver unloaded");
}

module_init (x10_init);
module_exit (x10_exit);

/* FOPS functions */

static int x10_open (struct inode *inode, struct file *file)
{
  int minor=XMINOR(file);
  int major=XMAJOR(file);
  int hc = HOUSECODE(minor);
  int uc = UNITCODE(minor);
  x10mqueue_t *mqueue;

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){ // userspace
    // bidirectional connection to the userspace portion of driver
    dbg("%s","bidirectional connection to userspace open");
    mqueue = kmalloc(sizeof(x10mqueue_t),GFP_KERNEL);
    file->private_data = (void *)mqueue;
    atomic_set(&mqueue->head,0);
    atomic_set(&mqueue->tail,0);
    spin_lock_init(&mqueue->spinlock);
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }

  return 0;
}

static int x10_release (struct inode *inode, struct file *file)
{
  int minor=XMINOR(file);
  int major=XMAJOR(file);
  int hc = HOUSECODE(minor);
  int uc = UNITCODE(minor);

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){
    // bidirectional connection to the userspace portion of driver
    if (x10api.us_connected != 0) {
      x10api.us_connected = 0;
      dbg("%s","bidirectional connection to userspace closed");
    }
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }
  kfree(file->private_data);
  file->private_data = NULL;
  return 0;
}

static unsigned int x10_poll (struct file *file, poll_table * pt)
{
  int minor = XMINOR (file);
  int major = XMAJOR(file);
  int hc = HOUSECODE(minor);
  int uc = UNITCODE (minor);

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){
    // bidirectional connection to the userspace portion of driver
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }
//  if (getstate (housecode, unit, 0))
//    return (POLLIN | POLLRDNORM | POLLWRNORM | POLLOUT);
//  else
//    return (POLLWRNORM | POLLOUT);
  return (POLLERR);
}

static ssize_t x10_read (struct file *file, char *ubuffer, size_t length, loff_t * offset)
{
  int minor = XMINOR (file);
  int major = XMAJOR (file);
  int hc = HOUSECODE (minor);
  int uc = UNITCODE (minor);
  ssize_t ret = 0;

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){
    x10_message_t m;
    // bidirectional connection to the userspace portion of driver
    if (length < sizeof(x10_message_t)) {
	dbg("%s","error:  Invalid message size");
	return -EFAULT;
    }
    if (x10mqueue_get((x10mqueue_t *)file->private_data,&m))
      return -EFAULT;
    if (copy_to_user(ubuffer,&m,sizeof(x10_message_t))) 
	return -EFAULT;
    return(sizeof(x10_message_t));
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }

  return ret;
}

static ssize_t x10_write (struct file *file, const char *ubuffer, size_t length, loff_t * offset)
{
  int minor = XMINOR (file);
  int major = XMAJOR (file);
  int hc = HOUSECODE (minor);
  int uc = UNITCODE (minor);
  x10_message_t message;

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){
    // bidirectional connection to the userspace portion of driver
    if (length < sizeof(message)) {
	dbg("%s","error:  Invalid message size");
	return -EFAULT;
    }
    if (copy_from_user(&message,ubuffer,sizeof(message)))
	return -EFAULT;
    return(x10_api_write(&message,(x10mqueue_t *)file->private_data));
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }

  // do something here
  return length;
}

static loff_t x10_llseek (struct file *file, loff_t offset, int origin)
{
  int minor = XMINOR (file);
  int major=XMAJOR(file);
  int hc = HOUSECODE(minor);
  int uc = UNITCODE(minor);

  ANNOUNCE;
  dbg ("origin=%d offset=%lld", origin, offset);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }

  return -ESPIPE;
}

static int x10mqueue_add(x10mqueue_t *q,__u8 type,__u8 hc, __u8 uc, __u8 cmd, __u32 f)
{
  int head,tail,ret=1;
  x10_message_t *message;

  spin_lock(&q->spinlock);

  tail = atomic_read(&q->tail);
  head = atomic_read(&q->head);
  message = &q->queue[tail];
  message->type = type;
  message->housecode = hc;
  message->unitcode = uc;
  message->command = cmd;
  message->flag = f;

  if (++tail >= MESSAGE_QUEUE_SIZE)
    tail = 0;
  if (tail != head) {
    atomic_set(&q->tail,tail);
    ret = 0;
  }
  spin_unlock(&q->spinlock);
  return ret;
}

static int x10mqueue_get(x10mqueue_t *q,x10_message_t *m)
{
  int head,tail,ret=1;
  x10_message_t *message;

  spin_lock(&q->spinlock);

  tail = atomic_read(&q->tail);
  head = atomic_read(&q->head);
  if (head != tail) {
    message = &q->queue[head];
    if (++head >= MESSAGE_QUEUE_SIZE)
      head = 0;
    atomic_set(&q->head,head);
    memcpy(m,message,sizeof(x10_message_t));
    ret = 0;
  }
  spin_unlock(&q->spinlock);
  return 1;
}

ssize_t x10_api_write(x10_message_t *message,x10mqueue_t *q)
{
  switch (message->type) {
    case X10_API:
	if (message->command == X10_CMD_STATUS) {
	  dbg("%s","X10_API connected");
	  x10api.us_connected = 1;
	  // OK...now write the status to the read queue
	  if (x10mqueue_add(q,X10_API,0,0,X10_CMD_ON,0))
	    return -EFAULT;
	  return 0;
	}
	dbg("Invalid X10_API command %x",message->command);
	break;
    case X10_DATA:
    case X10_CMD:
    default:
      dbg("Invalid message type %x",message->type);
      break;
  }
  return -EFAULT;
}


MODULE_LICENSE ("GPL");
