
/*
 *
 * $Id: dev.c,v 1.23 2006/03/29 02:33:23 root Exp john $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * X10 device abstraction module
 *
 */

#define DISTRIBUTION_VERSION "X10 DEV module v2.1.3 (wsh@sprintmail.com)"
#define CREATE_PROC 1
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
//#include <linux/config.h>

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
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/time.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>

#include <linux/version.h>

#define __X10_MODULE__
#include "x10.h"
#include "x10_strings.h"
#include "dev.h"

#undef CONFIG_OBSOLETE_MODPARM

#define DATA_DEVICE_NAME "x10d"
#define CONTROL_DEVICE_NAME "x10c"
#define API_BUFFER 1024

/* Here are some module parameters and definitions */
static int debug = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
MODULE_PARM (debug, "i");
#else
module_param(debug,int,0);
#endif
MODULE_PARM_DESC (debug, "turns on debug information (default=0)");

static int nonblockread = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
MODULE_PARM(nonblockread,"i");
#else
module_param(nonblockread,int,0);
#endif
MODULE_PARM_DESC(nonblockread,"If cat is to be used to read the status of units, this needs to be set to 1 so that it will return immediately after read.  Other wise the read blocks until a signal is received or new data is ready");

int syslogtraffic = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
MODULE_PARM (syslogtraffic, "i");
#else
module_param(syslogtraffic,int,0);
#endif
MODULE_PARM_DESC (syslogtraffic, "If set to 1, all X10 traffic is written to kern.notice in the syslog (default=0)");

static int data_major = 120;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
MODULE_PARM(data_major, "i");
#else
module_param(data_major,int,0);
#endif
MODULE_PARM_DESC(data_major, "Major character device for communicating with individual x10 units (default=120)");

static int control_major = 121;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
MODULE_PARM(control_major, "i");
#else
module_param(control_major,int,0);
#endif
MODULE_PARM_DESC(control_major, "Major character device for communicating with raw x10 transceiver (default=121)");

#define DRIVER_VERSION "$Id: dev.c,v 1.23 2006/03/29 02:33:23 root Exp john $"
char *version = DRIVER_VERSION;
static int delay=1;

static __inline__ int XMAJOR (struct file *a)
{
  return MAJOR ((a)->f_path.dentry->d_inode->i_rdev);
}

static __inline__ int XMINOR (struct file *a)
{
  return MINOR ((a)->f_path.dentry->d_inode->i_rdev);
}

/* prototypes for character device registration */
static int __init x10_init (void);
static void x10_exit (void);
static int x10_open (struct inode *inode, struct file *file);
static int x10_release (struct inode *inode, struct file *file);
static ssize_t x10_read(struct file *file,char *buffer,size_t length, loff_t * offset);
static ssize_t x10_write(struct file *file,const char *buffer,size_t length, loff_t * offset);
static loff_t x10_llseek(struct file *file, loff_t offset, int origin);
static ssize_t data_read(struct file *file,char *buffer,size_t length, loff_t * offset);
static ssize_t data_write(struct file *file,const char *buffer,size_t length, loff_t * offset);
static ssize_t control_read(struct file *file,char *buffer,size_t length, loff_t * offset);
static ssize_t control_write(struct file *file,const char *buffer,size_t length, loff_t * offset);
static loff_t control_llseek(struct file *file, loff_t offset, int origin);
static int x10mqueue_get(x10mqueue_t *q,x10_message_t *m);
static int x10mqueue_add(x10mqueue_t *q,int source,int hc, int uc, int cmd, __u32 f);
static int x10mqueue_test(x10mqueue_t *q);
static ssize_t api_write(x10_message_t *message,x10mqueue_t *q);
static ssize_t api_read(struct file *file,char *buffer,size_t length, loff_t * offset);

struct file_operations device_fops = {
	owner:		THIS_MODULE,
	read:		x10_read,
	write:		x10_write,
	open:		x10_open,
	release:	x10_release,
        llseek:		x10_llseek
};


typedef struct x10api {
        // storage of system information
        int data_major;         // the target data device major
        int control_major;      // the target control device major
        int data;               // the actual data device major
        int control;            // the actual control device major

	x10mqueue_t *mqueue;	// message queue to API
        int us_connected;	// flag for when API has initiated communication
        void *private;
} x10api_t;
static x10api_t x10api;

// ************************************************************************
// * hex dumping for most debugging and logging
// ************************************************************************
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

void usleep(int usec)
{
  int cs;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  // kernel 2.6.0 is better at multithreads so we have the ability to 
  // to use usleep to sleep for shorter intervals.  For 2.4.X, usleep 
  // will hang the kernel for the sleep period.
  if (usec < 600)
    udelay(usec);
  else if (usec*HZ < 2<<19)
    for (;usec>0;usec-=500) 
      udelay((usec>500)?500:usec);
  else {
#endif
    cs = current->state;
    current->state=TASK_INTERRUPTIBLE;
    schedule_timeout((long)((usec*HZ)>>19));
    current->state = cs;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
  }
#endif
}

