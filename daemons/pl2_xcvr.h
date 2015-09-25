#ifndef __X10_XCVR_PL_H__
#define __X10_XCVR_PL_H__

/*
 *
 * $Id: pl_xcvr.h,v 1.1 2004/01/14 03:17:14 whiles Exp $
 *
 * Copyright (c) 2002 Scott Hiles
 *
 * X10 header file for PowerLinc Transceiver driver
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
struct pl_cmd {
	unsigned char cmd;
	unsigned char hc;
	unsigned char uc;
	unsigned char fc;
	unsigned char term;
};

#define X10_PL_ACK 0x06
#define X10_PL_NAK 0x15
#define X10_PL_CR  0x0d
#define X10_PL_START 0x02
#define X10_PL_XMITRCV 0x78
#define X10_PL_XMITRCVSELF 0x58
#define X10_PL_REPEATONCE 0x41
#define X10_PL_SENTONCE 0x31
#define X10_PL_SENDCMD 0x63
#define X10_PL_EXTENDEDDATA 0x80

#define X10_PL_EXTDATASTART 0x45

#endif
