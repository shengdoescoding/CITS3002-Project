#include "rake-c.h"

int send_all(int s, char *buf, int *len){
	int total = 0;
	int bytesleft = *len;
	int n;

	while(total < *len){
		n = send(s, buf+total, bytesleft, 0);
		if(n == -1){
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total;

	return n==-1?-1:0; // return -1 on failure, 0 on success
}

void send_command(int s, int current_actset, int current_act){
	int len;
	int send_stat;
	len = strlen("COMMAND");
	send_stat = send_all(s, "COMMAND", &len);
	if(send_stat < 0){
		perror("Error: failed to send COMMAND");
	}
	len = strlen(rake_file.actsets[current_actset]->acts[current_act]->command);
	send_stat = send_all(s, rake_file.actsets[current_actset]->acts[current_act]->command, &len);
	if(send_stat < 0){
		perror("Error: Failed to send act");
	}
}