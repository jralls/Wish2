
/* 
 *
 * $Id: x10_strings.h,v 1.2 2003/01/20 00:01:41 whiles Exp $
 *
 * Copyright (c) 2002 Scott Hiles
 * 
 * X10 device abstraction module
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

extern char *logstring[];

extern int parse_function(unsigned char *buf,unsigned int mask);

#define X10_CMD_MASK_UNITCODES	0x01
#define X10_CMD_MASK_HOUSECODES	0x02
#define X10_CMD_MASK_EXTENDED	0x04
#define X10_CMD_MASK_PRESETDIM	0x08
#define X10_CMD_MASK_ALL	0xff