// ************************************************************************
// * state and status management routines 
// ************************************************************************
typedef struct {
  atomic_t status[MAX_HOUSECODES][MAX_UNITS];
  u16 uc_statuschanged[MAX_HOUSECODES];
  u16 hc_statuschanged;
  u16 all_statuschanged;
  spinlock_t spinlock_status;
  spinlock_t spinlock_changed;
} state_t;
static state_t state;

static int getstate(int hc,int uc,int reset)
{
  int ret;
  spin_lock(&state.spinlock_changed);
  if (hc == -1) {                         // Overall status
    ret = (state.all_statuschanged != 0);
    if (reset)
      state.all_statuschanged = 0;
  }
  else if (uc == -1) {                    // Housecode status
    hc &= 0x0f;
    ret = ((state.hc_statuschanged & (1<<hc)) != 0);
    if (reset)
      state.hc_statuschanged &= ~(1<<hc);
  }
  else {                                  // individual unit status
    hc &= 0x0f;
    uc &= 0x0f;
    ret = ((state.uc_statuschanged[hc] & (1<<uc)) != 0);
    if (reset)
      state.uc_statuschanged[hc] &= ~(1<<uc);
  }
  spin_unlock(&state.spinlock_changed);
  return ret;
}
static void updatestatus(int hc, int uc, int newvalue, int markchanged)
{
  spin_lock(&state.spinlock_status);
  hc &= 0x0f;
  uc &= 0x0f;
  atomic_set(&state.status[hc][uc],newvalue);
  if (markchanged) {
    state.uc_statuschanged[hc] |= 1<<uc;
    state.hc_statuschanged |= 1<<hc;
    state.all_statuschanged = 1;
  }
    if (waitqueue_active(&x10api.mqueue->wq)){
      dbg("Waking up data mqueue waits");
      wake_up_interruptible(&x10api.mqueue->wq);
    }
  spin_unlock(&state.spinlock_status);
}
static unsigned int getstatus(int hc, int uc,int reset)
{
  unsigned int ret;

  spin_lock(&state.spinlock_status);
  hc &= 0x0f;
  uc &= 0x0f;
  if (reset)
    getstate(hc,uc,1);              // reset the status bit
  ret = atomic_read(&state.status[hc][uc]);
  spin_unlock(&state.spinlock_status);
  return ret;
}

static int unitchanged(int hc, int uc, atomic_t *value)
{
  if (getstatus(hc,uc,0) != atomic_read(value))
    return 1;
  return 0;
}

static int housecodechanged(int hc,unsigned char *values)
{
  int i;

  for (i = 0; i < MAX_UNITS; i++)
    if (getstatus(hc,i,0) != values[i])
      return 1;
  return 0;
}

static int anychanged(unsigned char *values)
{
  int i, j;

  for (i = 0; i < MAX_HOUSECODES; i++)
    for (j = 0; j < MAX_UNITS; j++)
      if (getstatus(i,j,0) != values[i*MAX_HOUSECODES+j])
        return 1;
  return 0;
}

// status display for a single housecode or all housecodes uses the same header
static char *header =
    "    1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n";
static size_t x10_control_dumphousecode(struct file *file, char *buffer, size_t length, loff_t * offset)
{
  int minor = XMINOR(file);
//  int major = XMAJOR(file);
  int action = HOUSECODE(minor);
  int target = UNITCODE(minor);
  char tbuffer[4 * (MAX_UNITS + 2)];
  char *pbuffer;
  ssize_t ret = 0;
  int i, addheaderline;
  x10controlio_t *ctrl = (x10controlio_t *)file->private_data;

  if (minor < 0 || minor > 0xff) {
    err("bad minor %d",minor);
    return 0;
  }
  addheaderline = (action == X10_CONTROL_HCWHDRS ? 1 : 0);
  if ((*offset > addheaderline)) {
    if ((file->f_flags & O_NONBLOCK) || nonblockread)
      return 0;
    else {  // block
      if (wait_event_interruptible(x10api.mqueue->wq,housecodechanged(target,ctrl->values)))
        return 0;
      *offset = 0;
    }
  }
  if (length < sizeof(tbuffer))
    return -EINVAL;
  if (addheaderline && *offset == 0) {
    strcpy(tbuffer, header);
  } else {
    pbuffer = tbuffer;
    if (addheaderline) {
      *pbuffer++ = (char) ('A' + target);
      *pbuffer++ = ':';
      *pbuffer++ = ' ';
    }
    for (i = 0; i < MAX_UNITS; i++) {  // unit contains the housecod
      sprintf(pbuffer, "%03d ", getstatus(target, i,0));
      ctrl->values[i] = getstatus(target,i,0);
      pbuffer = &pbuffer[4];    // next position is the 5th char
    }
    *pbuffer++ = '\n';
    *pbuffer++ = '\0';
  }
  ret = strlen(tbuffer);
  if (copy_to_user(buffer, tbuffer, ret + 1))
    ret = -EFAULT;
  else
    ++*offset;
  return ret;
}

