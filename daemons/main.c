/*
 *
 * $Id: x10d.c,v 1.2 2004/01/03 20:44:23 whiles Exp whiles $
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../x10.h"

int main(int argc, char *argv[])
{
	unsigned char *inbuf,*outbuf;
	int n,inf;
	x10_message_t m;

	if (argc != 2) {
		printf("Syntax:  x10d <devicename>\n");
		return 1;
	}
//	inf = open(argv[1],O_RDWR | O_NONBLOCK);
	inf = open(argv[1],O_RDWR);
	if (inf < 0) {
		printf("Error:  Unable to open %s for reading\n",argv[1]);
		return 1;
	}
        m.source = X10_API;
	m.housecode = m.unitcode = 0;
	m.command = X10_CMD_STATUS;
	m.flag = 0;
	fprintf(stderr, "Writing...\n");
	write(inf,&m,sizeof(m));
	fprintf(stderr, "Writing completed...\n");
	sleep(1);
	while (1) {
		fprintf(stderr, "Reading...\n");
		memset(&m,0,sizeof(m));
		n = read(inf,&m,sizeof(m));
		if (n < sizeof(m)) {
			fprintf(stderr, "Error:  Unable to read %s\n",argv[1]);
			return 1;
		}
		if (n == 0)   // end of file
			return 0;
//		if (n > 0) {
			fprintf(stderr, "source:\t0x%x\n",m.source);
			fprintf(stderr, "housecode:\t0x%x\n",m.housecode);
			fprintf(stderr, "unitcode:\t0x%x\n",m.unitcode);
			fprintf(stderr, "command:\t0x%x\n",m.command);
			sleep(1);
//		}
		if (m.source == X10_API && m.command == X10_CMD_ON){
			fprintf(stderr, "Successfully opened X10 API\n");
		}
	}
	return 1;
}
