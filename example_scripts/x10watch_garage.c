/*
 *
 * $Id: x10watch_garage.c,v 1.2 2003/01/19 21:12:01 whiles Exp $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * blocking watch utility specifically designed to watch the garage doors
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

static char *g1="/dev/x10/garage1";
static char *g2="/dev/x10/garage2";
static char *sensor="/dev/x10/lightsensor";
static char *alwayson[]={"/dev/x10/livingroom"};
static char *teston[]={"/dev/x10/extgarage"};
static int debug = 0;
static int interval = 5;		// polling interval

#ifdef dbg
#undef dbg
#endif
#define dbg(format, arg...) do { if (debug) printf(__FILE__ "/%d/" __FUNCTION__ "(): " format "\n", __LINE__, ## arg); } while(0)

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
	for (i = 1; i < argc; i++) {
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
			default:
				syntax(argc,argv,i);
				exit(1);
		}
	}
	if (uid == 0 && !allowroot) {
		fprintf(stderr,"%s:  Unsave to run as root (specify -r to override)!!!!\n");
		exit(0);
	}

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
		if (newstatus && action_on){
			dbg("executing %s",action_on);
			sysret = system(action_on);
		}
		else if (!newstatus && action_off){
			dbg("executing %s",action_off);
			sysret = system(action_off);
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

	fprintf(stderr,"Syntax: %s device1 device2 ... [-0 ONaction] [-1 OFFaction] [-d]\n",progname);
	fprintf(stderr,"        device must be a valid x10 device\n");
	fprintf(stderr,"        -1: ONaction must be an executable command\n");
	fprintf(stderr,"        -0: OFFaction must be an executable command\n");
	fprintf(stderr,"        -d: Turn on debug mode\n");
}

static sighandler_type die(sig)
{
	close(fd);
}

