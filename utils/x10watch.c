/*
 *
 * $Id: x10watch.c,v 1.3 2003/02/24 17:20:40 whiles Exp whiles $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * blocking watch utility for reading /dev devices
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

static char *progname;
static char *device = NULL;
static char *action_on = NULL;
static char *action_off = NULL;
static char *device_and = NULL;
static int debug = 0;
static int interval = 5;		// polling interval
static int synchronous = 1;
static int allowroot = 0;
static int timedelay = 0;

#ifdef dbg
#undef dbg
#endif
#define dbg(format, arg...) if (debug) printf("%s/%d/%s(): " format "\n",__FILE__, __FUNCTION__ , __LINE__, ## arg)

#define TEST2NDARG if (i+2 > argc) { syntax(argc,argv,0); exit(1); }

#define sighandler_type void

static void syntax(int argc, char *argv[], int i);
static sighandler_type die(int sig);
static int watch();

// these store the status.  It is possible that someone could specify /dev/x10/e
int fd;

int main(int argc, char *argv[])
{
	char *arg;
	int i,n;
	uid_t uid;

	progname = basename(argv[0]);
	uid = getuid();
	if (argc < 2) {
		syntax(argc,argv,0);
		exit(-1);
	}
	device=argv[1];
	for (i = 2; i < argc; i++) {
		arg = argv[i];
		if (arg[0] != '-'){
			syntax(argc,argv,i);
			exit(-1);
		}
		switch (arg[1]) {
			case 'd':		// debug
				debug = 1;
				break;
			case 'r':		// debug
				allowroot = 1;
				break;
			case '0':		// specify interval
				TEST2NDARG;
				action_off = argv[++i];
				break;
			case '1':		// specify interval
				TEST2NDARG;
				action_on = argv[++i];
				break;
			case 'a':		// specify interval
				TEST2NDARG;
				device_and = argv[++i];
				break;
			case 'p':		// specify interval
				TEST2NDARG;
				timedelay = atoi(argv[++i]);
				break;
			default:
				syntax(argc,argv,i);
				exit(1);
		}
	}
	if (uid == 0 && !allowroot) {
		fprintf(stderr,"%s:  Unsave to run as root (specify -r to override)!!!!\n");
		exit(0);
	}
	if (action_on == NULL && action_off == NULL){
		fprintf(stderr,"%s: error - must specify at least one action\n",progname);
		exit(-1);
	}

	dbg("watching %s, interval=%d, on=%s, off=%s",device,interval,action_on,action_off);
	// OK...we know what to do...now do it
	
	fd = open(device,O_RDONLY);
	if (fd < 0){
		fprintf(stderr,"%s: error opening %d (%s)\n",progname,device,strerror(errno));
		exit(fd);
	}
	watch();
		

	// all done...ready to exit
	die(0);
}

static int watch() 
{
	char data[10];
	int i,sysret,currentstatus,newstatus;

	memset(data,0,sizeof(data));
	i = read(fd,data,sizeof(data));
	dbg("data=%s",data);
	if (i < 0) {
		fprintf(stderr,"%s: error reading device %s (%s)\n",progname,device,strerror(errno));
		return i;
	}
	if (data[5]) {	// test to see if we got extra stuff (reading control dev)
		fprintf(stderr,"%s: error - device must be a single device\n");
		return (1);
	}
	currentstatus = atoi(data);
	dbg("currentstatus=%d",currentstatus);
	if (currentstatus != 0)
		currentstatus = 1;
	while (1) {
		dbg("Waiting for data from %s...",device);
		i = read(fd,data,sizeof(data));	// this blocks
		newstatus = atoi(data);
		if (newstatus != 0)		// dim levels = on
			newstatus = 1;
		dbg("currentstatus=%d, newstatus=%d",currentstatus,newstatus);
		if (newstatus == currentstatus) 
			continue;
		currentstatus = newstatus;
		if (device_and) {
			int fda = open(device_and,O_RDONLY|O_NONBLOCK);
			i = read(fda,data,sizeof(data));	// non blocking
			close(fda);
			i = atoi(data);
			if (i == 0)
				continue;
		}
		if (newstatus && action_on){
			dbg("executing %s",action_on);
			sysret = system(action_on);
			sleep(timedelay);
		}
		else if (!newstatus && action_off){
			dbg("executing %s",action_off);
			sysret = system(action_off);
			sleep(timedelay);
		}
		else
			continue;
		if (WIFSIGNALED(sysret) && 
		(WTERMSIG(sysret) == SIGINT || WTERMSIG(sysret) == SIGQUIT))
			break;
	}
}

// Note!!!  we don't tell the user about the -r option.  They only get that
// once they try to run it as root.
static void syntax(int argc, char *argv[], int i)
{
	if (i == 0)
		fprintf(stderr,"%s: Missing argument\n",progname);
	else
		fprintf(stderr,"%s: Unknown argument - %s\n",progname,argv[i]);

	fprintf(stderr,"Syntax: %s device [-0 ONactioni] [-1 OFFaction] [-a device] [-d]\n",progname);
	fprintf(stderr,"        device must be a valid x10 device\n");
	fprintf(stderr,"        -1: ONaction must be an executable command\n");
	fprintf(stderr,"        -0: OFFaction must be an executable command\n");
	fprintf(stderr,"	-a: device that must be ON for script to execute\n");
	fprintf(stderr,"        -d: Turn on debug mode\n");
}

static sighandler_type die(sig)
{
	close(fd);
}

