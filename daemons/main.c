/*
 *
 * $Id: x10d.c,v 1.3 2004/01/06 03:45:51 whiles Exp whiles $
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
#include <libgen.h>
#include <asm/types.h>

#include "../x10.h"
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

char localhostname[MAXHOSTNAMELEN+1];
char *localdomain;

#define dprintf if(debug) printf

#define TESTNEXTARG if(i+2>argc){ syntax(argc,argv,0); exit(1); }

static sighandler_type die(int sig),reapchild();
static void syntax(int argc, char *argv[],int i);
static void write_log(int fd,char *buf,int len);
static void start();

int main(int argc,char *argv[])
{
  register int i;
  char *arg;

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
    } else if (!strcasecmp(&arg[1],"timeout")) {
      TESTNEXTARG;
      timeout = atoi(argv[++i]);
    } else {
      syntax(argc,argv,i);
      exit (-1);
    }
  }
  start();
  return -1;
}

void start()
{
  FILE *fp;
  register char *p;
  struct sigaction action;
  x10_message_t m;
  int ret,connected = 0,i;

  // first open up the syslog
  openlog(logtag,LOG_NDELAY | LOG_PID,syslog_facility);

  // open our API device
//  api = open(device,O_RDWR | O_NONBLOCK);
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
    syslog(LOG_INFO,"X10 message: src=0x%x, hc=0x%x, uc=0x%x, cmd=0x%x, f=0x%x\n",m.source,m.housecode,m.unitcode,m.command,m.flag);
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
  }

  // signal handlers started...device connected, now run run run!!!
 
} 

void syntax(int argc, char *argv[], int i)
{
  if (i == 0)
    fprintf(stderr,"Missing argument\n");
  else
    fprintf(stderr,"Unknown argument:  %s\n",argv[i]);

  fprintf(stderr,"Syntax:  %s [-api device] [-pid pidfile] [-tag logtag] [-debug]\n",progname);
  fprintf(stderr,"         -api device  - X10 device driver api file (default: %s)\n",defdevice);
  fprintf(stderr,"         -pid pidfile - File to write driver PID to (default: %s)\n",defpidfile);
  fprintf(stderr,"         -tag logtag  - Tag to be written to syslog (default: %s)\n",progname);
  fprintf(stderr,"         -timeout #   - Timeout while waiting for response from api (default: 10)\n");
  fprintf(stderr,"         -debug       - Turn on debug mode\n");
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
  dprintf("%s: reapchild()\n");
  while(waitpid(-1,&status,WNOHANG) > 0);
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
  syslog(LOG_INFO, "%s(%s): closing all files and exiting\n",progname,logtag);
  dprintf("%s(%s): closing all files and exiting\n",progname,logtag);
  close(api);
  unlink(pidfile);
  closelog();
  exit(0);
}

