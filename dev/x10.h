#ifndef __X10_H__
#define __X10_H__
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

#include <linux/types.h>

#define MESSAGE_QUEUE_SIZE	16
#define MAX_LOG 		512

typedef struct x10message {
  int		source;		// Where did the message come from/go to
  int		housecode;	// 0-16
  int		unitcode;	// 0-16
  int		command;	// see X10_CMD_* below
  __u32		flag;		// flags (see below)
} x10_message_t;

#define MAX_HOUSECODES 16
#define MAX_UNITS 16

// Values for the source field
#define X10_CONTROL		0x01
#define X10_DATA		0x02
#define X10_API			0x04

// Values for command field
#define X10_CMD_ALLUNITSOFF     0x0
#define X10_CMD_ALLLIGHTSON     0x1
#define X10_CMD_ON              0x2
#define X10_CMD_OFF             0x3
#define X10_CMD_DIM             0x4
#define X10_CMD_BRIGHT          0x5
#define X10_CMD_ALLLIGHTSOFF    0x6
#define X10_CMD_EXTENDEDCODE    0x7
#define X10_CMD_HAILREQUEST     0x8
#define X10_CMD_PRESETDIMHIGH   0x9
#define X10_CMD_PRESETDIMLOW    0xa
#define X10_CMD_EXTENDEDDATAA   0xb
#define X10_CMD_STATUSON        0xc
#define X10_CMD_STATUSOFF       0xd
#define X10_CMD_STATUS          0xe
#define X10_CMD_HAILACKNOWLEDGE 0xf
#define X10_CMD_END		0x10

// Values for flag field
#define X10_FLAG_LLSEEK		0x10	// flag to contain seek info
#define X10_FLAG_POLL		0x11	// determines if data is available

#define X10_FLAG_NONBLOCK	0x00000001

#endif
