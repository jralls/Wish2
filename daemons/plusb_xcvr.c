/*
 *
 * $Id: plusb_xcvr.c,v 1.9 2005/03/27 04:38:19 root Exp root $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * X10 line discipline attach program
 *
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

/* This code was adapted from the inputattach.c code used for the joystick
   drivers by Vojtech Pavlik.  It has been modified to match up the X10 
   transcievers but is basically the same logic.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <syslog.h>
#include <errno.h>

#include <asm/types.h>

#include <linux/usbdevice_fs.h>

#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0)
#define HID_MAX_USAGES 2
#endif
#include <linux/hiddev.h>

#include "x10.h"
#include "x10d.h"
#include "plusb_xcvr.h"

#define USB_ENDPOINT_IN		0x81
#define USB_ENDPOINT_OUT	0x01

#define DELAY 1000

int transmit(int hc, int uc, int cmd);
static void startup();
int serial=0;

struct xcvrio *io = NULL;
sem_t sem_ack;
int ack;
sem_t cts;


struct plusb {
  struct hiddev_report_info rep_info;
  struct hiddev_usage_ref usage_ref;
  struct hiddev_field_info field_info;
} hidout,hidin;

int hidwrite(int fd,unsigned char *c,int length)
{
  int ret = 0;
  int i = 0,alv = 0,yalv = 0;
  char scratch[80];
/*
  hidout.rep_info.report_type = HID_REPORT_TYPE_OUTPUT;
  hidout.rep_info.report_id = HID_REPORT_ID_FIRST;
  while (ioctl(fd, HIDIOCGREPORTINFO, &hidout.rep_info) >= 0) {
    dsyslog(LOG_INFO,"OUTPUT Report id: %d (%d fields)\n",hidout.rep_info.report_id,hidout.rep_info.num_fields);
    for (alv = 0; alv < hidout.rep_info.num_fields; alv++) {
      memset(&hidout.field_info,0,sizeof(hidout.field_info));
      hidout.field_info.report_type = hidout.rep_info.report_type;
      hidout.field_info.report_id = hidout.rep_info.report_id;
      hidout.field_info.field_index = alv;
      ioctl(fd, HIDIOCGFIELDINFO, &hidout.field_info);
      dsyslog(LOG_INFO,"OUTPUT Field: %d: app: %04x phys %04x flags %x (%d usages) unit %x exp %d\n", alv, hidout.field_info.application, hidout.field_info.physical, hidout.field_info.flags, hidout.field_info.maxusage, hidout.field_info.unit, hidout.field_info.unit_exponent);
      memset(&hidout.usage_ref, 0, sizeof(hidout.usage_ref));
      for (yalv = 0; yalv < hidout.field_info.maxusage; yalv++) {
        hidout.usage_ref.report_type = hidout.field_info.report_type;
        hidout.usage_ref.report_id = hidout.field_info.report_id;
        hidout.usage_ref.field_index = alv;
        hidout.usage_ref.usage_index = yalv;
        ioctl(fd, HIDIOCGUCODE, &hidout.usage_ref);
        ioctl(fd, HIDIOCGUSAGE, &hidout.usage_ref);
        dsyslog(LOG_INFO,"OUTPUT Usage: %04x val %02x idx %x\n", hidout.usage_ref.usage_code,hidout.usage_ref.value, hidout.usage_ref.usage_index);
      }
    }
    hidout.rep_info.report_id |= HID_REPORT_ID_NEXT;
  }
*/
  for (i = 0; i < hidout.field_info.maxusage; i++) {
    hidout.usage_ref.report_type = hidout.rep_info.report_type;
    hidout.usage_ref.report_id = 0;
    hidout.usage_ref.field_index = 0;
    hidout.usage_ref.usage_index = i;
    hidout.usage_ref.value = c[i];
    ret = ioctl(fd,HIDIOCSUSAGE,&hidout.usage_ref);
    if (ret < 0){
      dsyslog(LOG_INFO,"Error loading usage (%s)\n",strerror(errno));
      return ret;
    }
  }
  
  hidout.rep_info.report_type = HID_REPORT_TYPE_OUTPUT;
  hidout.rep_info.report_id = 0;
  hidout.rep_info.num_fields = 1;
  ret = ioctl(fd,HIDIOCSREPORT,&hidout.rep_info);
  if (ret < 0){
    dsyslog(LOG_INFO,"Error writing to USB device (%s)\n",strerror(errno));
    return ret;
  }
  else {
    dsyslog(LOG_INFO,"wrote %s\n",dumphex(scratch,(void *)c,length));
//    usleep(delay*250);
  }
  
  return length;
}

