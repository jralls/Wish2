
/* 
 *
 * $Id: strings.c,v 1.1 2004/01/03 20:40:49 whiles Exp $
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

#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/string.h>

#include "x10.h"
#include "strings.h"

struct cmd_list {
	char *str;
	int  fc;
};

struct ps_list {
	char *str;
	int fc;
	int hc;
};

char *logstring[16] = { 
	"ALL_UNITS_OFF",
	"ALL_LIGHTS_ON",
	"ON",
	"OFF",
	"DIM",
	"BRIGHT",
	"ALL_LIGHTS_OFF",
	"EXTENDED_CODE",
	"HAIL_REQUEST",
	"PRESETDIMHIGH",
	"PRESETDIMLOW",
	"EXTENDED_DATA",
	"STATUS=ON",
	"STATUS=OFF",
	"STATUS",
	"HAIL_ACKNOWLEDGE"
};

static struct cmd_list uc_cmd_list[2] = {		// only to unitcodes
	{"null",		X10_CMD_END},
	{"nothing",		X10_CMD_END},
};

static struct cmd_list ex_cmd_list[4] = {		// only to unitcodes
	{"EXTENDED_D",		X10_CMD_EXTENDEDDATAA},
	{"EXTENDED_C",		X10_CMD_EXTENDEDCODE},
	{"ECODE",		X10_CMD_EXTENDEDCODE},
	{"EDATA",		X10_CMD_EXTENDEDDATAA},
};

static struct cmd_list hc_cmd_list[16] = {		// only to housecodes
	{"ALL_U",		X10_CMD_ALLUNITSOFF},
	{"AUO",			X10_CMD_ALLUNITSOFF},
	{"ALL_LIGHTS_ON",	X10_CMD_ALLLIGHTSON},
	{"AON",			X10_CMD_ALLLIGHTSON},
	{"ALL_LIGHTS_OF",	X10_CMD_ALLLIGHTSOFF},
	{"AOF",			X10_CMD_ALLLIGHTSOFF},
	{"PRESETDIMH",		X10_CMD_PRESETDIMHIGH},
	{"PDIMHIGH",		X10_CMD_PRESETDIMHIGH},
	{"PDH",			X10_CMD_PRESETDIMHIGH},
	{"PRESETDIML",		X10_CMD_PRESETDIMLOW},
	{"PDIMLOW",		X10_CMD_PRESETDIMLOW},
	{"PDL",			X10_CMD_PRESETDIMLOW},
	{"HAIL_R",		X10_CMD_HAILREQUEST},
	{"HREQ",		X10_CMD_HAILREQUEST},
	{"HAIL_A",		X10_CMD_HAILACKNOWLEDGE},
	{"HACK",		X10_CMD_HAILACKNOWLEDGE},
};

static struct cmd_list cmd_list[13] = {			// all devices
	{"ON",			X10_CMD_ON},
	{"1",			X10_CMD_ON},
	{"OFF",			X10_CMD_OFF},
	{"0",			X10_CMD_OFF},
	{"DIM",			X10_CMD_DIM},
	{"-",			X10_CMD_DIM},
	{"BRI",			X10_CMD_BRIGHT},
	{"+",			X10_CMD_BRIGHT},
	{"STATUS=ON",		X10_CMD_STATUSON},
	{"S=ON",		X10_CMD_STATUSON},
	{"STATUS=OF",		X10_CMD_STATUSOFF},
	{"S=OFF",		X10_CMD_STATUSOFF},
	{"STA",			X10_CMD_STATUS},
};

static struct ps_list ps_list[32] = {
	{"ps1" ,X10_CMD_PRESETDIMLOW,'M'},
	{"ps2" ,X10_CMD_PRESETDIMLOW,'N'},
	{"ps3" ,X10_CMD_PRESETDIMLOW,'O'},
	{"ps4" ,X10_CMD_PRESETDIMLOW,'P'},
	{"ps5" ,X10_CMD_PRESETDIMLOW,'C'},
	{"ps6" ,X10_CMD_PRESETDIMLOW,'D'},
	{"ps7" ,X10_CMD_PRESETDIMLOW,'A'},
	{"ps8" ,X10_CMD_PRESETDIMLOW,'B'},
	{"ps9" ,X10_CMD_PRESETDIMLOW,'E'},
	{"ps10",X10_CMD_PRESETDIMLOW,'F'},
	{"ps11",X10_CMD_PRESETDIMLOW,'G'},
	{"ps12",X10_CMD_PRESETDIMLOW,'H'},
	{"ps13",X10_CMD_PRESETDIMLOW,'K'},
	{"ps14",X10_CMD_PRESETDIMLOW,'L'},
	{"ps15",X10_CMD_PRESETDIMLOW,'I'},
	{"ps16",X10_CMD_PRESETDIMLOW,'J'},
	{"ps17",X10_CMD_PRESETDIMHIGH,'M'},
	{"ps18",X10_CMD_PRESETDIMHIGH,'N'},
	{"ps19",X10_CMD_PRESETDIMHIGH,'O'},
	{"ps10",X10_CMD_PRESETDIMHIGH,'P'},
	{"ps21",X10_CMD_PRESETDIMHIGH,'C'},
	{"ps22",X10_CMD_PRESETDIMHIGH,'D'},
	{"ps23",X10_CMD_PRESETDIMHIGH,'A'},
	{"ps24",X10_CMD_PRESETDIMHIGH,'B'},
	{"ps25",X10_CMD_PRESETDIMHIGH,'E'},
	{"ps26",X10_CMD_PRESETDIMHIGH,'F'},
	{"ps27",X10_CMD_PRESETDIMHIGH,'G'},
	{"ps28",X10_CMD_PRESETDIMHIGH,'H'},
	{"ps29",X10_CMD_PRESETDIMHIGH,'K'},
	{"ps30",X10_CMD_PRESETDIMHIGH,'L'},
	{"ps31",X10_CMD_PRESETDIMHIGH,'I'},
	{"ps32",X10_CMD_PRESETDIMHIGH,'J'}
};

// Note...one problem with comparing based on length of command...if a shorter
// command matches part of a longer command and is hit first, the user command
// will always be matched to the shorter command.
int parse_function(unsigned char *p,unsigned int mask)
{
        int j;

        for (j = 0; j < sizeof(cmd_list)/sizeof(struct cmd_list); j++)
                if (!strncasecmp(p,cmd_list[j].str,strlen(cmd_list[j].str)))
                        return cmd_list[j].fc;
if (mask & X10_CMD_MASK_UNITCODES)
        for (j = 0; j < sizeof(uc_cmd_list)/sizeof(struct cmd_list); j++)
                if (!strncasecmp(p,uc_cmd_list[j].str,strlen(uc_cmd_list[j].str)))
                        return uc_cmd_list[j].fc;
if (mask & X10_CMD_MASK_HOUSECODES)
        for (j = 0; j < sizeof(hc_cmd_list)/sizeof(struct cmd_list); j++)
                if (!strncasecmp(p,hc_cmd_list[j].str,strlen(hc_cmd_list[j].str)))
                        return hc_cmd_list[j].fc;
if (mask & X10_CMD_MASK_EXTENDED)
        for (j = 0; j < sizeof(ex_cmd_list)/sizeof(struct cmd_list); j++)
                if (!strncasecmp(p,ex_cmd_list[j].str,strlen(ex_cmd_list[j].str)))
                        return ex_cmd_list[j].fc;
if (mask & X10_CMD_MASK_PRESETDIM)
        for (j = 0; j < sizeof(ps_list)/sizeof(struct ps_list); j++)
                if (!strncasecmp(p,ps_list[j].str,strlen(p)))
                        return ((ps_list[j].hc<<8) | ps_list[j].fc);
    return -1;
}