static size_t x10_control_dumpstatus(struct file *file, char *buffer, size_t length, loff_t * offset)
{
  int minor = XMINOR(file);
//  int major = XMAJOR(file);
//  int action = HOUSECODE(minor);
  int target = UNITCODE(minor);
  char tbuffer[4 * (MAX_UNITS + 2)];
  char *pbuffer;
  ssize_t ret = 0;
  int j, addheaderline, dumpchanged;
  x10controlio_t *ctrl = (x10controlio_t *)file->private_data;

  if (minor < 0 || minor > 0xff) {
    err("bad minor %d",minor);
    return 0;
  }
  addheaderline = (target & X10_CONTROL_WITHHDRS ? 1 : 0);
  dumpchanged = (target & X10_CONTROL_READCHANGED ? 1 : 0);
  // make sure that you display it at least once.  But, don't 
  // display again unless there are changes.  
  // cat < /dev/x10/a10 should return one thing.  
  if (*offset >= (MAX_HOUSECODES + addheaderline)) {
    // special case...if we are dumping the change log, 
    // nothing is reset so we can't block.  When dumping 
    // the changelong, it always acts as if it is non-blocking
    if ((file->f_flags & O_NONBLOCK) || dumpchanged || nonblockread)
      return 0;
    else {  // block
      if (wait_event_interruptible(x10api.mqueue->wq,anychanged(ctrl->values)))
        return 0;
      *offset = 0;
    }
  }
  if (length < sizeof(tbuffer))
    return -EINVAL;
  if (addheaderline && *offset == 0)
    strcpy(tbuffer, header);
  else {
    pbuffer = tbuffer;
    if (addheaderline) {
      *pbuffer++ = (char) ('A' + (*offset) - addheaderline);
      *pbuffer++ = ':';
      *pbuffer++ = ' ';
    }
    for (j = 0; j < MAX_UNITS; j++) {// unit contains the housecode
      if (dumpchanged){
        sprintf(pbuffer, " %d  ", getstate(*offset-addheaderline,j,0));
      }
      else {
        int hc = *offset-addheaderline;
        sprintf(pbuffer,"%03d ",getstatus(hc,j,0));
        ctrl->values[hc*MAX_HOUSECODES+j] = getstatus(hc,j,0);
      }
      pbuffer = &pbuffer[4];
    }
    *pbuffer++ = '\n';
    *pbuffer++ = '\0';
  }
  ret = strlen(tbuffer);
  if (copy_to_user(buffer, tbuffer, ret + 1))
    ret = -EFAULT;
  else
    ++ *offset;
  return ret;
}

// ************************************************************************
// * log management routines 
// ************************************************************************
                        
// log.end always points to the last entry in the log.  
typedef struct {
  struct {
    int dir;		// direction 0=receive, 1=transmit
    int hc;		// housecode
    int uc;		// unit code
    int fc;		// function code
    struct timespec64 tv;	// timestamp
  } data[MAX_LOG];
  atomic_t begin;
  atomic_t end;
  spinlock_t spinlock;
  wait_queue_head_t changed;
} log_t;

static log_t log;

static void log_init(void)
{
  spin_lock_init(&log.spinlock);
  init_waitqueue_head(&log.changed);
  atomic_set(&log.begin,0);
  atomic_set(&log.end,0);
}

static void log_store(int dir,int hc, int uc, int fc)
{
  int begin,end;

  spin_lock(&log.spinlock);
  dbg("dir=%d, hc=%d, uc=%d, fc=%d",dir,hc,uc,fc);
  if (uc >= 0 && fc >= 0){
    log("%c %c%d",(dir?'T':'R'),hc+'A',uc+1);
    log("%c %c %s",(dir?'T':'R'),hc+'A',logstring[fc]);
  }
  else if (uc >= 0)
    log("%c %c%d",(dir?'T':'R'), hc+'A', uc+1);
  else if (fc >= 0)
    log("%c %c %s",(dir?'T':'R'), hc+'A', logstring[fc]);

  end = atomic_read(&log.end);
  begin = atomic_read(&log.begin);
  if (++end >= MAX_LOG)
    end = 0;
  atomic_set(&log.end,end);
  if (end == begin) {
    if (++begin >= MAX_LOG)
      begin = 0;
    atomic_set(&log.begin,begin);
  }
  log.data[end].dir = dir;
  log.data[end].hc = hc;
  log.data[end].uc = uc;
  log.data[end].fc = fc;
  ktime_get_real_ts64(&log.data[end].tv);
  spin_unlock(&log.spinlock);
  if (waitqueue_active(&log.changed))
    wake_up_interruptible(&log.changed);
}

