/*
 *
 * $Id: pl_xcvr.c,v 1.7 2004/01/17 20:50:47 whiles Exp whiles $
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

#include <linux/hiddev.h>

#include "../include/x10.h"
#include "x10d.h"
#include "plusb_xcvr.h"

int transmit(int hc, int uc, int cmd);
static void start();
int serial=0;

struct xcvrio *io = NULL;
sem_t sem_ack;
int ack;
sem_t cts;

int readpacket(int fd,char *c,int timeout)
{
  struct timeval tv;
  fd_set set;
  int ret;

  tv.tv_sec = 0;
  tv.tv_usec = timeout*1000;
  FD_ZERO(&set);
  FD_SET(fd,&set);
  ret = select(fd+1,&set,NULL,NULL,&tv);
  if (ret != 0)
    return ret;
  ret = read(fd,c,8);
  return ret;
}

// xmit_init is the only function that is required.  It must set the xcvrio.send function
// to a non-null value immediately
//
// return a non-zero number to indicate error
int xmit_init(struct xcvrio *arg)
{
  ssize_t ret;
  struct hiddev_devinfo usbinfo;
  __u8 start[8];

  syslog(LOG_INFO,"Transmit thread starting\n");
  sem_init(&sem_ack,0,0);
  io = arg;
  io->status = 1;
  io->send = transmit;
  // Now, connect up and set the status to 0 if successful.  Finally, post to the semaphore to let
  // the parent know that all is well...then just wait for input from the device...
  serial = open(io->device,O_RDWR | O_NONBLOCK);
  if (serial < 0) {
    syslog(LOG_INFO,"Error opening %s (%s)\n",io->device,strerror(errno));
    goto error;
  }
  dsyslog(LOG_INFO,"Connecting to USB Powerlinc Interface on %s\n",io->device);
  ioctl(serial,HIDIOCGDEVINFO,&usbinfo);
  dsyslog(LOG_INFO,"%s: Vendor 0x04hx, Product 0x%04hx version 0x%04hx\n",io->device,usbinfo.vendor,usbinfo.product,usbinfo.version);
  dsyslog(LOG_INFO,"%s: %i application%s on bus %d, devnum %d, ifnum %d\n",io->device,usbinfo.num_applications,(usbinfo.num_applications==1?"":"s"),usbinfo.busnum,usbinfo.devnum,usbinfo.ifnum);
  syslog(LOG_INFO,"USB Powerinc version 0x%04hx found\n",usbinfo.version);
  memset(start,0,sizeof(start));
  start[0] = X10_PLUSB_XMITCTRL;
  start[1] = X10_PLUSB_TCTRLQUALITY;
  start[2] = 0x1e;                        // detect threshold
  ret = write(serial,start,8);
  if (ret < 0) {
    syslog(LOG_INFO,"USB Powerlinc initialization failed on setup1 (%s)\n",strerror(errno));
    goto error;
  }
  start[1] = X10_PLUSB_TCTRLRSTCONT       // make sure continuous is off
           | X10_PLUSB_TCTRLRSTFLAG       // turn off flagging services
           | X10_PLUSB_TCTRLRSTRPT        // turn off reporting mode
           | X10_PLUSB_TCTRLWAIT;         // transmit after quiet period
  start[2] = 0;
  ret = write(serial,start,8);
  if (ret < 0) {
    syslog(LOG_INFO,"USB Powerlinc initialization failed on setup2 (%s)\n",strerror(errno));
    goto error;
  }
  usleep(delay*1000);
  start[0] = X10_PLUSB_XMITCTRL;
  start[1] = X10_PLUSB_TCTRLSTATUS;       // request status response
  ret = write(serial,start,8);
  if (ret < 0) {
    syslog(LOG_INFO,"USB Powerlinc initialization failed on status request (%s)\n",strerror(errno));
    goto error;
  }
  ret = readpacket(serial,start,timeout);
  if (ret < 0) {
    syslog(LOG_INFO,"USB Powerlinc initialization failed to return status (%s)\n",strerror(errno));
    goto error;
  }
  syslog(LOG_INFO,"USB Powerlinc Hardware Rev %d.%d found\n",start[3],start[4]);
  goto error;

done:
  io->status = 0;
  sem_post(&io->connected);
  if (delay < 1000)
    delay = 1000;
  sem_init(&cts,0,1);
//  start();
  return 0;
error:
  io->status = errno;
  sem_post(&io->connected);
  return 1;
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
    usleep(1000);
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

  dsyslog(LOG_INFO,"sem_wait_timeout: timeout %d\n",timeout);
  for (i = 0; i < timeout; i++) {
    if (sem_trywait(sem) != 0) 
      return 0;
    usleep(1000);
  }
  dsyslog(LOG_INFO,"timeout waiting for transmitter\n",timeout);
  return 1;
}

int transmit(int hc, int uc, int cmd)
{
  int ret, i;
  struct pl_cmd data;
  char scratch[40];

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
    ret = write(serial, &data, sizeof(data));
    if (!waitforack(timeout)){
      syslog(LOG_INFO,"timeoutwaiting for ACK from USB PowerLinc\n");
      ret = -1;
      goto done;
    }
  }
  // Reuse current transmit data since cmd goes in same place as unit
  if (cmd >= 0){          // negative value for cmd causes us to skip cmd
    data.d.byte[i] = functioncode[cmd] | 0x60;
    dsyslog(LOG_INFO,"sending %s", dumphex(scratch,(void *) &data, sizeof(data)));
    ret = write(serial, &data, sizeof(data));
    if (!waitforack(timeout)){
      syslog(LOG_INFO,"timeoutwaiting for ACK from USB PowerLinc\n");
      ret = -1;
      goto done;
    }
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

static void start()
{
  int n;
  int index=0,flags;
  unsigned char buf[48],c[9];

// set the device into blocking mode now
//  fcntl....
  flags = fcntl(serial,F_GETFL);
  flags &= ~O_NONBLOCK;
  fcntl(serial,F_SETFL,flags);
  while (1) {
    flags = read(serial,&c,8);
    if (flags > 0){
      decode(c,8);
    }
    else if (flags < 0){
      syslog(LOG_INFO,"ERROR:  read error (%s)\n",strerror(errno));
      exit (-1);
    }
  }
}
