#ifndef __X10_H__
#define __X10_H__

#include <linux/types.h>

#define MESSAGE_QUEUE_SIZE	16
#define MAX_LOG 		32

typedef struct x10message {
  __u8		source;		// Where did the message come from/go to
  __u8		housecode;	// 0-16
  __u8		unitcode;	// 0-16
  __u8		command;	// see X10_CMD_* below
  __u32		flag;		// flags (see below)
} x10_message_t;

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
