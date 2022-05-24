/**
 * @file protocol.c
 * @author ShengTang Zhao (22980212)
 * @brief contains some functions used for the socket client to interact with the server
 */
#include "rake-c.h"
#include "protocol.h"

/**
 * @brief Send all bytes of the provided string to the server
 * 
 * @param s socket descriptor of the connection between client and destination server
 * @param buf string to be sent to the server
 * @param len total length of the string to be sent 
 * @return int -1 on failure, 0 on success
 */
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

	return n==-1?-1:0;
}

/**
 * @brief Send all bytes of the provided unsigned 32 bits integer to the server
 * 
 * @param s socket descriptor of the connection between client and destination server
 * @param num a 32 bits integer to be sent to the server
 * @param len number of bytes to be sent (will always be 4 since an unsigned 32 bits integer is 4 bytes)
 * @return int -1 on failure, 0 on success
 */
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

/**
 * @brief Send a command from the rakefile to the server
 * 
 * @param s socket descriptor of the connection between client and destination server
 * @param current_actset index of current_actset for rake_file.actsets[current_actset]
 * @param current_act index of current_act for rake_file.actset[current_actset]->acts[current_act]
 * @return int return errno
 */
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

/**
 * @brief Send a file required by a command
 * 
 * @param s socket descriptor of the connection between client and destination server
 * @param current_actset index of current actset for rake_file.actsets[current_actset]
 * @param current_act index of current action for rake_file.actset[]->acts[current_act]
 * @param current_file index of current file for rake_file.actset[]->acts[]->required_files[current_file]
 * @return int return errno
 */
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
		exit(EXIT_FAILURE);
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
		bytes_remaining -= sent_bytes;
	}
	return(errno);
}

/**
 * @brief send an instruction instructing so the server can expect what is coming from the client
 * 
 * @param s socket descriptor of the connection between client and destination server
 * @param instruction an instruction, outlined in protocol.h
 * @return int return errno
 */
int send_instruction(int s, int instruction){
	uint32_t data_type = htonl((uint32_t) instruction);
	if(send_all_int(s, data_type, SIZEOF_INT) < 0){
		perror("Failed: could not send data type!");
		return errno;
	}
	return errno;
}

/**
 * @brief unpack some recieved bytes from server into an unsigned 32 bit int
 * 
 * @param bytes to be unpacked
 * @return uint32_t the unpacked int
 */
uint32_t unpack_uint32(const unsigned char *bytes){
	uint32_t unpacked = bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24);
	return unpacked;
}