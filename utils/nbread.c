/*
 *
 * $Id: nbread.c,v 1.2 2002/12/17 15:40:07 whiles Exp $
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

int main(int argc, char *argv[])
{
	unsigned char *inbuf,*outbuf;
	char line[256];
	int n,inf;

	if (argc != 2) {
		printf("Syntax:  nbread <devicename>\n");
		return 1;
	}
	inf = open(argv[1],O_RDONLY | O_NONBLOCK);
	if (inf < 0) {
		printf("Error:  Unable to open %s for reading\n",argv[1]);
		return 1;
	}
	while (1) {
		n = read(inf,line,256);
		if (n < 0) {
			printf("Error:  Unable to read %s\n",argv[1]);
			return 1;
		}
		if (n == 0)   // end of file
			return 0;
		line[n] = '\0';
		printf("%s",line);
	}
	return 1;
}
