/*
 *
 * $Id: cm11a_xcvr.c,v 1.3 2004/02/21 02:21:20 whiles Exp whiles $
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

#include "x10.h"
#include "x10d.h"
#include "cm11a_xcvr.h"

#define min(a,b) ((a) < (b) ? (a) : (b))

int transmit(int hc, int uc, int cmd);
static void start();
int serial=0;

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
sem_t cts;

// encodes time into buffer for writing to cm11a
char *cm11a_encodetime(char *buf)
{
  time_t t;
  struct timeval tv;
  struct tm tm;

  gettimeofday(&tv,0);
  t = (time_t)tv.tv_sec;
  localtime_r(&t,&tm);

  buf[0] = CM11A_POWERFAILPOLLACK;
  buf[1] = tm.tm_sec;
  buf[2] = tm.tm_min + (((tm.tm_hour) & 0x01) * 60);
  buf[3] = tm.tm_hour >> 1;
  buf[4] = tm.tm_yday & 0xff;
  buf[5] = ((tm.tm_yday & 0x100) >> 1) & 0xff;
  buf[5] |= (1 << tm.tm_wday);
  buf[6] = 0;

  return buf;
}

// parses batter buffer to extract information
time_t cm11a_decodebattery(char *buf)
{
  time_t t;
  struct timeval tv;
  struct tm tm;

  int battery, rev, h, m, s, jday, day, hcode, addmon, statmon, statdim;
  int i;

  // get long value for January 1 of the current year
  gettimeofday(&tv,0);
  t = (time_t)tv.tv_sec;
  localtime_r(&t,&tm);
  tm.tm_sec = 0;
  tm.tm_min = 0;
  tm.tm_hour = 0;
  tm.tm_mday = 0;
  tm.tm_mon = 0;
  tm.tm_wday = 0;
  tm.tm_yday = 0;
  t = mktime(&tm);

  // discect the buffer
  battery = ((buf[0] << 8) | (unsigned int) buf[0]);
  if (battery == 0xffff)
    battery = 0;
  s = buf[2] & 0xff;
  m = (buf[3] & 0xff) % 60;
  h = ((buf[4] & 0xff) * 2) + ((buf[3] & 0xff) / 60);
  jday = buf[5] + (((buf[6] & 0x80) >> 7) * 256);         // day of year
  t += (time_t)(jday+1)*24*60*60;
  t += (time_t)h*60*60;
  t += (time_t)m*60;
  t += (time_t)s;

  day = buf[6] & 0x7f;                                    // day mask
  hcode = (buf[7] & 0xf0) >> 4;                           // mon'd HC
  rev = buf[7] & 0x0f;                                    // revision
  addmon = buf[8] | (buf[9] << 8);                        // addr mon'd
  statmon = buf[10] | (buf[11] << 8);                     // stat of mon'd
  statdim = buf[12] | (buf[13] << 8);                     // stat of dim'd
  syslog(LOG_INFO,"cm11a version %d found\n", rev);
  return t;
}


// xmit_init is the only function that is required.  It must set the xcvrio.send function
// to a non-null value immediately
//
// return a non-zero number to indicate error
int xmit_init(struct xcvrio *arg)
{
  unsigned char d;
  char buf[32];
  ssize_t res;
  int ret,i,j;
  time_t t;

  syslog(LOG_INFO,"Transmit thread starting\n");
  sem_init(&sem_ack,0,0);
  io = arg;
  io->status = 1;
  io->send = transmit;
  fakereceive = 1;
  // Now, connect up and set the status to 0 if successful.  Finally, post to the semaphore to let
  // the parent know that all is well...then just wait for input from the device...
  serial = open(io->device,O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (serial < 0) {
    syslog(LOG_INFO,"Error opening %s (%s)\n",io->device,strerror(errno));
    goto error;
  }
  setline(serial,CS8,B4800);	// specific to the device
  lseek(serial,0,SEEK_END);
  for (i = 0; i < retries; i++) {
    while(!readchar(serial,&d,100)) {
      if (d == CM11A_POLL) {
        dsyslog(LOG_INFO,"xmit_init:  poll received\n");
        d = CM11A_POLLACK;
        res = write(serial,&d,1);
      }
      else if (d == CM11A_MACROPOLL) {
        dsyslog(LOG_INFO,"xmit_init:  macro poll received\n");
        d = CM11A_MACROPOLLACK;
        res = write(serial,&d,1);
      }
      else if (d == CM11A_POWERFAILPOLL) {
        dsyslog(LOG_INFO,"xmit_init:  received time request from CM11A\n");
        write(serial,cm11a_encodetime(buf),7);
        dsyslog(LOG_INFO,"xmit_init:  Wrote %02x %02x %02x %02x %02x %02x %02x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6]);
      }
      d = 0x8b;
      res = write(serial,&d,1);
      if (res < 1) {
        dsyslog(LOG_INFO,"xmit_init:  unable to write to %s (%s)\n",io->device,strerror(errno));
        goto error;
      }
      for (j = 0; j < 14; j++) {
        ret = readchar(serial,&buf[j],timeout);
        if (ret)
          break;
      }
    }
  }
  if (ret) {
    syslog(LOG_INFO,"Unable to establish communications with CM11A transceiver\n");
    goto error;
  }
  t = cm11a_decodebattery(buf);
  syslog(LOG_INFO,"CM11A current date/time %s\n",ctime(&t));

done:
  io->status = 0;
  sem_post(&io->connected);
//  if (delay < 1000)
//    delay = 1000;
  sem_init(&cts,0,1);
  start();
  return 0;
error:
  io->status = errno;
  sem_post(&io->connected);
  return 1;
}

static unsigned char housecode[] = {
  0x60, 0xe0, 0x20, 0xa0,
  0x10, 0x90, 0x50, 0xd0,
  0x70, 0xf0, 0x30, 0xb0,
  0x00, 0x80, 0x40, 0xc0
};

static unsigned char unitcode[] = {
  0x06, 0x0e, 0x02, 0x0a,
  0x01, 0x09, 0x05, 0x0d,
  0x07, 0x0f, 0x03, 0x0b,
  0x00, 0x08, 0x04, 0x0c
};

static unsigned char functioncode[] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x0b, 0x0a, 0x0c,
  0x0d, 0x0e, 0x0f, 0x09
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
  return -1;
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
    usleep(1000);
  }
  dsyslog(LOG_INFO,"timeout waiting for transmitter\n",timeout);
  return 1;
}

static int sendit(unsigned char c1, unsigned char c2, unsigned char *buf, int len)
{
  int i, checksum, response, ret = 0, xmitlen;
  unsigned char xmitbuf[16];
  char scratch[80];

  xmitbuf[0] = c1;
  xmitbuf[1] = c2;
  xmitlen = 2+len;
  for (i = 0; i < len; i++)
    xmitbuf[2+i] = buf[i];
  checksum = 0;
  for (i = 0; i < xmitlen; i++)
    checksum += xmitbuf[i];
  checksum &= 0xff;
  dsyslog(LOG_INFO,"sending %s", dumphex(scratch, (void *) xmitbuf, xmitlen));
  if (sem_wait_timeout(&cts,timeout)) {
    syslog(LOG_INFO,"Timeout while waiting for transmitter\n");
    syslog(LOG_INFO,"If this persists, restart daemon\n");
    return -1;
  }
  for (i = 0; i < retries; i++) { // try to get correct checksum
    ret = write(serial, xmitbuf, xmitlen);
    response = waitforack(timeout);    // return -1 if timed out
    if (response < 0){
      ret = -1;
      goto done;
    }
    if ((response & 0xff) == checksum)
      break;
  }
  if (i >= retries){       // failed to get valid checksum in "retries"
    ret = -1;
    goto done;
  }
  xmitbuf[0] = CM11A_OK;
  usleep(1000);            // make sure you don't send the handshake to fast
  ret = write(serial, xmitbuf, 1); // handshake done
  if (ret < 0) {
    dsyslog(LOG_INFO,"Error sending OK to CM11A\n");
    goto done;
  }
  dsyslog(LOG_INFO,"sent CM11A_OK (0x00)");
  response = waitforack(timeout);    // wait for a 0x00 to be returned;
  if (response == CM11A_POLL)  // this is valid...
    goto done;
  if (response != CM11A_READY) {  // something went wrong...sequencing?
    dsyslog(LOG_INFO,"Expected 0x55 and got 0x%x", response);
  }
  else{
    dsyslog(LOG_INFO,"%s","got CM11A_READY (0x55)");
  }
done:
  sem_post(&cts);
  return ret;
}

int transmit(int hc, int uc, int cmd)
{
  int ret, i;
  char scratch[40];

  dsyslog(LOG_INFO,"transmit:  hc=0x%x, uc=0x%x, fc=0x%x\n",hc,uc,cmd);
  if (cmd >= X10_CMD_END){
    dsyslog(LOG_INFO,"transmit:  invalid fc\n");
    return 1;
  }
  if (sem_wait_timeout(&cts,timeout)) {
    syslog(LOG_INFO,"Timeout while waiting for transmitter\n");
    syslog(LOG_INFO,"If this persists, restart daemon\n");
    return -1;
  }
  if (uc >= 0) {
    if (sendit(CM11A_HEADER|CM11A_ADDRESS,housecode[hc]|unitcode[uc],NULL,0)<0){
      ret = -1;
      goto error;
    }
  }
  dsyslog(LOG_INFO,"sleeping for %d microseconds\n",delay);
  usleep(delay*1000);
  if (cmd >= 0) {
    if (sendit(CM11A_HEADER|CM11A_FUNCTION,housecode[hc]|functioncode[cmd],NULL,0)<0) {
      ret = -1;
      goto error;
    }
  }
  dsyslog(LOG_INFO,"sleeping for %d microseconds\n",delay);
  usleep(delay*1000);
error:
  sem_post(&cts);			// enable transmitter
  return ret;
}

static void decode(unsigned char *buf, int count)
{
  int hc, uc, fc, i, bytecount;
  unsigned char *pbuf,headermask;
  char scratch[80];

  dsyslog(LOG_INFO,"received data - decoding:  %s\n", dumphex(scratch, buf, count));
  hc = uc = fc = -1;
  if (buf[0] == CM11A_POLL) {     // tells us this is a data packet
    bytecount = buf[1]&0x0f;
    headermask = buf[2];
    pbuf = &buf[3];
    for (i = 0; i < bytecount-1; i++){
      uc = fc = -1;
      hc = decode_housecode(pbuf[i] & 0xf0);
      if (headermask & (0x01<<i))
        fc = decode_functioncode(pbuf[i] & 0x0f);
      else
        uc = decode_unitcode(pbuf[i] & 0x0f);
      if (io->received(hc, uc, fc, NULL, 0))
        dsyslog(LOG_INFO,"unknown cmd %s", dumphex(scratch,buf, count));
    }
  }
  else {
    dsyslog(LOG_INFO,"unable to handle data %s",dumphex(scratch,buf,count));
  }
}

static void start()
{
  int n,ret,length;
  int index=0,flags;
  unsigned char buf[48],c,a;
  static int receivemacro = 0;

// set the device into blocking mode now
//  fcntl....
  flags = fcntl(serial,F_GETFL);
  flags &= ~O_NONBLOCK;
  fcntl(serial,F_SETFL,flags);
  while (1) {
    c = 0;
    flags = read(serial,&c,1);
    if (flags < 0){
      syslog(LOG_INFO,"ERROR:  read error (%s)\n",strerror(errno));
      exit (-1);
    }
    else if (flags > 0){
      if (updateack(c) && c != CM11A_POLL)
        continue;
      if (c == CM11A_POLL) {
        dsyslog(LOG_INFO,"CM11A_POLL");
          index = 0;
          a = CM11A_POLLACK;
          ret = write(serial, &a, 1);      // acknowledge the poll so that data starts
          dsyslog(LOG_INFO,"sent CM11A_POLLACK");
       } else if (c == CM11A_MACROPOLL) {      // macro report coming in
         dsyslog(LOG_INFO,"MACRO_POLL");
         receivemacro = 1;
         index = 0;
       } else if (c == CM11A_POWERFAILPOLL) {  // power fail, macro download poll
         dsyslog(LOG_INFO,"POWERFAIL_POLL");
         index = 0;
         a = CM11A_POWERFAILPOLLACK;
         ret = write(serial,&a,1);       // acknowledge the poll
         dsyslog(LOG_INFO,"sent CM11A_POWERFAILPOLLACK");
       }

       buf[index++] = c;   //go ahead and store the poll character

       if (receivemacro) {
         if (index == 3) { // macro read complete
           index = 0;
           dsyslog(LOG_INFO,"MACRO RECEIVED");
           receivemacro = 0;
         }
       }
       else if (index > 1 && buf[0] == CM11A_POLL) {
         // we have at least received the first byte (upload size)
         // remember [0] has CM11A_POLL request
         length = (int) buf[1];
         // we really have length+1...only decode if it is a POLL
         if (index >= length+2){
           index = min(index,16);
           index = 0;
           decode(buf, length + 2);
         }
       }
    }
  }
}