struct hiddev_event ev[64];
int hidread(int fd,unsigned char *c, int length, int timeout)
{
  fd_set fdset;
  int rd;
  int i;
  struct timeval tv;
  int alv,yalv;

//  timeout = 0;
  tv.tv_sec = 0;
  tv.tv_usec = timeout * 1000;
  FD_ZERO(&fdset);
  i = 0;
  FD_SET(fd,&fdset);
  rd = select(fd+1,&fdset,NULL,NULL,(timeout <= 0 ? NULL : &tv));
  if (rd < 0) {
    dsyslog(LOG_INFO,"Error reading USB device - %s\n",strerror(errno));
    return rd;
  }
  if (rd == 0) {
//    dsyslog(LOG_INFO,"hidread timeout\n");
    return 0;  // timeout
  }

  // now we have an event ready to go...
  rd = read(fd,ev,sizeof(ev));
  if (rd < 0){
    dsyslog(LOG_INFO,"Error reading USB device - %s\n",strerror(errno));
    return rd;
  }
  if (rd < sizeof(ev[0])){
    dsyslog(LOG_INFO,"Error - got short read from USB device\n");
    return -1;
  }

  // now read the report and usage fields
  hidin.rep_info.report_type = HID_REPORT_TYPE_INPUT;
  hidin.rep_info.report_id = HID_REPORT_ID_FIRST;
  ioctl(fd,HIDIOCGREPORT,&hidout.rep_info);
  while (ioctl(fd, HIDIOCGREPORTINFO, &hidin.rep_info) >= 0) {
    dsyslog(LOG_INFO,"INPUT Report id: %d (%d fields)\n",hidin.rep_info.report_id,hidin.rep_info.num_fields);
    if (delay > 0) usleep(delay*250);
    for (alv = 0; alv < hidin.rep_info.num_fields; alv++) {
      memset(&hidin.field_info,0,sizeof(hidin.field_info));
      hidin.field_info.report_type = hidin.rep_info.report_type;
      hidin.field_info.report_id = hidin.rep_info.report_id;
      hidin.field_info.field_index = alv;
      ioctl(fd, HIDIOCGFIELDINFO, &hidin.field_info);
      dsyslog(LOG_INFO,"INPUT Field: %d: app: %04x phys %04x flags %x (%d usages) unit %x exp %d\n", alv, hidin.field_info.application, hidin.field_info.physical, hidin.field_info.flags, hidin.field_info.maxusage, hidin.field_info.unit, hidin.field_info.unit_exponent);
      if (delay > 0) usleep(delay*250);
      memset(&hidin.usage_ref, 0, sizeof(hidin.usage_ref));
      for (yalv = 0; yalv < hidin.field_info.maxusage; yalv++) {
        hidin.usage_ref.report_type = hidin.field_info.report_type;
        hidin.usage_ref.report_id = hidin.field_info.report_id;
        hidin.usage_ref.field_index = alv;
        hidin.usage_ref.usage_index = yalv;
        ioctl(fd, HIDIOCGUCODE, &hidin.usage_ref);
        ioctl(fd, HIDIOCGUSAGE, &hidin.usage_ref);
        dsyslog(LOG_INFO,"INPUT Usage: %04x val %02x idx %x\n", hidin.usage_ref.usage_code,hidin.usage_ref.value, hidin.usage_ref.usage_index);
        if (delay > 0) usleep(delay*250);
        c[hidin.usage_ref.usage_index] = hidin.usage_ref.value;
      }
    }
    hidin.rep_info.report_id |= HID_REPORT_ID_NEXT;
  }
  return hidin.field_info.maxusage;
}  

