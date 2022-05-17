#ifndef protocol_h
#define protocol_h

#define SIZEOF_INT 4
#define ISCOMMAND 1
#define ISFILE 2

int send_all(int s, char *buf, int len);
void send_command(int s, int current_actset, int current_act);

#endif