#ifndef __CM11A_XCVR_H__
#define __CM11A_XCVR_H__

/*
 *
 * $Id: pl_xcvr.h,v 1.1 2004/01/14 03:17:14 whiles Exp whiles $
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

#define CM11A_ADDRESSMASK       0x02
#define CM11A_TRANSMISSIONMASK  0x01
#define CM11A_DIMMASK           0xf8

#define CM11A_HEADER            0x04
#define CM11A_ADDRESS           0x00
#define CM11A_FUNCTION          0x02
#define CM11A_EXTENDED          0x01

#define CM11A_HCMASK            0xf0
#define CM11A_FCMASK            0x0f
#define CM11A_UCMASK            0x0f

#define CM11A_POLL              0x5a
#define CM11A_POLLACK           0xc3
#define CM11A_MACROPOLL         0x5b
#define CM11A_MACROPOLLACK      0xfb
#define CM11A_POWERFAILPOLL     0xa5
#define CM11A_POWERFAILPOLLACK  0x9b
#define CM11A_READY             0x55
#define CM11A_OK                0x00

#endif
