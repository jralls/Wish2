#ifndef __PLUSB_XCVR_H__
#define __PLUSB_XCVR_H__

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

#include <linux/types.h>

struct pl_cmd {
        union {
                struct {
                        __u8 start;
                        __u8 power;
                        __u8 hc;
                        __u8 cmd;
                        __u8 zero[4];
                } rcv;
                struct {
                        __u8 start;
                        __u8 hc;
                        __u8 uc;
                        __u8 fc;
                        __u8 zero[4];
                } snd;
                __u8 byte[8];
        } d;
};

// these have been validated to work properly
#define X10_PLUSB_CR  0x0d

// first byte of transmit packets
#define X10_PLUSB_XMIT          0x00
#define X10_PLUSB_XMITEDATA     0x10
#define X10_PLUSB_XMITCTRL      0x20
#define X10_PLUSB_XMITRPT       0x30    // continuously reports anything on PL

// following are the bits for the a control packet
#define X10_PLUSB_TCTRLWAIT     0x01    // Transmit X10 after quiet period
#define X10_PLUSB_TCTRLCOLDET   0x02    // Collision Detect: Stop on collision
#define X10_PLUSB_TCTRLQUALITY  0x04    // Set Detection quality
#define X10_PLUSB_TCTRLSTATUS   0x08    // status request
#define X10_PLUSB_TCTRLRSTRPT   0x10    // reset report mode
#define X10_PLUSB_TCTRLRSTFLAG  0x20    // reset flagged services
#define X10_PLUSB_TCTRLRSTCONT  0x40    // Cancel continuous transmission

// following are the masks for the first byte of data received
#define X10_PLUSB_MASK          0xc0
#define X10_PLUSB_RCV           0x00
#define X10_PLUSB_RCVEDATA      0x40
#define X10_PLUSB_RCVCTRL       0x80
#define X10_PLUSB_RCVRPT        0xc0

// following are the bits for the first control byte received
#define X10_PLUSB_RCTRLON       0x01    // PowerLinc ON
#define X10_PLUSB_RCTRLRBUSY    0x02    // Receiving data
#define X10_PLUSB_RCTRLTBUSY    0x04    // Transmitting data
#define X10_PLUSB_RCTRLQMODE    0x08    // In Quiet Mode
#define X10_PLUSB_RCTRLRMODE    0x10    // In Retry Mode
#define X10_PLUSB_RCTRLRPT      0x20    // In report mode
#define X10_PLUSB_RCTRLOVERRUN  0x40    // To much extended data (overrun)
#define X10_PLUSB_RCTRLERROR    0x80    // Error on last transmission

// following are the bits for the second control byte received
#define X10_PLUSB_RCTRLCONT     0x01    // Continuous mode on

// following is the bitmask for the third control byte
#define X10_PLUSB_RCTRLMAJVER   0x7f    // Major Version number (mask)
#define X10_PLUSB_RCTRLMINVER   0xff    // Minor version number (mask)

#endif
