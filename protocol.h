#ifndef protocol_h
#define protocol_h

#define SIZEOF_INT 4
#define BUFFSIZE 1024

#define ISCOMMAND 1
#define ISFILE 2
#define ISLOADQUERY 3
#define ISLOAD 4
#define ALLCOMMANDSSENT 5

#include <sys/sendfile.h>

int send_all(int s, char *buf, int len);
int send_command(int s, int current_actset, int current_act);
int send_file(int s, int current_actset, int current_act, int current_file);
int send_loadquery(int s);
uint32_t unpack_uint32(const char *bytes);

#endif