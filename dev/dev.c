
/*
 *
 * $Id: x10_dev.c,v 1.2 2003/11/03 00:37:13 whiles Exp whiles $
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
#include "x10_dev.h"
#include "x10.h"

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

#define DRIVER_VERSION "$Id: x10_dev.c,v 1.2 2003/11/03 00:37:13 whiles Exp whiles $"
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
static int x10_data_open (struct inode *inode, struct file *file);
static int x10_data_release (struct inode *inode, struct file *file);
static unsigned int x10_data_poll (struct file *, poll_table *);
static ssize_t x10_data_read (struct file *file, char *buffer,
			      size_t length, loff_t * offset);
static ssize_t x10_data_write (struct file *file, const char *buffer,
			       size_t length, loff_t * offset);
static loff_t x10_data_llseek (struct file *file, loff_t offset, int origin);

struct file_operations device_fops = {
	owner:		THIS_MODULE,
	read:		x10_data_read,
	write:		x10_data_write,
	open:		x10_data_open,
	poll:		x10_data_poll,
	release:	x10_data_release,
	llseek:		x10_data_llseek
};

struct x10api {
        // storage of system information
        int data_major;         // the target data device major
        int control_major;      // the target control device major
        int data;               // the actual data device major
        int control;            // the actual control device major

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
  int i;
  int res;

  x10api.data = -1;
  x10api.control = -1;
  x10api.data_major = data_major;
  x10api.control_major = control_major;

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
// only used if DEVFS is available
  int i, j;

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

static int x10_data_open (struct inode *inode, struct file *file)
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
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }

  return 0;
}

static int x10_data_release (struct inode *inode, struct file *file)
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
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }
  return 0;
}

static unsigned int x10_data_poll (struct file *file, poll_table * pt)
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

static ssize_t x10_data_read (struct file *file, char *buffer, size_t length, loff_t * offset)
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
  }
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.control_data)
  }

  return ret;
}

static ssize_t x10_data_write (struct file *file, const char *ubuffer, size_t length, loff_t * offset)
{
  int minor = XMINOR (file);
  int major = XMAJOR (file);
  int hc = HOUSECODE (minor);
  int uc = UNITCODE (minor);

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
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

  // do something here
  return length;
}

static loff_t x10_data_llseek (struct file *file, loff_t offset, int origin)
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

MODULE_LICENSE ("GPL");