__s32 statusrequest() {
  unsigned char start[8];
  int ret = 0;
  int i;

  memset(start,0,sizeof(start));
  start[0] = X10_PLUSB_XMITCTRL;
  start[1] = X10_PLUSB_TCTRLSTATUS;       // request status response
  i = 0;
  do {
    ret = hidwrite(serial,start,sizeof(start));
    if (ret == EBUSY)
      usleep(DELAY*1000);
  } while (ret < 0 && (i++ < retries));
  if (ret < 0) {
    syslog(LOG_INFO,"USB Powerlinc initialization failed on status request (%s)\n",strerror(errno));
    return ret;
  }

  i = 0;
  do {
    ret = hidread(serial,start,sizeof(start),timeout);
    if (ret == EBUSY)
      usleep(DELAY*1000);
  } while (ret < 0 && (i++ < retries));
  if (ret < 0) {
    syslog(LOG_INFO,"USB Powerlinc initialization failed to return status (%s)\n",strerror(errno));
    return ret;
  }
  if (ret == 0){
    syslog(LOG_INFO,"Timeout waiting for response from PowerLinc\n");
    return -1;
  }
  return((start[0]<<24 | start[1]<<16 | start[3]<<8 | start[4])&0x7fffffff);
}

int hidinit(int fd)
{
  struct hiddev_devinfo usbinfo;
  struct hiddev_field_info field_info;
  unsigned char start[8];
  int report_type;
  int appl,alv,yalv;
  unsigned int intf;
  __s32 status;
  ssize_t ret;

  dsyslog(LOG_INFO,"Connecting to USB Powerlinc Interface on %s\n",io->device);
  ioctl(serial,HIDIOCGDEVINFO,&usbinfo);
  dsyslog(LOG_INFO,"%s: Vendor 0x%04hx, Product 0x%04hx version 0x%04hx\n",
          io->device,usbinfo.vendor,usbinfo.product,usbinfo.version);
  dsyslog(LOG_INFO,"%s: %i application%s on bus %d, devnum %d, ifnum %d\n",
          io->device,usbinfo.num_applications,
          (usbinfo.num_applications==1?"":"s"),usbinfo.busnum,
          usbinfo.devnum,usbinfo.ifnum);
  if (usbinfo.vendor == 0x10bf) {
    syslog(LOG_INFO,"USB Powerlinc version 0x%04hx found\n",usbinfo.version);
  }
  else {
    syslog(LOG_INFO,"%s is not a USB PowerLinc\n",io->device);
    goto error;
  }
  //
  // now send the start command to initiate communications
  //
  if (ioctl(fd, HIDIOCINITREPORT,0) < 0) {
    goto error;
  }
  hidin.rep_info.report_type = HID_REPORT_TYPE_INPUT;
  hidin.rep_info.report_id = HID_REPORT_ID_FIRST;
  while (ioctl(fd, HIDIOCGREPORTINFO, &hidin.rep_info) >= 0) {
    dsyslog(LOG_INFO,"INPUT Report id: %d (%d fields)\n",hidin.rep_info.report_id,hidin.rep_info.num_fields);
    if (delay > 0) usleep(delay*250);
    for (alv = 0; alv < hidin.rep_info.num_fields; alv++) {
      memset(&hidin.field_info,0,sizeof(hidin.field_info));
      hidin.field_info.report_type = hidin.rep_info.report_type;
      hidin.field_info.report_id = hidin.rep_info.report_id;
      hidin.field_info.field_index = alv;
      ioctl(fd, HIDIOCGFIELDINFO, &hidin.field_info);
      dsyslog(LOG_INFO,"INPUT Field: %d: app: %04x phys %04x flags %x (%d usages) unit %x exp %d\n", alv, hidin.field_info.application, hidin.field_info.physical, hidin.field_info.flags, hidin.field_info.maxusage, hidin.field_info.unit, hidin.field_info.unit_exponent);
      if (delay > 0) usleep(delay*250);
      memset(&hidin.usage_ref, 0, sizeof(hidin.usage_ref));
      for (yalv = 0; yalv < hidin.field_info.maxusage; yalv++) {
        hidin.usage_ref.report_type = hidin.field_info.report_type;
        hidin.usage_ref.report_id = hidin.field_info.report_id;
        hidin.usage_ref.field_index = alv;
        hidin.usage_ref.usage_index = yalv;
        ioctl(fd, HIDIOCGUCODE, &hidin.usage_ref);
        ioctl(fd, HIDIOCGUSAGE, &hidin.usage_ref);
        dsyslog(LOG_INFO,"INPUT Usage: %04x val %02x idx %x\n", hidin.usage_ref.usage_code,hidin.usage_ref.value, hidin.usage_ref.usage_index);
        if (delay > 0) usleep(delay*250);
      }
    }
    hidin.rep_info.report_id |= HID_REPORT_ID_NEXT;
  }
  hidout.rep_info.report_type = HID_REPORT_TYPE_OUTPUT;
  hidout.rep_info.report_id = HID_REPORT_ID_FIRST;
  while (ioctl(fd, HIDIOCGREPORTINFO, &hidout.rep_info) >= 0) {
    dsyslog(LOG_INFO,"OUTPUT Report id: %d (%d fields)\n",hidout.rep_info.report_id,hidout.rep_info.num_fields);
    if (delay > 0) usleep(delay*250);
    for (alv = 0; alv < hidout.rep_info.num_fields; alv++) {
      memset(&hidout.field_info,0,sizeof(hidout.field_info));
      hidout.field_info.report_type = hidout.rep_info.report_type;
      hidout.field_info.report_id = hidout.rep_info.report_id;
      hidout.field_info.field_index = alv;
      ioctl(fd, HIDIOCGFIELDINFO, &hidout.field_info);
      dsyslog(LOG_INFO,"OUTPUT Field: %d: app: %04x phys %04x flags %x (%d usages) unit %x exp %d\n", alv, hidout.field_info.application, hidout.field_info.physical, hidout.field_info.flags, hidout.field_info.maxusage, hidout.field_info.unit, hidout.field_info.unit_exponent);
      if (delay > 0) usleep(delay*250);
      memset(&hidout.usage_ref, 0, sizeof(hidout.usage_ref));
      for (yalv = 0; yalv < hidout.field_info.maxusage; yalv++) {
        hidout.usage_ref.report_type = hidout.field_info.report_type;
        hidout.usage_ref.report_id = hidout.field_info.report_id;
        hidout.usage_ref.field_index = alv;
        hidout.usage_ref.usage_index = yalv;
        ioctl(fd, HIDIOCGUCODE, &hidout.usage_ref);
//        ioctl(fd, HIDIOCGUSAGE, &hidout.usage_ref);
        dsyslog(LOG_INFO,"OUTPUT Usage: %04x val %02x idx %x\n", hidout.usage_ref.usage_code,hidout.usage_ref.value, hidout.usage_ref.usage_index);
        if (delay > 0) usleep(delay*250);
      }
    }
    hidout.rep_info.report_id |= HID_REPORT_ID_NEXT;
  }

  dsyslog(LOG_INFO,"Reports initialized...now starting PowerLinc initialization\n");

  // now write the start request
  memset(start,0,sizeof(start));
  start[0] = X10_PLUSB_XMITCTRL;
  start[1] = X10_PLUSB_TCTRLQUALITY;
  start[2] = 0x1e;                        // detect threshold
  do {
    ret = hidwrite(serial,start,sizeof(start));
    if (ret == EBUSY)
      usleep(DELAY*1000);
    else if (ret < 0) {
      syslog(LOG_INFO,"USB Powerlinc initialization failed on setup1 (%s)\n",strerror(errno));
      goto error;
    }
  } while (ret < 0);
  if (delay > 0) usleep(delay*250);
  start[0] = X10_PLUSB_XMITCTRL;
  start[1] = X10_PLUSB_TCTRLRSTCONT       // make sure continuous is off
           | X10_PLUSB_TCTRLRSTFLAG       // turn off flagging services
           | X10_PLUSB_TCTRLRSTRPT        // turn off reporting mode
           | X10_PLUSB_TCTRLWAIT;         // transmit after quiet period
  start[2] = 0;
  do {
    ret = hidwrite(serial,start,sizeof(start));
    if (ret == EBUSY)
      usleep(DELAY*1000);
    else if (ret < 0) {
      syslog(LOG_INFO,"USB Powerlinc initialization failed on setup2 (%s)\n",strerror(errno));
      goto error;
    }
  } while (ret < 0);
  if (delay > 0) usleep(delay*250);
  dsyslog(LOG_INFO,"USB PowerLinc stages 1 and 2 complete\n");
  status = statusrequest();
  if (status < 0)
    goto error;
  syslog(LOG_INFO,"USB Powerlinc Hardware Rev %d.%d found\n",(status>>8)&0xff,status&0xff);

done:
  io->status = 0;
  fakereceive = 1;
  sem_post(&io->connected);
//  if (DELAY < 1000)
//    DELAY = 1000;
  sem_init(&cts,0,1);
  startup();
  return 0;
error:
  io->status = errno;
  sem_post(&io->connected);
  return 1;
}


