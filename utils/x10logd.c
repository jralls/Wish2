
/*
 *
 * $Id: x10logd.c,v 1.3 2005/05/02 00:26:38 root Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <syslog.h>
#include <time.h>
#include <libgen.h>

#include <termios.h>

#include <asm/types.h>

#define sighandler_type void

static char *logfile="/var/log/x10log";
static char *device="/dev/x10/log";
static char *pidfile = "/var/run/x10logd.pid";
static int debug = 0;
static int x10dev,x10log;
static char *progname;
static char *logtag;
static int syslog_facility = LOG_LOCAL5;
static int f_syslog;		// flag for sending to syslog
static int f_logall;		// flag to log everything
static int f_logfile;		// flag to log to a file

#define dprintf if(debug) printf
#define mysyslog if (f_syslog) syslog

#define MAXHOSTNAMELEN 128
#define MAXLINE 32

#define TESTNEXTARG if (i+2 > argc) { syntax(argc,argv,0); exit(1); }

char data[MAXLINE];
char localhostname[MAXHOSTNAMELEN+1];
char *localdomain;

static sighandler_type die(int sig),reapchild();
static void syntax(int argc, char *argv[],int i);
static void write_log(int fd,char *buf,int len);
static void start();

int main(int argc, char *argv[])
{
	register int i;
	char *arg;

	f_syslog=0;
	f_logall=0;
	f_logfile=0;
	progname = basename(argv[0]);
	logtag = progname;
	for (i = 1; i < argc; i++){
		arg = argv[i];
		if (arg[0] != '-') {
			syntax(argc,argv,i);
			exit(-1);
		}
		switch (arg[1]) {
			case 'o':			// log file specified
				TESTNEXTARG;
				logfile = argv[++i];
				f_logfile = 1;
				break;
			case 'i':			// input device specified
				TESTNEXTARG;
				device = argv[++i];
				break;
			case 'd':			// debug turned on
				debug = 1;
				break;
			case 'p':
				TESTNEXTARG;
				pidfile = argv[++i];
				break;
			case 's':			// send to syslog
				f_syslog = 1;
				break;
			case 'a':			// start from epoch
				f_logall = 1;
				break;
			case 't':			// tag
				TESTNEXTARG;
				logtag = argv[++i];
				break;
			default:
				syntax(argc,argv,i);
				exit(-1);
		}
	}

	if (!f_syslog && !f_logfile)
		f_logfile = 1;
	start();
	return -1;
}

void untty() 
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

void start() 
{	
	register int i;
	FILE *fp;
	register char *p;
	struct sigaction action;
	int n;
	//
	// Open up a syslog descriptor
	if (f_syslog)
		openlog(logtag, LOG_NDELAY | LOG_PID, syslog_facility);

	// now let's open our files before we try to to daemon mode
	x10dev = open(device,O_RDONLY);
	if (x10dev < 0) {
		mysyslog(LOG_ERR, "Error:  Unable to open device %s (%s)\n",
				device,strerror(errno));
		fprintf(stderr,"Error:  Unable to open device %s (%s)\n",
			device,strerror(errno));
		exit(-1);
	}	

	if (f_logfile) {
		x10log = open(logfile,O_APPEND|O_CREAT|O_SYNC|O_RDWR,
				      S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		if (x10log < 0) {
			mysyslog(LOG_ERR,"Error:  Unable to open log %s (%s)\n",
				logfile,strerror(errno));
			fprintf(stderr,"Error:  Unable to open log %s (%s)\n",
				logfile,strerror(errno));
			exit(-1);
		}	
	}

	mysyslog(LOG_INFO, "starting.  Logfile=%s, socket-device=%s, pidfile=%s",
		logfile,device,pidfile);
	dprintf("%s starting\nlogfile=%s\ndevice=%s\npidfile=%s\nlogtag=%s\n",
		progname, logfile,device,pidfile,logtag);
	if (!debug)
		if (fork())
			exit(0);

	// OK...we are a daemon or in debug...close down all standard files
	if (!debug) {
		for (i = 0; i < 3; i++)
			close(i);
		open("/",0);
		dup2(0,1);
		dup2(0,2);
		untty();
	}

	gethostname(localhostname,sizeof(localhostname));
	if ((p = strchr(localhostname,'.'))) {
		*p++='\0';
		localdomain = p;
	}
	else 
		localdomain = "";
	
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);

	action.sa_handler = debug ? die : SIG_IGN;
	sigaction(SIGTERM,&action,NULL);
	sigaction(SIGINT,&action,NULL);

	action.sa_handler = die;
	sigaction(SIGHUP,&action,NULL);

	action.sa_handler = reapchild;
	sigaction(SIGCHLD,&action,NULL);

	if (!f_logall)
		lseek(x10dev,0,SEEK_END);
	if ((fp = fopen(pidfile,"w"))!= NULL) {
		fprintf(fp,"%d\n",getpid());
		fclose(fp);
	}
	while (1) {
		n = read(x10dev,data,sizeof(data)-1);
		if (n < 0)
			die(0);
		if (n > 0){
			data[n] = '\0';
			write_log(x10log,data,n);
		}
	}
	exit(0);
}
static char *mon[12]={"Jan","Feb","Mar","Apr","May","Jun",
		      "Jul","Aug","Sep","Oct","Nov","Dec"};
static void write_log(int fd,char *buf,int len)
{
	register int i;
	register char *p = NULL;
	char *time = NULL;
	time_t t;
	struct tm tm;
	char outbuf[256];

	if (buf[0] >= '0' || buf[0] <= '9'){
		time = buf;
		for (i = 0; i < len; i++){
			if (buf[i] < '0' || buf[i] > '9'){
				buf[i] = '\0';
				p = &buf[i+1];
				break;
			}
		}
		mysyslog(LOG_INFO, "%s", p);
		if (f_logfile){
			t = atol(time);
			localtime_r(&t,&tm);
			sprintf(outbuf,"%s %2d %02d:%02d:%02d %s %s",
				mon[tm.tm_mon],
				tm.tm_mday,
				tm.tm_hour,tm.tm_min,tm.tm_sec,
				localhostname,
				p);
			write(fd,outbuf,strlen(outbuf));
		}
		dprintf(outbuf);
	}
	else {
		mysyslog(LOG_INFO, "invalid log message (%s)", buf);
		dprintf("%s(%s): invalid log message (%s)\n",progname,logtag,buf);
	}
}

void syntax(int argc, char *argv[],int i)
{
	if (i == 0)
		printf("Missing argument\n");
	else 
		printf("Unknown argument:  %s\n",argv[i]);

	fprintf(stderr,"Syntax:  %s [-o logfile] [-i device] [-p pidfile] [t syslogtag] [-d] [-s] [-a]\n",progname);
	fprintf(stderr,"         -o <logfile> - logfile to write to (default: %s)\n", logfile);
	fprintf(stderr,"         -i <device>  - device socket to read from (default: %s)\n", device);
	fprintf(stderr,"         -p <pidfile> - pidfile(default: %s)\n", pidfile);
	fprintf(stderr,"         -t <logtag>  - tag provided to syslogd (default: %s)\n",logtag);
	fprintf(stderr,"         -d           - debug mode; do not fork; output to STDOUT.\n");
	fprintf(stderr,"         -s           - write log to system syslog\n");
	fprintf(stderr,"         -a           - display all in log buffer\n");
}


static sighandler_type reapchild()
{
	int status;
	dprintf("%s: reapchild()\n",progname);
	while(waitpid(-1,&status,WNOHANG) > 0);
}

static sighandler_type die(int sig)
{
	if (sig) {
		mysyslog(LOG_INFO, "exiting on signal %d\n",sig);
		dprintf("%s(%s): exiting on signal %d\n",progname,logtag,sig);
		errno = 0;
	}
	mysyslog(LOG_INFO, "closing all files and exiting\n");
	if (f_syslog){
		closelog();
	}
	dprintf("%s(%s): closing all files and exiting\n",progname,logtag);
	if (f_logfile)
		close(x10log);
	close(x10dev);
	unlink(pidfile);
	if (sig == SIGHUP) {
		mysyslog(LOG_INFO, "restarting on signal %d\n",sig);
		dprintf("%s(%s): restarting on signal %d\n",progname,logtag,sig);
		start();
	}
	exit(0);
}

