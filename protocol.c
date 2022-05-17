#include "rake-c.h"
#include "protocol.h"

int send_all_string(int s, char *buf, int len){
	int total = 0;
	int bytesleft = len;
	int n;

	while(total < len){
		n = send(s, buf+total, bytesleft, 0);
		if(n == -1){
			break;
		}
		total += n;
		bytesleft -= n;
	}

	return n==-1?-1:0; // return -1 on failure, 0 on success
}

int send_all_int(int s, uint32_t num, int len){
	int total = 0;
	int bytesleft = len;
	int n;
	while(total < len){
		n = send(s, &num, len, 0);
		if(n == -1){
			break;
		}
		total += n;
		bytesleft -= n;
	}
	return n==-1?-1:0;
}

void send_command(int s, int current_actset, int current_act){
	// First send ISCOMMAND to inform server a command is comming
	uint32_t data_type = htonl((uint32_t) ISCOMMAND);
	if(send_all_int(s, data_type, SIZEOF_INT) < 0){
		perror("Failed: could not send data type!");
	}
	// Second send size of incoming command
	uint32_t nettwork_command_len = htonl((uint32_t) strlen(rake_file.actsets[current_actset]->acts[current_act]->command));
	if(send_all_int(s, nettwork_command_len, SIZEOF_INT) < 0){
		perror("Failed: could not send all!");
	}
	uint32_t command_len = strlen(rake_file.actsets[current_actset]->acts[current_act]->command);
	// Lastly send command
	if(send_all_string(s, rake_file.actsets[current_actset]->acts[current_act]->command, command_len) < 0){
		perror("Error: Failed to send act");
	};
}

void send_loadquery(int s){
	uint32_t data_type = htonl((uint32_t) ISLOADQUERY);
	if(send_all_int(s, data_type, SIZEOF_INT) < 0){
		perror("Failed: could not send data type!");
	}
}

uint32_t unpack_uint32(const char *bytes){
	uint32_t unpacked = bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24);
	return unpacked;
}

// void get_load(){

// }