// xmit_init is the only function that is required.  It must set the xcvrio.send function
// to a non-null value immediately
//
// return a non-zero number to indicate error
int xmit_init(struct xcvrio *arg)
{
  struct pl_cmd data;

  syslog(LOG_INFO,"Transmit thread starting\n");
  sem_init(&sem_ack,0,0);
  io = arg;
  io->status = 1;
  io->send = transmit;
  // Now, connect up and set the status to 0 if successful.  Finally, post to the semaphore to let
  // the parent know that all is well...then just wait for input from the device...
  serial = open(io->device,O_RDWR | O_NOCTTY);
  if (serial < 0) {
    syslog(LOG_INFO,"Error opening %s (%s)\n",io->device,strerror(errno));
    return 1;
  }
  if (hidinit(serial)){
    syslog(LOG_INFO,"Error initializing USB device %s\n",io->device);
    return 1;
  }
  return 0;
}

static unsigned char housecode[] = {
  0x01, 0x02, 0x03, 0x04,
  0x05, 0x06, 0x07, 0x08,
  0x09, 0x0a, 0x0b, 0x0c,
  0x0d, 0x0e, 0x0f, 0x10
};

static unsigned char unitcode[] = {
  0x01, 0x02, 0x03, 0x04,
  0x05, 0x06, 0x07, 0x08,
  0x09, 0x0a, 0x0b, 0x0c,
  0x0d, 0x0e, 0x0f, 0x10
};