static int log_get(loff_t *offset,unsigned char *ubuffer,int len)
{
  int hc,uc,fc,dir;
  struct timespec64 *tv;
  char tbuffer[80];
  int ret;

  spin_lock(&log.spinlock);
  if (++*offset >= MAX_LOG)
    *offset = 0;
  dir= log.data[*offset].dir;
  hc = log.data[*offset].hc;
  uc = log.data[*offset].uc;
  fc = log.data[*offset].fc;
  tv = &log.data[*offset].tv;
  spin_unlock(&log.spinlock);
  if (uc >= 0 && fc >= 0){
    sprintf(tbuffer,"%ld %c %c%d\n%ld %c %c %s\n",(long)tv->tv_sec,(dir?'T':'R'),hc+'A',uc+1,(long)tv->tv_sec,(dir?'T':'R'), hc+'A',logstring[fc]);
  }
  else if (uc >= 0)
    sprintf(tbuffer,"%ld %c %c%d\n",(long)tv->tv_sec,(dir?'T':'R'),hc+'A',uc+1);
  else if (fc >= 0)
    sprintf(tbuffer,"%ld %c %c %s\n",(long)tv->tv_sec,(dir?'T':'R'), hc+'A',logstring[fc]);
  else
    sprintf(tbuffer, "Error - dir=%d, hc=%d, uc=%d, fc=%d\n",dir,hc, uc,fc);
  ret = min((int)strlen(tbuffer)+1,len);
  if (copy_to_user(ubuffer, tbuffer, ret))
    ret = -EFAULT;
  return ret;
}

/*
   Initialization code:  When the module is loaded, it attempts to open the 
   serial port specified in "serial".  

   Note also that it is possible to set major_device and major_unit to a value
   of zero to have the system allocate major device numbers.  If this is done, 
   the user must look in /proc/devices to find the device numbers.
*/
char *procdirname = "bus/x10";
const char *procinfoname = "info";
static struct proc_dir_entry *procdir, *procinfo;
#ifndef CREATE_PROC
static int proc_read_info(char *page, char **start, off_t off, int counter,
                          int *eof, void *data)
#else
static ssize_t proc_read_info(struct file *filp, char *page, size_t counter,
                          loff_t *offp)
#endif
{
  ssize_t len;
//  MOD_INC_USE_COUNT;
  len = sprintf(page, "%s\nUserspace Status:  %s\n",
                DISTRIBUTION_VERSION,
                x10api.us_connected != 0 ? "connected" : "not connected");
//  MOD_DEC_USE_COUNT;
  return len;
}

#ifdef CREATE_PROC
const struct proc_ops proc_fops = {
.proc_read = proc_read_info
};
#endif

