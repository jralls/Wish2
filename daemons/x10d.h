#ifndef __X10D_H__
#define __X10D_H__

#define DEFDEVICE "/dev/x10/.api"
#define DEFPIDFILE "/var/run/x10d.pid"
#define MAXHOSTNAMELEN 128
#define CHILDSTACKSIZE 1024*256

#define dsyslog if(debug) syslog

typedef struct xcvrio {
  void *private;
  int (*received)(int hc, int uc, int fc, unsigned char *buf,int len);
  int (*send)(int hc, int uc, int fc);
  char *device;
  int debug;
  char *logtag;
  sem_t connected;
  int status;
} xcvrio_t;

extern int xmit_init(struct xcvrio *io);
extern char *dumphex(char *hexbuffer,void *data, int len);

extern int retries, delay, timeout, debug;

#endif
