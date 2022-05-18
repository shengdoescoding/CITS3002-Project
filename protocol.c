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

int send_command(int s, int current_actset, int current_act){
	// First send ISCOMMAND to inform server a command is comming
	uint32_t data_type = htonl((uint32_t) ISCOMMAND);
	if(send_all_int(s, data_type, SIZEOF_INT) < 0){
		perror("Failed: could not send data type!");
		return(errno);
	}
	// Second send size of incoming command
	uint32_t command_len = strlen(rake_file.actsets[current_actset]->acts[current_act]->command);
	uint32_t nettwork_command_len = htonl((uint32_t) command_len);
	if(send_all_int(s, nettwork_command_len, SIZEOF_INT) < 0){
		perror("Failed: could not send command size!");
		return(errno);
	}
	// Lastly send command
	if(send_all_string(s, rake_file.actsets[current_actset]->acts[current_act]->command, command_len) < 0){
		perror("Error: Failed to send command");
		return(errno);
	};
	return(errno);
}

int send_file(int s, int current_actset, int current_act, int current_file){
	// First send ISFILE to inform server a file is coming
	uint32_t data_type = htonl((uint32_t) ISFILE);
	if(send_all_int(s, data_type, SIZEOF_INT) < 0){
		perror("Failed: could not send data type!");
		return(errno);
	}
	// Second send size of file name
	uint32_t file_name_len = strlen(rake_file.actsets[current_actset]->acts[current_act]->required_files[current_file]);
	uint32_t network_file_name_len = htonl((uint32_t) file_name_len);
	if(send_all_int(s, network_file_name_len, SIZEOF_INT) < 0){
		perror("Failed: could not send file name size!");
		return(errno);
	}
	// Third send name of file
	if(send_all_string(s, rake_file.actsets[current_actset]->acts[current_act]->required_files[current_file], file_name_len) < 0){
		perror("Error: Failed to send file name");
		return(errno);
	};
	// Fourth send size of file
	FILE *fp = fopen(rake_file.actsets[current_actset]->acts[current_act]->required_files[current_file], "r");
	if (fp == NULL) {
		perror("Unable to open file");
	}
	struct stat f_stat;
	if(fstat(fileno(fp), &f_stat) < 0){
		perror("Failed: could not get file stats");
	}
	uint32_t size_of_file = f_stat.st_size;
	uint32_t network_size_of_file = htonl((uint32_t) size_of_file);
	if(send_all_int(s, network_size_of_file, SIZEOF_INT) < 0){
		perror("Failed: could not send file size!");
		return(errno);
	}
	// Send file
	int sent_bytes = 0;
	off_t offset = 0;
	int bytes_remaining = size_of_file;
	while(((sent_bytes = sendfile(s, fileno(fp), &offset, BUFFSIZE)) > 0) && (bytes_remaining > 0)){
		printf("1. Server sent %d bytes from file's data, offset is now : %ld and remaining data = %d\n", sent_bytes, offset, bytes_remaining);
		bytes_remaining -= sent_bytes;
		printf("1. Server sent %d bytes from file's data, offset is now : %ld and remaining data = %d\n", sent_bytes, offset, bytes_remaining);
	}
	return(errno);
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