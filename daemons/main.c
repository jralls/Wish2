/*
 *
 * $Id: x10d.c,v 1.4 2004/01/10 19:31:03 whiles Exp whiles $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * non-blocking read utility for reading /dev devices
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <syslog.h>
#include <sched.h>
#include <libgen.h>
#include <asm/types.h>
#include <semaphore.h>

#include "../include/x10.h"
#include "../fs/strings.h"
#include "x10d.h"

#define sighandler_type void

static char *defdevice=DEFDEVICE;		// device API interface
static char *device=DEFDEVICE;			// device API interface
static char *defpidfile=DEFPIDFILE;	// where to store PID number
static char *pidfile=DEFPIDFILE;		// where to store PID number
static int debug=0;				// debug=0:  debug off
static char *progname;				// name used to start program
static char *logtag;				// tag to add to log lines
static int syslog_facility=LOG_LOCAL5;
static int api;					// file handle for the API interface
static int timeout=10;				// timeout for waiting for response from API
static int xmitpid=0;				// pid for transmitter thread
static char *serial=NULL;			// serial device
static void **xmitstack = NULL;			// stack space for transmit thread

char localhostname[MAXHOSTNAMELEN+1];
char *localdomain;

#define dprintf if(debug) printf

#define TESTNEXTARG if(i+2>argc){ syntax(argc,argv,0); exit(1); }

static sighandler_type die(int sig),reapchild();
static void syntax(int argc, char *argv[],int i);
static void write_log(int fd,char *buf,int len);
static void start();
static void listen();
static int update_state(int dir, int hc, int uc, int fc,unsigned char *buf, int len);

static struct state {
  int mode[MAX_HOUSECODES];
  unsigned char status[MAX_HOUSECODES][MAX_UNITS];
  int hc;
  int uc[MAX_HOUSECODES][MAX_UNITS];
  int lastuc;
  int fcsent[MAX_UNITS];
  int fc;
  sem_t lock;
} state;

int main(int argc,char *argv[])
{
  register int i;
  char *arg;

  memset(&state,0,sizeof(state));
  progname = basename(argv[0]);
  logtag = progname;
  for (i = 1; i < argc; i++) {
    arg = argv[i];
    dprintf("Argument %d: %s\n",i,&arg[1]);
    if (arg[0] != '-') {
      syntax(argc,argv,i);
      exit(-1);
    } else if (!strcasecmp(&arg[1],"api")) {
      TESTNEXTARG;
      device = argv[++i];
    } else if (!strcasecmp(&arg[1],"debug")) {
      debug = 1;
    } else if (!strcasecmp(&arg[1],"pid")) {
      TESTNEXTARG;
      pidfile = argv[++i];
    } else if (!strcasecmp(&arg[1],"tag")) {
      TESTNEXTARG;
      logtag = argv[++i];
    } else if (!strcasecmp(&arg[1],"device")) {
      TESTNEXTARG;
      serial = argv[++i];
    } else if (!strcasecmp(&arg[1],"timeout")) {
      TESTNEXTARG;
      timeout = atoi(argv[++i]);
    } else {
      syntax(argc,argv,i);
      exit (-1);
    }
  }
  if (serial == NULL) {
    dprintf("No device specified.  Must have -device <device> present\n");
    exit (-1);
  }
  start();
  return -1;
}

void start()
{
  FILE *fp;
  register char *p;
  struct sigaction action;
  int ret,connected = 0,i;

  // first open up the syslog
  openlog(logtag,LOG_NDELAY | LOG_PID,syslog_facility);

  api = open(device,O_RDWR);
  if (api < 0) {
    char obuffer[256];
    sprintf(obuffer,"Error:  Unable to open device %s (%s)\n",device,strerror(errno));
    syslog(LOG_ERR,obuffer);
    fprintf(stderr,obuffer);
    exit (-1);
  }
  syslog(LOG_INFO,"starting. api-device=%s, pidfile=%s\n",device,pidfile);
  dprintf("%s starting\ndevice=%s\npidfile=%s\nlogtag=%s\n",progname,device,pidfile,logtag);
  // this could be a real function to do the read/write, but I did a null for quick and dirty test
  {
    x10_message_t m;
    memset(&m,0,sizeof(m));
    m.source = X10_API;
    m.command = X10_CMD_STATUS;
    ret = write(api,&m,sizeof(m));
    if (ret < sizeof(m)) {
      syslog(LOG_INFO,"Error:  Unable to write to X10 device driver\n");
      exit(-1);
    }
    for (i = 0; i < timeout; i++) {
      memset(&m,0,sizeof(m));
      if (read(api,&m,sizeof(m)) != sizeof(m))
        syslog(LOG_INFO,"Error:  Incorrect byte count read from %s\n",device);
      if (m.source == X10_API && m.command == X10_CMD_ON) {
        syslog(LOG_INFO,"Successfully opened X10 API device %s\n",device);
        connected = 1;
        break;
      }
    }
    dprintf("X10 message: src=0x%x, hc=0x%x, uc=0x%x, cmd=0x%x, f=0x%x\n",m.source,m.housecode,m.unitcode,m.command,m.flag);
    sleep(1);
  }
  if (connected = 0) {
    syslog(LOG_INFO,"Error:  timeout while connecting to X10 API device %s\n",device);
    exit (-1);
  }

  // successfully connected to the API device
  if (!debug)  // go to daemon mode only if we are not in debug mode
    if (fork())
      exit (0);
  
  gethostname(localhostname,sizeof(localhostname));
  if (p = strchr(localhostname,'.')){
    *p++='\0';
    localdomain = p;
  } else 
    localdomain = "";

  // now set up the sighandlers
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);

  action.sa_handler = debug ? die : SIG_IGN;
  sigaction(SIGTERM,&action,NULL);
  sigaction(SIGINT,&action,NULL);

  action.sa_handler = die;
  sigaction(SIGHUP,&action,NULL);

  action.sa_handler = reapchild;
  sigaction(SIGCHLD,&action,NULL);

  if ((fp = fopen(pidfile,"w"))!= NULL) {
    fprintf(fp,"%d\n",getpid());
    fclose(fp);
  } else {
    syslog(LOG_INFO,"Warning:  Unable to open pid file\n");
  }

  // signal handlers started...device connected, now run run run!!!
  listen();
} 

void syntax(int argc, char *argv[], int i)
{
  if (i == 0)
    fprintf(stderr,"Missing argument\n");
  else
    fprintf(stderr,"Unknown argument:  %s\n",argv[i]);

  fprintf(stderr,"Syntax:  %s [-api device] [-pid pidfile] [-tag logtag] [-debug] -device device\n",progname);
  fprintf(stderr,"         -api device  - X10 device driver api file (default: %s)\n",defdevice);
  fprintf(stderr,"         -pid pidfile - File to write driver PID to (default: %s)\n",defpidfile);
  fprintf(stderr,"         -tag logtag  - Tag to be written to syslog (default: %s)\n",progname);
  fprintf(stderr,"         -timeout #   - Timeout while waiting for response from api (default: 10)\n");
  fprintf(stderr,"         -debug       - Turn on debug mode\n");
  fprintf(stderr,"         -device	- transceiver device file\n");
}

untty()
{
  int i;
  if (!debug) {
    i = open("/dev/tty",O_RDWR);
    if (i >= 0) {
      ioctl(i,(int)TIOCNOTTY,(char *)0);
      close(i);
    }
  }
}


static sighandler_type reapchild()
{
  int status;
  syslog(LOG_INFO,"%s(%s): killing all children\n",progname,logtag);
  dprintf("%s: reapchild()\n",progname);
  kill(xmitpid,SIGHUP);
  while(waitpid(-1,&status,WNOHANG) > 0);
  free(xmitstack);
  xmitstack = NULL;
}

static sighandler_type die(int sig)
{
  register struct filed *f;
  char buf[100];

  if (sig) {
    syslog(LOG_INFO, "exiting on signal %d\n",sig);
    dprintf("%s(%s): exiting on signal %d\n",progname,logtag,sig);
    errno = 0;
  }
  syslog(LOG_INFO, "%s(%s): killing all children\n",progname,logtag);
  dprintf("%s(%s): killing all children\n",progname,logtag);
  if (xmitpid > 0)
    kill(xmitpid,SIGHUP);
  if (xmitstack != NULL){
    free(xmitstack);
    xmitstack = NULL;
  }
  syslog(LOG_INFO, "%s(%s): closing all files and exiting\n",progname,logtag);
  dprintf("%s(%s): closing all files and exiting\n",progname,logtag);
  close(api);
  unlink(pidfile);
  closelog();
  exit(0);
}

static int tty_received(int hc, int uc, int fc, unsigned char *buf, int len)
{
  return(update_state(0,hc,uc,fc,buf,len));
}

xcvrio_t xcvrio = {
  received:	tty_received,	// callback function for transmitter to call when data is available
};

static void listen()
{
  int n;
  x10_message_t m;

  sem_init(&start.lock,1,1);
  xcvrio.device = serial;
  xcvrio.debug = debug;
  xcvrio.logtag = logtag;
  xmitstack = (void **)malloc(1024*16);
  xmitpid = clone((int (*)(void *))xmit_init,xmitstack,CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_PTRACE | CLONE_VM,(void *)&xcvrio);
  if (xmitpid < 0) {
    syslog(LOG_INFO, "%s(%s): unable to clone, exiting - %s\n",progname,logtag,strerror(errno));
    unlink(pidfile);
    closelog();
    free(xmitstack);
    exit(-1);
  }

  // physical device transmitter started...now listen to the api
  while (1) {
    memset(&m,0,sizeof(m));
    n = read(api,&m,sizeof(m));
    if (n < 0) {
      syslog(LOG_INFO,"%s(%s): Error reading API interface - %s\n",progname,logtag,strerror(errno));
      dprintf("%s:  Error reading API interface, continuing...\n");
      continue;
    }
    if (n != sizeof(m)) {
      syslog(LOG_INFO,"%s(%s): Error - incorrect API byte count, got %d, should have gotten %d\n",progname,logtag,n,sizeof(m));
      dprintf("%s: Error - incorrect API byte count, got %d, should have gotten %d\n",progname,n,sizeof(m));
      continue;
    }
    dprintf("X10 message: src=0x%x, hc=0x%x, uc=0x%x, cmd=0x%x, f=0x%x\n",m.source,m.housecode,m.unitcode,m.command,m.flag);
    switch (m.source) {
      case X10_API:
        break;
      case X10_DATA:
      case X10_CONTROL:
        xmit_send(m.housecode,m.unitcode,m.command);
        update_state(1,m.housecode,m.unitcode,m.command,NULL,0);
        break;
      default:
        dprintf("%s: Error - invalid API source %d\n",progname,m.source);
        break;
    }
  } // while(1) loop
}

static int updatestatus(int hc, int uc, int value)
{
  x10_message_t m;
  memset(&m,0,sizeof(m));
  m.source = X10_DATA;
  m.housecode = hc;
  m.unitcode = uc;
  m.command = value;
  if (write(api,&m,sizeof(m)) == sizeof(m)){
    state.uc[hc][uc] = value;
    return 0;
  }
  else{
    syslog(LOG_INFO,"%s(%s): Error - write did not complete\n",progname,logtag);
    dprintf("%s: Error - write did not complete, incorrect byte count\n",progname);
    return -1;
  }
}

static void clear_uc(int hc)
{
  int i;
  for (i = 0; i < MAX_UNITS; i++)
    state.uc[hc][i] = 0;
}

unsigned char preset_low[16]= {19,23,13,16,26,29,32,35,45,48,39,42,0,3,6,10};
unsigned char preset_high[16]={71,74,65,68,77,81,84,87,97,100,90,94,52,55,58,61};

static int update_state(int dir, int hc, int uc, int fc,unsigned char *buf, int len)
{
  int i,ret=0;

  dprintf("dir=%d, hc=0x%x, uc=0x%x, fc=0x%x\n",dir,hc,uc,fc);
  hc &= 0x0f;
  uc &= 0x0f;

  sem_wait(&state.lock);

  // if we get a preset dim high/low, we can store the level only if:
  //   the previous housecode was stored
  //   the previous message received was not a function (had to be unit)
  //   a valid unit is stored
  if ((fc == X10_CMD_PRESETDIMHIGH || fc == X10_CMD_PRESETDIMLOW)
      && (state.hc>=0 && state.hc < MAX_HOUSECODES)
      && (state.fcsent[state.hc] != 1)
      && (state.lastuc != -1)) {
    updatestatus(state.hc,state.lastuc,fc == X10_CMD_PRESETDIMLOW ? preset_low[hc] : preset_high[hc]);
  }
  if (hc != state.hc) {    // restart if new housecode received
    state.hc = hc;
    state.lastuc = -1;
    state.fc = -1;
  }
  if (uc >= 0 && uc < MAX_UNITS) {
    if (state.fcsent[state.hc]) {
      clear_uc(state.hc);
      state.fcsent[state.hc] = 0;
    }
    state.uc[state.hc][uc] = 1;
    state.lastuc = uc;
  }
  log_store(dir, hc, uc, fc);
  // something just changed so now update the queue head to wake everyone up

  if (fc >= 0) {
    // we have a completed command
    // we may never have received a unit code so it may still be -1
//              if (fc != X10_CMD_STATUS)
      state.fcsent[state.hc] = 1;
    state.fc = fc;
    switch (fc) {
    case X10_CMD_ALLLIGHTSOFF:      // we don't know which ones are lights
    case X10_CMD_ALLUNITSOFF:
      for (i = 0; i < MAX_HOUSECODES; i++) {
        updatestatus(state.hc,i,0);
      }
      break;
    case X10_CMD_ALLLIGHTSON:
      for (i = 0; i < MAX_HOUSECODES; i++) {
        updatestatus(state.hc,i,100);
      }
      break;
    case X10_CMD_ON:
      for (i = 0; i < MAX_UNITS; i++)
        if (state.uc[state.hc][i]) {
          updatestatus(state.hc,i,100);
        }
      break;
    case X10_CMD_OFF:
      for (i = 0; i < MAX_UNITS; i++)
        if (state.uc[hc][i]) {
          updatestatus(state.hc,i,0);
        }
      break;
    case X10_CMD_DIM:
      for (i = 0; i < MAX_UNITS; i++)
        if (state.uc[hc][i]) {
          int value = state.status[state.hc][i];
          value -= 6;
          if (value < 0)
            value = 0;
          updatestatus(state.hc,i,value);
        }
      break;
    case X10_CMD_BRIGHT:
      for (i = 0; i < MAX_UNITS; i++)
        if (state.uc[hc][i]) {
          int value = state.status[state.hc][i];
          value += 6;
          if (value > 100)
            value = 100;
          updatestatus(state.hc,i,value);
        }
      break;
    case X10_CMD_STATUS:
      dbg("status request to housecode %c",'A'+state.hc);
      break;
    case X10_CMD_STATUSOFF:
      dbg("status of %c%d is OFF",'A'+state.hc,state.lastuc+1);
      if (state.hc < MAX_HOUSECODES && state.lastuc < MAX_UNITS) {
        updatestatus(state.hc,state.lastuc,0);
      }
      break;
    case X10_CMD_STATUSON:
      dbg("status of %c%d is ON",'A'+state.hc,state.lastuc+1);
      if (state.hc < MAX_HOUSECODES && state.lastuc < MAX_UNITS) {
        updatestatus(state.hc,state.lastuc,100);
      }
      break;
    case X10_CMD_HAILREQUEST:
      dbg("hail request to housecode %c",'A'+state.hc);
      break;
    case X10_CMD_HAILACKNOWLEDGE:
      dbg("hail ACK to housecode %c",'A'+state.hc);
      break;
    case X10_CMD_EXTENDEDCODE:      // extended code
      dbg("extended code to housecode %c",'A'+state.hc);
      if (len > 0){
        dbg("%s",dumphex(buf,len));
        edata_store(state.hc,buf,len);
      }
      break;
    case X10_CMD_EXTENDEDDATAA:     // extended data (analog)
      dbg("extended data (analog) to housecode %c", 'A' + state.hc);
      if (len > 0){
        dbg("%s",dumphex(buf,len));
        edata_store(state.hc,buf,len);
      }
      break;
    case X10_CMD_PRESETDIMHIGH:     // already handled
      dbg("preset dim low %c%d",'A'+state.hc,state.lastuc);
      break;
    case X10_CMD_PRESETDIMLOW:      // already handled
      dbg("preset dim low %c%d",'A'+state.hc,state.lastuc);
      break;
    default:
      ret = -1;
      break;
    }  // end of switch statement
  } // end of if (fc >= 0)
  sem_post(&state.lock);
  return ret;
}