static unsigned char functioncode[] = {
  0x0d, 0x05, 0x03, 0x0b,
  0x0f, 0x07, 0x01, 0xff,
  0x0e, 0x0c, 0x04, 0xff,
  0x08, 0x02, 0x0a, 0x06
};

static int decode_housecode(char hc)
{
  int i;

  for (i = 0; i < sizeof(housecode); i++)
    if (hc == housecode[i])
      return i;
  return -1;
}

static int decode_unitcode(char uc)
{
  int i;

  for (i = 0; i < sizeof(unitcode); i++)
    if (uc == unitcode[i])
      return i;
  return -1;
}

static int decode_functioncode(char fc)
{
  int i;

  for (i = 0; i < sizeof(functioncode); i++)
    if (fc == functioncode[i])
      return i;
  return -1;
}

// returns ack if received before timeout
// returns 0 if timeout
static int waitforack(int timeout)
{
  int i;

  dsyslog(LOG_INFO,"waitforack:  timeout %d\n",timeout);
  for (i = 0; i < timeout; i++){
    if (sem_trywait(&sem_ack) == 0){
      dsyslog(LOG_INFO,"received ACK/NAK\n");
      return ack;
    }
    usleep(DELAY*1000);
  }
  return 0;
}

static int updateack(int value)
{
  int data;

  dsyslog(LOG_INFO,"updateack:  %d\n",value);
  ack = value;
  sem_getvalue(&sem_ack,&data);
  if (data == 0)
    sem_post(&sem_ack);
}

static int clearack()
{
  int data = 0;

  dsyslog(LOG_INFO,"clearing ack\n");
  do {
    data = sem_trywait(&sem_ack);
  } while (data == 0);
  ack = 0;
}

static int sem_wait_timeout(sem_t *sem,int timeout)
{
  int i;

  dsyslog(LOG_INFO,"sem_wait_timeout: timeout=%d\n",timeout);
  for (i = 0; i < timeout; i++) {
    if (sem_trywait(sem) != 0) 
      return 0;
    usleep(DELAY*1000);
  }
  dsyslog(LOG_INFO,"timeout waiting for transmitter\n",timeout);
  return 1;
}