int __init
x10_init (void)
{
  int res,returnvalue = 0;

  procdir=proc_mkdir(procdirname,NULL);
  if (procdir == NULL) {
    returnvalue = -ENOMEM;
    goto error_dir;
  }
#ifndef CREATE_PROC
  procinfo = create_proc_read_entry(procinfoname, S_IRUSR|S_IRGRP|S_IROTH,
                                    procdir, proc_read_info, NULL);
#else
  procinfo = proc_create(procinfoname, S_IRUSR|S_IRGRP|S_IROTH,
                         procdir, &proc_fops);
#endif
  if (procinfo == NULL) {
    returnvalue = -ENOMEM;
    goto error_info;
  }

  memset(&x10api,0,sizeof(x10api_t));
  x10api.data = -1;
  x10api.control = -1;
  x10api.data_major = data_major;
  x10api.control_major = control_major;

  memset(&state,0,sizeof(state_t));
  spin_lock_init(&state.spinlock_status);
  spin_lock_init(&state.spinlock_changed);
  log_init();

  info (DISTRIBUTION_VERSION);
  info ("%s", version);
  dbg ("%s", "starting initialization");

  // the private data for each device identifies the minor device info

  dbg ("registering data_major device %d for %s",
       x10api.data_major, DATA_DEVICE_NAME);
  res = register_chrdev (x10api.data_major, DATA_DEVICE_NAME, &device_fops);
  if (res < 0) {
    err ("unable to register major device %d for %s", x10api.data_major,
		 DATA_DEVICE_NAME);
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
error_info:
  remove_proc_entry(procdirname,NULL);
error_dir:
  return returnvalue;
}

void
x10_exit (void)
{
  dbg ("%s", "Unloading X10 driver");

  if (x10api.data >= 0) {
    dbg ("unregistering %d:%s", x10api.data, DATA_DEVICE_NAME);
    unregister_chrdev (x10api.data, DATA_DEVICE_NAME);
    //if (unregister_chrdev (x10api.data, DATA_DEVICE_NAME) == -EINVAL)
    //  dbg ("error unregistering %s", DATA_DEVICE_NAME);
  }
  if (x10api.control >= 0) {
    dbg ("unregistering %d:%s", x10api.control, CONTROL_DEVICE_NAME);
    unregister_chrdev (x10api.control, CONTROL_DEVICE_NAME);
    //if (unregister_chrdev (x10api.control, CONTROL_DEVICE_NAME) == -EINVAL)
    //  dbg ("error unregistering %s", CONTROL_DEVICE_NAME);
    }
  remove_proc_entry(procinfoname,procdir);
  remove_proc_entry(procdirname,NULL);
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
//  int uc = UNITCODE(minor);

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){ // userspace
    // bidirectional connection to the userspace portion of driver
    dbg("%s","bidirectional connection to userspace open");
    x10api.mqueue = kmalloc(sizeof(x10mqueue_t),GFP_KERNEL);
    file->private_data = (void *)x10api.mqueue;
    atomic_set(&x10api.mqueue->head,0);
    atomic_set(&x10api.mqueue->tail,0);
    spin_lock_init(&x10api.mqueue->spinlock);
    init_waitqueue_head(&x10api.mqueue->wq);
    init_waitqueue_head(&x10api.mqueue->apiq);
  }
  else if (major == x10api.control_major) {
    x10controlio_t *ctrl;
    if (x10api.us_connected != 1){
      dbg("%s","/dev device opened without userspace connection");
      return -EFAULT;
    }
    ctrl = (x10controlio_t *)kmalloc(sizeof(x10controlio_t),GFP_KERNEL);
    file->private_data = (void *)ctrl;
    memset(ctrl,0,sizeof(x10controlio_t));
    atomic_set(&ctrl->changed,1);
    switch (hc) {
      case X10_CONTROL_DUMPLOG:
        file->f_pos = atomic_read(&log.begin);
        break;
      case X10_CONTROL_STATUS:
      case X10_CONTROL_HCWHDRS:
      case X10_CONTROL_HCWOHDRS:
	break;
    }
  }
  else if (major == x10api.data_major){
    x10dataio_t *data;
    if (x10api.us_connected != 1){
      dbg("%s","/dev device opened without userspace connection");
      return -EFAULT;
    }
    data = (x10dataio_t *)kmalloc(sizeof(x10dataio_t),GFP_KERNEL);
    file->private_data = (void *)data;
    atomic_set(&data->changed,1);
    atomic_set(&data->value,0);
  }
  else
    return -EFAULT;

  return 0;
}

static int x10_release (struct inode *inode, struct file *file)
{
  int minor=XMINOR(file);
  int major=XMAJOR(file);
  int hc = HOUSECODE(minor);
//  int uc = UNITCODE(minor);

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
      x10api.mqueue = NULL;		// freed at kfree(file->private_data)
      dbg("%s","bidirectional connection to userspace closed");
    }
  }
/*
  // we don't really need to do anything here...the only cleanup is to
  // free up the private data
  else if (major == x10api.control_major) {
  }
  else {  // if (major = x10api.data_major)
  }
*/
  kfree(file->private_data);
  file->private_data = NULL;
  return 0;
}

static loff_t x10_llseek(struct file *file, loff_t offset, int origin)
{
  int minor=XMINOR(file);
  int major=XMAJOR(file);
  int hc = HOUSECODE(minor);
//  int uc = UNITCODE(minor);

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);

  if (major == x10api.control_major && hc == X10_CONTROL_API){
    return -ESPIPE;
  }
  else if (major == x10api.control_major) {
    switch (hc) {
      case X10_CONTROL_DUMPLOG:
        return(control_llseek(file,offset,origin));
        break;
      case X10_CONTROL_STATUS:
      case X10_CONTROL_HCWHDRS:
      case X10_CONTROL_HCWOHDRS:
      default:
        return -ESPIPE;
	break;
    }
  }
  else {  // if (major = x10api.data_major)
    return -ESPIPE;
  }
}


static ssize_t x10_read (struct file *file, char *ubuffer,
						 size_t length, loff_t * offset)
{
  int major, minor, hc;
  if (!file) return 0;
  minor = XMINOR (file);
  major = XMAJOR (file);
  hc = HOUSECODE (minor);
//  int uc = UNITCODE (minor);

  if (!(major == x10api.control_major && hc == X10_CONTROL_API)){
    // don't add the debug information if the reads come from the API interface
    ANNOUNCE;
    dbg ("major=%d, minor=%d", major, minor);
    if (major == x10api.control_major)
      dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags,
		   file->f_mode);
    else
      dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags,
		   file->f_mode);
  }

  if (major == x10api.control_major && hc == X10_CONTROL_API){
    // called from the userspace driver
    return(api_read(file,ubuffer,length,offset));
  }
  else if (major == x10api.control_major) {
    // called from the /dev/x10/* character devices
    return(control_read(file,ubuffer,length,offset));
  }
  else if (major == x10api.data_major) {  
    // called from the /dev/x10/[a-p][1-16] character devices
    return(data_read(file,ubuffer,length,offset));
  }
  else 
    return -EFAULT;
}

