#ifndef protocol_h
#define protocol_h

#define SIZEOF_INT 4

#define ISCOMMAND 1
#define ISFILE 2
#define FILERECIEVED 3
#define ISLOADQUERY 4
#define ISLOAD 5
#define ALLCOMMANDSSENT 6
#define ALLCOMMANDEXECUTED 7

#include <sys/sendfile.h>

int send_all(int s, char *buf, int len);
int send_command(int s, int current_actset, int current_act);
int send_file(int s, int current_actset, int current_act, int current_file);
int send_loadquery(int s);
uint32_t unpack_uint32(const char *bytes);

#endif