int transmit(int hc, int uc, int cmd)
{
  int ret = 0, i = 0;
  struct pl_cmd data;
  char scratch[40];
  __s32 status;

  dsyslog(LOG_INFO,"transmit:  hc=0x%x, uc=0x%x, fc=0x%x\n",hc,uc,cmd);
  if (cmd >= X10_CMD_END){
    dsyslog(LOG_INFO,"transmit:  invalid fc\n");
    return 1;
  }
  hc &= 0x0f;
  memset(&data,0,sizeof(data));
  if (sem_wait_timeout(&cts,timeout)) {
    syslog(LOG_INFO,"Timeout while waiting for transmitter\n");
    syslog(LOG_INFO,"If this persists, restart daemon\n");
    return -1;
  }
  i = 0;
  data.d.byte[i++] = X10_PLUSB_XMIT;
  data.d.byte[i++] = housecode[hc] | 0x40;
  if (uc >= 0){            // negative value for u causes us to skip u
    data.d.byte[i] = unitcode[uc] | 0x40;
    dsyslog(LOG_INFO,"sending %s", dumphex(scratch,(void *) &data, sizeof(data)));
/*    do {
      status = statusrequest();
      if (status < 0)
        goto done;
    } while (status & 0x06000000); */
    usleep(DELAY*1000);
    do {
      ret = hidwrite(serial, (unsigned char *)&data, sizeof(data));
      if (ret == EBUSY)
        usleep(DELAY*1000);
      else if (ret < 0) {
        dsyslog(LOG_INFO,"Error writing to device (%s)\n",strerror(errno));
        goto done;
      }
    } while (ret < 0);
/*    do {
      status = statusrequest();
      if (status < 0)
        goto done;
    } while (status & 0x06000000); */
  }
  // Reuse current transmit data since cmd goes in same place as unit
  if (cmd >= 0){          // negative value for cmd causes us to skip cmd
    data.d.byte[i] = functioncode[cmd] | 0x60;
    dsyslog(LOG_INFO,"sending %s", dumphex(scratch,(void *) &data, sizeof(data)));
    usleep(DELAY*1000);
    do {
      ret = hidwrite(serial, (unsigned char *)&data, sizeof(data));
      if (ret == EBUSY)
        usleep(DELAY*1000);
      else if (ret < 0) {
        dsyslog(LOG_INFO,"Error writing to device (%s)\n",strerror(errno));
        goto done;
      }
    } while (ret < 0);
  }

done:
  sem_post(&cts);			// enable transmitter
  return ret;
}

static void decode(unsigned char *buf, int count)
{
  struct pl_cmd *data;
  int hc, uc, fc;
  char scratch[80];

  if (!buf)
    return;
  dsyslog(LOG_INFO,"received data - decoding:  %s\n", dumphex(scratch, buf, count));
  hc = uc = fc = -1;
  if ((buf[0]&X10_PLUSB_MASK) == X10_PLUSB_RCVCTRL) {
    dsyslog(LOG_INFO,"Control message received");
    updateack(1);
  }
  else if ((buf[0]&X10_PLUSB_MASK) == X10_PLUSB_RCVEDATA) { // ext data
    dsyslog(LOG_INFO,"Error:  Extended data not supported");
  }
  else if ((buf[0]&X10_PLUSB_MASK) == X10_PLUSB_RCV) {
    data = (struct pl_cmd *) buf;
    hc = decode_housecode(data->d.rcv.hc & ~0x40);
    if (hc < 0 || hc >= MAX_HOUSECODES) {
      dsyslog(LOG_INFO,"invalid housecode 0x%x", data->d.rcv.hc);
      return;
    }
    uc = fc = -1;
    if ((data->d.rcv.cmd & 0x60) == 0x40)   // we have a unit
      uc = decode_unitcode(data->d.rcv.cmd & ~0x40);
    else{
      fc = decode_functioncode(data->d.rcv.cmd & ~0x60);
      if (fc == 0xff)
        fc = -1;
    }
    if (io->received(hc, uc, fc,NULL,0))
      dsyslog(LOG_INFO,"unknown cmd %s", dumphex(scratch,buf, count));
  }
  else {
    dsyslog(LOG_INFO,"unable to handle data %s",dumphex(scratch,buf,count));
  }
}

static void startup()
{
  int flags;
  unsigned char c[8];

// set the device into blocking mode now
//  fcntl....
//  flags = fcntl(serial,F_GETFL);
//  flags &= ~O_NONBLOCK;
//  fcntl(serial,F_SETFL,flags);
  while (1) {
    flags = hidread(serial,c,sizeof(c),timeout);
    if (flags > 0){
      decode(c,sizeof(c));
    }
    else if (flags < 0){
      syslog(LOG_INFO,"ERROR:  read error (%s)\n",strerror(errno));
      exit (-1);
    }
//    syslog(LOG_INFO,"Timeout on read\n");
  }
}
