/*
 *
 * $Id: nbecho.c,v 1.1 2004/01/17 21:17:29 whiles Exp root $
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
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char line[256];
	int inf,flags,i;

	if (argc < 2) {
		printf("Syntax:  nbecho <argument>\n");
		return 1;
	}
	inf = STDOUT_FILENO;
        flags = fcntl(inf,F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(inf,F_SETFL,flags);
        strcpy(line,"");
        for (i = 1; i < argc; i++)
          strcat(line,argv[i]);
	fprintf(stdout,"%s\n",line);
	return 0;
}
