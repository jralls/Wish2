/*
 *
 * $Id: pl_xcvr.c,v 1.2 2004/01/11 21:18:54 whiles Exp whiles $
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

#include "../include/x10.h"
#include "x10d.h"
#include "pl_xcvr.h"

int transmit(int hc, int uc, int cmd);
static void start();
int serial=0;
int debug=0;

int readchar(int fd, unsigned char *c, int timeout)
{
  struct timeval tv;
  fd_set set;

  tv.tv_sec = 0;
  tv.tv_usec = timeout * 1000;
  FD_ZERO(&set);
  FD_SET(fd, &set);
  if (!select(fd + 1, &set, NULL, NULL, &tv)) 
    return -1;
  if (read(fd, c, 1) != 1) 
    return -1;
  return 0;
}

void setline(int fd, int flags, int speed)
{
  struct termios t;

  tcgetattr(fd, &t);

  t.c_cflag = flags | CREAD | HUPCL | CLOCAL;
  t.c_iflag = IGNBRK | IGNPAR;
  t.c_oflag = 0;
  t.c_lflag = 0; 
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  cfsetispeed(&t, speed);
  cfsetospeed(&t, speed);

  tcsetattr(fd, TCSANOW, &t);
}

struct xcvrio *io = NULL;
sem_t sem_ack;
int ack;

// xmit_init is the only function that is required.  It must set the xcvrio.send function
// to a non-null value immediately
//
// return a non-zero number to indicate error
int xmit_init(struct xcvrio *arg)
{
  unsigned char d,cr;
  char buf[15];
  ssize_t res;

  syslog(LOG_INFO,"Transmit thread starting\n");
  sem_init(&sem_ack,0,0);
  io = arg;
  io->status = 1;
  io->send = transmit;
  debug = io->debug;
  // Now, connect up and set the status to 0 if successful.  Finally, post to the semaphore to let
  // the parent know that all is well...then just wait for input from the device...
  serial = open(io->device,O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serial < 0) {
    syslog(LOG_INFO,"Error opening %s (%s)\n",io->device,strerror(errno));
    goto error;
  }
  setline(serial,CS8,B9600);	// specific to the device
  while(!readchar(serial,&d,100));
  syslog(LOG_INFO,"Connecting to Powerlinc Serial Interface on %s\n",io->device);
  d = 0x02;
  res = write(serial,&d,1);
  if (res < 1) {
    syslog(LOG_INFO,"Error write to %s (%s)\n",io->device,strerror(errno));
    goto error;
  }
  if (readchar(serial,&d,1000)) {
    syslog(LOG_INFO,"Timeout reading from %s\n",io->device);
    goto error;
  }
  readchar(serial,&cr,1000);
  if (d != 0x06) {
    syslog(LOG_INFO,"PowerLinc serial module found, but not ready\n");
    goto error;
  }
  d = 'g';
  res = write(serial,&d,1);
  readchar(serial,&buf[0],1000);
  readchar(serial,&buf[1],1000);
  readchar(serial,&buf[2],1000);
  buf[3] = '\0';
  syslog(LOG_INFO,"Powerinc version %s found\n",buf);

done:
  io->status = 0;
  sem_post(&io->connected);
  start();
  return 0;
error:
  io->status = errno;
  sem_post(&io->connected);
  return 1;
}

static unsigned char housecode[] = {
  0x06, 0x0e, 0x02, 0x0a,
  0x01, 0x09, 0x05, 0x0d,
  0x07, 0x0f, 0x03, 0x0b,
  0x00, 0x08, 0x04, 0x0c
};

static unsigned char unitcode[] = {
  0x0c, 0x1c, 0x04, 0x14,
  0x02, 0x12, 0x0a, 0x1a,
  0x0e, 0x1e, 0x06, 0x16,
  0x00, 0x10, 0x08, 0x18
};

static unsigned char functioncode[] = {
  0x01, 0x03, 0x05, 0x07,
  0x09, 0x0b, 0x0d, 0x0f,
  0x11, 0x17, 0x15, 0x19,
  0x1b, 0x1d, 0x1f, 0x13
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

  for (i = 0; i < timeout; i++){
    if (sem_trywait(&sem_ack) == 0)
      return ack;
    usleep(1);
  }
  return 0;
}

static int updateack(int value)
{
  int data;

  ack = value;
  sem_getvalue(&sem_ack,&data);
  if (data == 0)
    sem_post(&sem_ack);
}

int transmit(int hc, int uc, int cmd)
{
  int ret, i;
  unsigned char data[5];
  char scratch[40];

  if (cmd >= X10_CMD_END)
    return 1;
  hc &= 0x0f;
  uc &= 0x0f;
  memset(data,0,sizeof(data));
  i = 0;
  data[i++] = X10_PL_SENDCMD;
  data[i++] = housecode[hc] | 0x40;
  if (uc >= 0)
    data[i++] = unitcode[uc] | 0x40;
  if (cmd >= 0)
    data[i++] = functioncode[cmd] | 0x40;
  data[4] = X10_PL_REPEATONCE;
  syslog(LOG_INFO,"sending %s\n",dumphex(scratch,(void *)data,sizeof(data)));
  ret = write(serial,data,sizeof(data));
  if (!waitforack(5000))
    syslog(LOG_INFO,"timeout waiting for ACK\n");
  return ret;
}

static void decode(unsigned char *buf, int count)
{
  struct pl_cmd *data;
  int hc, uc, fc;
  char scratch[80];

  if (!buf)
    return;
  dsyslog(LOG_INFO,"%s", dumphex(scratch, buf, count));
  hc = uc = fc = -1;
  if (buf[0] == X10_PL_ACK) 
    updateack(1);

  else if (buf[0] == X10_PL_NAK) 
    updateack(-1);

  else if (buf[0] == X10_PL_EXTDATASTART)        // 0x45='E'
    dsyslog(LOG_INFO,"Error:  Extended data not supported");

  else if (buf[0] == X10_PL_XMITRCV || buf[0] == X10_PL_XMITRCVSELF) {
    data = (struct pl_cmd *) buf;
    hc = decode_housecode(data->hc & ~0x40);
    if (hc < 0 || hc >= MAX_HOUSECODES) {
      dsyslog(LOG_INFO,"invalid housecode 0x%x", data->hc);
      return;
    }
    uc = decode_unitcode(data->uc & ~0x40);
    if (uc < 0) 
      fc = decode_functioncode(data->uc & ~0x40);

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
  int index=0;
  unsigned char buf[48],c;

  while (1) {
    if (!readchar(serial,&c,50)){
      buf[index++] = c;
      if (index >= sizeof(buf) || c == X10_PL_CR) {
        decode(buf,index);
        index = 0;
      }
    }
  }
}