static ssize_t x10_write (struct file *file, const char *ubuffer, size_t length, loff_t * offset)
{
  int minor = XMINOR (file);
  int major = XMAJOR (file);
  int hc = HOUSECODE (minor);
//  int uc = UNITCODE (minor);

  ANNOUNCE;
  dbg ("major=%d, minor=%d", major, minor);
  if (major == x10api.control_major)
    dbg ("(CTRL) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);
  else
    dbg ("(DATA) file->f_flags = 0x%x file->f_mode=0x%x", file->f_flags, file->f_mode);


  if (major == x10api.control_major && hc == X10_CONTROL_API){
    x10_message_t message;
    if (length < sizeof(message)) {
      dbg("%s","error:  Invalid message size");
      return -EFAULT;
    }
    if (copy_from_user(&message,ubuffer,sizeof(message)))
      return -EFAULT;
    // this comes from the userspace daemon
    return(api_write(&message,(x10mqueue_t *)file->private_data));
  }
  else if (major == x10api.control_major) {
    // this comes from the /dev/x10/* character devices
    return(control_write(file,ubuffer,length,offset));
  }
  else if (major == x10api.data_major) {
    // this comes from the /dev/x10/[a-p][1-16] devices
    return(data_write(file,ubuffer,length,offset));
  }
  else 
    return -EFAULT;
}

static int x10mqueue_add(x10mqueue_t *q,int source,int hc, int uc, int cmd, __u32 f)
{
  int head,tail,ret=1;
  x10_message_t *message;

  ANNOUNCE;
  spin_lock(&q->spinlock);

  tail = atomic_read(&q->tail);
  head = atomic_read(&q->head);
  message = &q->queue[tail];
  message->source = source;
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
  dbg("source=%x, hc=%x, uc=%x, cmd=%x, f=%x, pos=%d",source,hc,uc,cmd,f,tail);
  if (waitqueue_active(&x10api.mqueue->apiq)){
    dbg("Waking up mqueue waits");
    wake_up_interruptible(&x10api.mqueue->apiq);
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
    ANNOUNCE;
    message = &q->queue[head];
    if (++head >= MESSAGE_QUEUE_SIZE)
      head = 0;
    atomic_set(&q->head,head);
    memcpy(m,message,sizeof(x10_message_t));
    ret = 0;
    dbg("source=%x, hc=%x, uc=%x, cmd=%x, f=%x, pos=%d",m->source,m->housecode,m->unitcode,m->command,m->flag,head);
  }
  spin_unlock(&q->spinlock);
  return ret;
}

static int x10mqueue_status(x10mqueue_t *q)
{
  int head,tail;

  spin_lock(&q->spinlock);
  tail = atomic_read(&q->tail);
  head = atomic_read(&q->head);
  spin_unlock(&q->spinlock);
  return (head == tail);
}

static int x10mqueue_test(x10mqueue_t *q)
{
  int ret;

  spin_lock(&q->spinlock);
  ret = atomic_read(&q->tail) == atomic_read(&q->head) ? 0 : 1;
  spin_unlock(&q->spinlock);
  return ret;
}

ssize_t api_write(x10_message_t *message,x10mqueue_t *q)
{
  int value;

  ANNOUNCE;
  switch (message->source) {
    case X10_API:
	if (message->command == X10_CMD_STATUS) {
	  dbg("%s","X10_API connected");
	  x10api.us_connected = 1;
	  // OK...now write the status to the read queue
	  if (x10mqueue_add(q,X10_API,0,0,X10_CMD_ON,0))
	    return -EFAULT;
	  return sizeof(x10_message_t);
	}
	dbg("Invalid X10_API command %x",message->command);
	break;
    case X10_DATA:
	dbg("%s","X10_API data");
	value = (int)message->flag;
	if (value > 100 || value < 0)
	  return -EFAULT;
	updatestatus(message->housecode,message->unitcode,value,1);
	return sizeof(x10_message_t);
	break;
    case X10_CONTROL:
	dbg("%s","X10_API control");
	if (message->unitcode >=0 && message->command >= 0){
        	log_store((int)message->flag,message->housecode,message->unitcode,-1);
       		log_store((int)message->flag,message->housecode,-1,message->command);
	} else 
        	log_store((int)message->flag,message->housecode,message->unitcode,message->command);
	return sizeof(x10_message_t);
	break;
    default:
      dbg("Invalid message type %x",message->source);
      break;
  }
  return -EFAULT;
}

static ssize_t api_read(struct file *file,char *ubuffer,size_t length, loff_t * offset)
{
  x10_message_t m;

  ANNOUNCE;
  if (length < sizeof(x10_message_t)) {
    dbg("%s","error:  Invalid message size");
    return -EFAULT;
  }
  if (!x10mqueue_test((x10mqueue_t *)file->private_data)) {
    if (file->f_flags & O_NONBLOCK)
      return 0;
    else{
      if (wait_event_interruptible(x10api.mqueue->apiq,x10mqueue_test((x10mqueue_t *)file->private_data)))
        return 0;
      *offset = 0;
    }
  }
  if (x10mqueue_get((x10mqueue_t *)file->private_data,&m))
    return -EFAULT;
  if (copy_to_user(ubuffer,&m,sizeof(x10_message_t))) 
    return -EFAULT;
  return(sizeof(x10_message_t));
}

static ssize_t data_read(struct file *file,char *buffer,size_t length, loff_t * offset)
{
  int ret = -EINVAL;
  char tbuffer[64];
  int minor=XMINOR(file);
  int major=XMAJOR(file);
  int hc = HOUSECODE(minor);
  int uc = UNITCODE(minor);
  x10dataio_t *data = (x10dataio_t *)file->private_data;
  if (*offset > 0) {
    if ((file->f_flags & O_NONBLOCK) || nonblockread)
      return 0;
    else {
      if (wait_event_interruptible(x10api.mqueue->wq,unitchanged(hc,uc,&data->value)))
	return 0;
      *offset = 0;
    }
  }
  if (length < sizeof(tbuffer))
    return -EINVAL;
  dbg("major:minor %d:%d (%c%02d)",major,minor,'a'+(char)(hc),uc+1);
  atomic_set(&data->value,getstatus(hc,uc,0));
  sprintf(tbuffer,"%03d\n",atomic_read(&data->value));
  ret = strlen(tbuffer);
  if (copy_to_user(buffer,&tbuffer,ret+1))
    return -EFAULT;
  else
    ++*offset;
  return ret;
}

static ssize_t data_write(struct file *file,const char *ubuffer,size_t length, loff_t * offset)
{
  int minor=XMINOR(file);
//  int major=XMAJOR(file);
  int hc = HOUSECODE(minor);
  int uc = UNITCODE(minor);
  int cmd = -1, pdim=-1,i,ret=0;
  char *buffer;
//  note that data->value is only updated when received from the user API
//  x10dataio_t *data = (x10dataio_t *)file->private_data;
  if (length > 1) {
    buffer = kmalloc((length+1)*sizeof(char),GFP_KERNEL);
    if (buffer == NULL) return -ENOMEM;
    if (copy_from_user(buffer,ubuffer,length)) {
      kfree(buffer);
      return -EFAULT;
    }
    buffer[length] = '\0';
    for (i=length-1; i >= 0; i--)
      if (buffer[i] < ' ') buffer[i] = '\0';
    dbg("hc=%c, uc=%d, buffer=%s",(char)hc+'A',uc,buffer);
    cmd = parse_function(buffer,X10_CMD_MASK_UNITCODES|X10_CMD_MASK_PRESETDIM);
    if (cmd < 0) {
      dbg("Invalid command %s",buffer);
      warn("Invalid command '%s' to %c%d",buffer,(char)hc+'A',uc+1);
      kfree(buffer);
      return -EINVAL;
    }
    kfree(buffer);
    if (cmd == X10_CMD_END)
      cmd = -1;
  }
  else {
    dbg("hc=%c, uc=%d, buffer=(null)",(char)hc+'A',uc);
    cmd = -1;
  }
  if ((cmd >> 8 != 0) && 
     ((cmd & 0xff) == X10_CMD_PRESETDIMLOW ||
     (cmd & 0xff) == X10_CMD_PRESETDIMHIGH)) {
    pdim = (cmd >> 8) - 'A';
    cmd &= 0xff;
    dbg("%c%d %s %c",(char)hc+'A',uc+1,logstring[cmd],(char)pdim+'A');
    ret = x10mqueue_add(x10api.mqueue,X10_DATA,hc,uc,-1,0);
    if (ret >= 0) 
      ret = x10mqueue_add(x10api.mqueue,X10_DATA,pdim,-1,cmd,0);
  }
  else 
    ret = x10mqueue_add(x10api.mqueue,X10_DATA,hc,uc,cmd,0);
  if (!(file->f_flags & O_NONBLOCK)){
    int i = 0;
    while (!x10mqueue_status(x10api.mqueue)){
//      dbg("Blocking while sending message\n");
      usleep(delay*1000);
      if (i++ > 10000){
        dbg("Timeout on blocking while sending message");
        return -EINVAL;
      }
    }
  }
  if (ret < 0)
    return ret;
  return length;
}

static ssize_t control_read(struct file *file,char *buffer,size_t length, loff_t *offset)
{
  int minor=XMINOR(file);
  int major=XMAJOR(file);
  int action = HOUSECODE(minor);
  int target = UNITCODE(minor);
//  x10controlio_t *ctrl = (x10controlio_t *)file->private_data;

  switch (action) {
    case X10_CONTROL_HCWHDRS:
    case X10_CONTROL_HCWOHDRS:
      return(x10_control_dumphousecode(file,buffer,length,offset));
      break;
    case X10_CONTROL_STATUS:
      return(x10_control_dumpstatus(file,buffer,length,offset));
      break;
    case X10_CONTROL_DUMPLOG:
      if (target == 0) {
        if ((*offset==(loff_t)atomic_read(&log.end))&&(file->f_flags&O_NONBLOCK))
          return 0;
        if (wait_event_interruptible(log.changed,(int)*offset != (loff_t)atomic_read(&log.end)))
          return 0;
        return(log_get(offset,buffer,length));
      }
      break;
    default:
      err("Unknown device major=%d, minor=%d",major,minor);
      return -EINVAL;
      break;
  }
  return -EINVAL;
}

static ssize_t control_write(struct file *file,const char *ubuffer,size_t length, loff_t * offset)
{
  int minor=XMINOR(file);
//  int major=XMAJOR(file);
  int action = HOUSECODE(minor);
  int target = UNITCODE(minor);
  char *tbuffer;
  int i,cmd,ret;
//  x10controlio_t *ctrl = (x10controlio_t *)file->private_data;

  switch (action) {
    case X10_CONTROL_HCWHDRS:
    case X10_CONTROL_HCWOHDRS:
      if (length <= 1) return -EINVAL;
      tbuffer = kmalloc((length+1)*sizeof(char),GFP_KERNEL);
      if (tbuffer == NULL) return -ENOMEM;
      if (copy_from_user(tbuffer,ubuffer,length)){
        kfree(tbuffer);
        return -EFAULT;
      }
      tbuffer[length] = '\0';
      for (i = length-1; i >=0; i--) 
        if (tbuffer[i] < ' ') 
          tbuffer[i] = '\0';
      if (strlen(tbuffer) <= 0) {
        kfree(tbuffer);
        return 0;
      }
      // now we have a clean copy of the user text string
      if ((cmd = parse_function(tbuffer,X10_CMD_MASK_HOUSECODES)) < 0) {
        dbg("Invalid command %s",tbuffer);
        warn("Invalid command '%s' to %c",tbuffer,(char)target+'A');
        kfree(tbuffer);
        return -EINVAL;
      }
      ret = x10mqueue_add(x10api.mqueue,X10_CONTROL,target,-1,cmd,0);
      kfree(tbuffer);
      if (!(file->f_flags & O_NONBLOCK)){
        while (!x10mqueue_status(x10api.mqueue)){
          dbg("Blocking while sending message\n");
          usleep(delay*1000);
        }
      }
      if (ret == 0)
        return length;
      else
	return -EFAULT;
      break;
    case X10_CONTROL_STATUS:
    case X10_CONTROL_DUMPLOG:
    default:
      // you can't write to the status or log devices!!!
      return -EINVAL;
      break;
  }
  return -EINVAL;
}

static loff_t control_llseek(struct file *file, loff_t offset, int origin)
{
  int minor=XMINOR(file);
//  int major=XMAJOR(file);
  int action = HOUSECODE(minor);
//  int target = UNITCODE(minor);
//  x10controlio_t *ctrl = (x10controlio_t *)file->private_data;
  dbg("origin=%d offset=%lld",origin,offset);
  switch(action) {
    case X10_CONTROL_DUMPLOG:
      dbg("%s","action=X10_CONTROL_DUMPLOG");
      switch (origin) {
        case 0:  // SEEK_SET
          dbg("SEEK_SET offset=%lld",offset);
          break;
        case 1:  // SEEK_CUR
          dbg("SEEK_CUR offset=%lld",offset);
          break;
        case 2:  // SEEK_END
          dbg("SEEK_END offset=%lld",offset);
          file->f_pos=(unsigned long)atomic_read(&log.end);
          dbg("SEEK_END new offset=%lld",file->f_pos);
          return(file->f_pos);
          break;
        default:
          dbg("%s","bad origin");
          return -EINVAL;
      }
      break;
    default:
      dbg("bad action=0x%x",action);
      return -EINVAL;
      break;
  }
  return -ESPIPE;
}

MODULE_LICENSE ("GPL");
