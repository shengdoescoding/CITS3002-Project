#include "rake-c.h"

void init_rake_file() {
	rake_file.port = -1;
	rake_file.total_hosts = 0;
	rake_file.total_actionsets = 0;
	rake_file.hosts = NULL;
	rake_file.actsets = NULL;
}

void init_actionset(struct Actionset *actionset) {
	actionset->total_actions = 0;
	actionset->acts = NULL;
}

void init_action(struct Action *action) {
	action->remote = false;
	action->command = NULL;
	action->required_files = NULL;
	action->total_files = 0;
}

void mem_alloc_check(const void *p, char *var_name) {
	if (p == NULL) {
		fprintf(stderr, "Fatal: failed to allocate memory for %s", var_name);
	}
}

char *trim_white_space(char *str) {
	char *end;
	while (isspace((unsigned char)*str)) str++;
	if (*str == 0) {
		return str;
	}

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	end[1] = '\0';

	return str;
}

bool prefix(const char *str, const char *pre) {
	return strncmp(pre, str, strlen(pre)) == 0;
}

void add_port(const char *line) {
	int nwords;
	char **port = strsplit(line, &nwords);
	int port_number = atoi(port[2]);
	rake_file.port = port_number;
	free_words(port);
}

void add_command(struct Action *action, const char *line_no_whitespace) {
	if (prefix(line_no_whitespace, "remote-") == true) {
		action->remote = true;
		action->command = realloc(action->command, (strlen(line_no_whitespace) - 7) * sizeof(char) + 1);
		mem_alloc_check(action->command, "action.command");
		strcpy(action->command, &line_no_whitespace[7]);
	} else {
		action->remote = false;
		action->command = realloc(action->command, strlen(line_no_whitespace) * sizeof(char) + 1);
		mem_alloc_check(action->command, "action.command");
		strcpy(action->command, line_no_whitespace);
	}
}

void add_actionset_to_rakefile(struct Actionset *actionset) {
	rake_file.actsets = realloc(rake_file.actsets, sizeof(struct Actionset *) * rake_file.total_actionsets);
	mem_alloc_check(rake_file.actsets, "rake_file.actsets");
	rake_file.actsets[CURR_ACTSET_INDEX] = actionset;
}

void add_action_to_actionset(struct Action *action) {
	rake_file.actsets[CURR_ACTSET_INDEX]->acts = realloc(rake_file.actsets[CURR_ACTSET_INDEX]->acts, sizeof(struct Action *) * rake_file.actsets[CURR_ACTSET_INDEX]->total_actions);
	mem_alloc_check(rake_file.actsets[CURR_ACTSET_INDEX]->acts, "rake_file.actsets[CURR_ACTSET_INDEX]->acts");
	rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX] = action;
}

void add_hosts(const char *line) {
	int nwords;
	char **hosts = strsplit(line, &nwords);
	rake_file.total_hosts = nwords - 2;
	rake_file.hosts = malloc(rake_file.total_hosts * sizeof(char *));
	mem_alloc_check(rake_file.hosts, "rake_file.hosts");
	int i = 0;
	for (int j = 2; j < nwords; j++) {
		rake_file.hosts[i] = malloc(strlen(hosts[j]) * sizeof(char) + 1);
		mem_alloc_check(rake_file.hosts[i], "rake_file.hosts[i]");
		strcpy(rake_file.hosts[i], hosts[j]);
		i++;
	}
	free_words(hosts);
}

void parse_rf(const char *rake_file_address) {
	FILE *fp = fopen(rake_file_address, "r");
	if (fp == NULL) {
		perror("Unable to open file");
	}
	char line[MAX_LINE_LEN];
	while (fgets(line, MAX_LINE_LEN, fp)) {
		char *line_no_whitespace = trim_white_space(line);

		// Ignore comments
		if (line_no_whitespace[0] == '#') {
			continue;
		}

		// Read PORT number and store into structure
		if (strstr(line, "PORT") != NULL && line[0] != '\t') {
			add_port(line);
		}

		// Read HOST and store into structure
		if (strstr(line, "HOST") && line[0] != '\t') {
			add_hosts(line);
		}

		// Detect actionset
		if (strstr(line, "actionset") != NULL && line[0] != '\t') {
			struct Actionset *actionset = malloc(sizeof(*actionset));
			mem_alloc_check(actionset, "actionset");
			init_actionset(actionset);
			rake_file.total_actionsets++;
			CURR_ACTSET_INDEX = rake_file.total_actionsets - 1;
			add_actionset_to_rakefile(actionset);
		}

		// Detect command
		if (line[0] == '\t' && line[1] != '\t') {
			struct Action *action = malloc(sizeof(*action));
			mem_alloc_check(action, "action");
			init_action(action);
			rake_file.actsets[CURR_ACTSET_INDEX]->total_actions++;
			CURR_ACT_INDEX = rake_file.actsets[CURR_ACTSET_INDEX]->total_actions - 1;
			add_command(action, line_no_whitespace);
			add_action_to_actionset(action);
		}

		// Detect required files
		if (line[0] == '\t' && line[1] == '\t') {
			// Populate action in actionset with required files
			int nwords;
			char **required_files = strsplit(line, &nwords);
			rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->total_files = nwords - 1;
			rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->required_files = malloc(rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->total_files * sizeof(char *));
			mem_alloc_check(rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->required_files, "rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->required_files");

			int i = 0;
			for (int j = 1; j < nwords; j++) {
				rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->required_files[i] = malloc(strlen(required_files[j]) * sizeof(char) + 1);
				mem_alloc_check(rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->required_files[i], "actionset.acts[actionset.total_actions - 1]->required_files[i]");
				strcpy(rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX]->required_files[i], required_files[j]);
				i++;
			}

			free_words(required_files);
		}
	}
	fclose(fp);
}

/** This function will execute all commands in an actionset
* in parallel. It is commented out because it is not used,
* parts of it has been adapted into the execute_command(const char **command)
* function.
**/
// void execute_actionset() {
// 	pid_t PID;
// 	int child_exit_status;
// 	for (int i = 0; i < rake_file.total_actionsets; i++) {
// 		int terminated_child = 0;
// 		for (int j = 0; j < rake_file.actsets[i]->total_actions; j++) {
// 			int nwords;
// 			char **command_args = strsplit(rake_file.actsets[i]->acts[j]->command, &nwords);
// 			char *prog_name = command_args[0];
// 			command_args[nwords] = NULL;
// 			nwords++;
// 			PID = fork();
// 			if (PID == 0) {
// 				printf("Child PID = %d\n", getpid());
// 				errno = 0;
// 				execvp(prog_name, command_args);
// 				if(errno != 0){
// 					perror("Fatal: execvp failed!");
// 				}
// 			} else if (PID > 0) {
// 				free_words(command_args);
// 			} else {
// 				perror("Failed to fork");
// 			}
// 		}
// 		while(terminated_child != rake_file.actsets[i]->total_actions){
// 			if(wait(&child_exit_status)){
// 				// if fail do something... 
// 				printf("Child process exited with %d status\n", WEXITSTATUS(child_exit_status));
// 				terminated_child++;
// 			}
// 		}
// 	}
// }

int execute_command(char *command) {
	pid_t PID;
	int child_exit_status;
    int nwords;
    char **command_args = strsplit(command, &nwords);
    char *prog_name = command_args[0];
    command_args[nwords] = NULL;
    nwords++;
    PID = fork();
    if (PID == 0) {
        printf("Child PID = %d\n", getpid());
        errno = 0;
        execvp(prog_name, command_args);
        if(errno != 0){
            perror("Fatal: execvp failed!");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else if (PID > 0) {
        free_words(command_args);
        if(wait(&child_exit_status) >= 0){
            if (WIFEXITED(child_exit_status)){
                return WEXITSTATUS(child_exit_status);
            }
        }
    }
    else {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }
    
    // return failure if function reached this point without returning
    return 1;
}

char *strip_port(char *host){
	char *host_no_port;
	host_no_port = malloc(strlen(host) * sizeof(char) + 1);
	mem_alloc_check(host_no_port, "host_no_port");
	strcpy(host_no_port, host);
	char *colon_index = strchr(host_no_port, ':');
	*colon_index = '\0';
	return host_no_port;
}

int main(int argc, char const *argv[]) {
	if (argc <= 1) {
		fprintf(stderr, "Fatal: must provide rakefile\n");
		exit(EXIT_FAILURE);
	} else if (argc > 2) {
		fprintf(stderr, "Fatal: only provide one rakefile\n");
		exit(EXIT_FAILURE);
	}

	char *rake_file_address;
	rake_file_address = malloc(strlen(argv[1]) * sizeof(char) + 1);
	mem_alloc_check(rake_file_address, "rake_file_address");
	strcpy(rake_file_address, argv[1]);

	init_rake_file();
	parse_rf(rake_file_address);
	free(rake_file_address);

	// // Debug
	// for (int i = 0; i < rake_file.total_actionsets; i++) {
	// 	for (int j = 0; j < rake_file.actsets[i]->total_actions; j++) {
	// 		printf("rakefile actionset %i, action %i, command = %s\n", i, j, rake_file.actsets[i]->acts[j]->command);
	// 		printf("rakefile actionset %i, action %i, remote = %i\n", i, j, rake_file.actsets[i]->acts[j]->remote);
	// 		for (int k = 0; k < rake_file.actsets[i]->acts[j]->total_files; k++) {
	// 			printf("rakefile actionset %i, action %i required files = %s\n", i, j, rake_file.actsets[i]->acts[j]->required_files[k]);
	// 		}
	// 	}
	// }
	// for (int i = 0; i < rake_file.total_hosts; i++) {
	// 	printf("rakefile hosts = %s\n", rake_file.hosts[i]);
	// }
	// printf("rakefile port = %i\n", rake_file.port);


	// execute_actionset();

	fd_set master;
	fd_set read_fd;
	fd_set write_fd;
	int fdmax = 0;
	int newfd;	// New socket descriptor

	FD_ZERO(&master);
	FD_ZERO(&read_fd);
	FD_ZERO(&write_fd);

	struct sockaddr_in server;

	for(int i = 0; i < rake_file.total_hosts; i++){
		newfd = socket(AF_INET, SOCK_STREAM, 0);
		if(newfd < 0){
			perror("Fatal: Failed to create socket\n");
		}
		if(newfd > fdmax){
			fdmax = newfd;
		}
		server.sin_family = AF_INET;
		{
			char *colon = strchr(rake_file.hosts[i], ':');
			if(colon != NULL){
				char *host_no_port = strip_port(rake_file.hosts[i]);
				server.sin_addr.s_addr = inet_addr(host_no_port);
				free(host_no_port);
				server.sin_port = htons(atoi(colon + 1));
			}
			else{
				server.sin_port = htons(rake_file.port);
				server.sin_addr.s_addr = inet_addr(rake_file.hosts[i]);
			}
		}
		int conn_stat = connect(newfd, (struct sockaddr *) &server, sizeof(server));
		if(conn_stat < 0){
			printf("Connecting to Address = %s, Port = %i\n", rake_file.hosts[i], ntohs(server.sin_port));
			perror("Failed to connect to server");
		}
		else{
			FD_SET(newfd, &master);
			printf("newfd = %i\n", newfd);
		}
	}

	// Used to access data from rake_file
	int current_actset = 0;
	int current_act = 0;

	// Used to keep track of host with lowest load
	uint32_t lowest_load = __INT32_MAX__;
	int lowest_load_socket = -1;

	// Used to store data temporarily
	// Server will only ever send 4 bytes, no need for huge buffer
	// Will need to increase for file transfer
	// MAJOR LEARNING POINT, SPECIFY UNSIGNED, BECAUSE SOMETIMES A BYTE CAN BE LARGER THAN 4 BITS IN UNSIGNED CHAR
	unsigned char int_data_bytes[SIZEOF_INT];
	
	// Used to terminate infinite loop
	bool actsets_finished = false;
	
	// Used to track if all commands have been sent to server
	bool all_act_sent = false;

	// Used to track which sockets recieved commads
	bool command_sent[fdmax - 1];
	for(int i = 0; i < fdmax; i++){
		command_sent[i] = false;
	}
	int total_sock_recv_command = 0;

	// Used to count queries for load from servers
	bool load_queried[fdmax - 1];
	for(int i = 0; i <= fdmax; i++){
		load_queried[i] = false;
	}
	int total_quries = 0;

	// Used to track if errors occured during remote execution
	bool command_exec_error = false;

	while(true){
		// Ran all actionsets, close all socket connections
		if(actsets_finished || command_exec_error){
			if(actsets_finished){
				printf("All actionset succesfully executed, shutting down all sockets\n");
			}
			else if(command_exec_error){
				printf("An error occured when executing remote commands, shutting down all sockets\n");
			}

			for(int i = 0; i <= fdmax; i++){
				close(i);
			}
			break;
		}

		read_fd = master;	
		write_fd = master;	
		if(select(fdmax + 1, &read_fd, &write_fd, NULL, NULL) < 0){
			perror("Failed in selecting socket");
			break;
		}
		
		// Main read loop
			for(int i = 0; i <= fdmax; i++){
				if(actsets_finished == false){
					if(FD_ISSET(i, &write_fd) && load_queried[i] == false && all_act_sent == false){
						printf("Sending load query to %i\n", i);
						if(send_instruction(i, ISLOADQUERY) < 0){
							perror("Error: could not send load query");
						}
						load_queried[i] = true;
						total_quries++;
					}

					if(all_act_sent == true && command_sent[i] == true){
						printf("Sending execute all command request to %i\n", i);
						if(send_instruction(i, ALLCOMMANDSSENT) < 0){
							perror("Error: could not send load query");
						}
						command_sent[i] = false;
					}

				}
				
				if(FD_ISSET(i, &read_fd)){
					// Recv Datatype (header)
					int nbytes;
					nbytes = recv(i, int_data_bytes, SIZEOF_INT, 0);
					if(nbytes < 0){
						perror("Error: failed to recieve");
					}
					uint32_t data_type_int = unpack_uint32(int_data_bytes);
					printf("Socket %i returned data type = %i\n", i, data_type_int);
					
					if (data_type_int == 0){
						printf("Socket %i closed unexpectedly by server", i);
						close(i);
						FD_CLR(i, &master);
					}
					else if(data_type_int == ISLOAD){
						// INCOMING LOAD
						nbytes = recv(i, int_data_bytes, SIZEOF_INT, 0);
						if(nbytes < 0){
							perror("Error: failed to recieve");
						}
						total_quries--;
						uint32_t server_load = unpack_uint32(int_data_bytes);
						printf("Socket %i returned server load = %i\n", i, server_load);
						if(server_load <= lowest_load){
							lowest_load = server_load;
							lowest_load_socket = i;
						}
						
					}
					else if(data_type_int == ALLCOMMANDEXECUTED){
						total_sock_recv_command--;

						if(total_sock_recv_command == 0){
							current_actset++;
							current_act = 0;
							all_act_sent = false;
							if(current_actset == rake_file.total_actionsets){
								actsets_finished = true;
							}
						}
					}
					else if(data_type_int == FAILEDCOMMANDEXECUTION){
						command_exec_error = true;
					}
					else if(data_type_int == ISFILE){
						printf("Recieving file from server\n");
						nbytes = recv(i, int_data_bytes, SIZEOF_INT, 0);
						if(nbytes < 0){
							perror("Error: failed to recieve");
						}
						uint32_t file_name_len = unpack_uint32(int_data_bytes);
						printf("file name len = %i\n", file_name_len);

						char file_name[file_name_len + 1];	// +1 for null-terminator byte
						nbytes = recv(i, file_name, file_name_len, 0);
						if(nbytes < 0){
							perror("Error: failed to recieve");
						}
						file_name[file_name_len] = '\0';
						printf("File name = %s\n", file_name);

						nbytes = recv(i, int_data_bytes, SIZEOF_INT, 0);
						if(nbytes < 0){
							perror("Error: failed to recieve");
						}
						uint32_t file_size = unpack_uint32(int_data_bytes);
						printf("file size = %i\n", file_size);

						FILE *fp = fopen(file_name, "wb");
						if (fp == NULL) {
							perror("Unable to open file");
						}
						uint32_t remaining_data = file_size;
						char *buff = malloc(file_size);
						while((remaining_data > 0) && ((nbytes = recv(i, buff, file_size, 0)) > 0)){
							fwrite(buff, sizeof(char), nbytes, fp);
							remaining_data -= nbytes;
							printf("Receive %d bytes and we hope :- %d bytes\n", nbytes, remaining_data);
						}
						fclose(fp);
						free(buff);
						printf("Sending FILE RECIEVED to %i\n", i);
						if(send_instruction(i, FILERECIEVED) < 0){
							perror("Error: could not send load query");
						}
					}
				}
			}
		
		if(all_act_sent == false && actsets_finished == false){
			if(all_act_sent == false && rake_file.actsets[current_actset]->acts[current_act]->remote == true && total_quries == 0 && FD_ISSET(lowest_load_socket, &write_fd)){
				if(rake_file.actsets[current_actset]->acts[current_act]->total_files == 0){
					printf("current command = %s\n", rake_file.actsets[current_actset]->acts[current_act]->command);
					printf("Sending to socket %i\n", lowest_load_socket);
					if(send_command(lowest_load_socket, current_actset, current_act) < 0){
						perror("Failed to send command");
					}
				}
				else{
					printf("CURRENT COMMAND REQUIRES FILES\n");
					printf("current command = %s\n", rake_file.actsets[current_actset]->acts[current_act]->command);
					printf("Sending to socket %i\n", lowest_load_socket);
					for (int i = 0; i < rake_file.actsets[current_actset]->acts[current_act]->total_files; i++){
						printf("Sending file %s\n", rake_file.actsets[current_actset]->acts[current_act]->required_files[i]);
						send_file(lowest_load_socket, current_actset, current_act, i);
						// Wait for server to respond with file recieved
						while(true){
							fd_set temp_read_fd;
							FD_ZERO(&temp_read_fd);
							temp_read_fd = master;
							if(select(fdmax + 1, &temp_read_fd, NULL, NULL, NULL) < 0){
								perror("Failed in selecting socket");
								break;
							}
							if(FD_ISSET(lowest_load_socket, &temp_read_fd)){
								int nbytes;
								nbytes = recv(lowest_load_socket, int_data_bytes, SIZEOF_INT, 0);
								if(nbytes < 0){
									perror("Error: failed to recieve");
								}
								uint32_t data_type_int = unpack_uint32(int_data_bytes);
								printf("Socket %i returned data type = %i\n", lowest_load_socket, data_type_int);
								if(data_type_int == FILERECIEVED){
									break;
								}
							}
						}
					}
					printf("File sent, sending command now!\n");
					if(send_command(lowest_load_socket, current_actset, current_act) < 0){
						perror("Failed to send command");
					}
				}
				// Keep track of which socket, and how many unique sockets recieved commands
				if(command_sent[lowest_load_socket] == false){
					total_sock_recv_command++;
					command_sent[lowest_load_socket] = true;
				}
				// Reset checkers for next command
				current_act++;
				if(current_act == rake_file.actsets[current_actset]->total_actions){
					all_act_sent = true;
				}
				lowest_load_socket = -1;
				lowest_load = __INT32_MAX__;
				for (int i = 0; i <= fdmax; i++)
				{
					if(FD_ISSET(i, &master)){
						load_queried[i] = false;
					}
				}
			}
            else if (rake_file.actsets[current_actset]->acts[current_act]->remote == false){
                int status = execute_command(rake_file.actsets[current_actset]->acts[current_act]->command);
                if (status != 0){
                    command_exec_error = true;
                }
                // Reset checkers for next command
				current_act++;
				if(current_act == rake_file.actsets[current_actset]->total_actions){
					all_act_sent = true;
				}
            }
            
		}
	}

	// Clean up
	for (int i = 0; i < rake_file.total_actionsets; i++) {
		for (int j = 0; j < rake_file.actsets[i]->total_actions; j++) {
			for (int k = 0; k < rake_file.actsets[i]->acts[j]->total_files; k++) {
				free(rake_file.actsets[i]->acts[j]->required_files[k]);
			}
			if (rake_file.actsets[i]->acts[j]->total_files != 0) {
				free(rake_file.actsets[i]->acts[j]->required_files);
			}
			free(rake_file.actsets[i]->acts[j]->command);
			free(rake_file.actsets[i]->acts[j]);
		}
		free(rake_file.actsets[i]->acts);
		free(rake_file.actsets[i]);
	}
	free(rake_file.actsets);

	if (rake_file.hosts != NULL) {
		for (int i = 0; i < rake_file.total_hosts; i++) {
			free(rake_file.hosts[i]);
		}
		free(rake_file.hosts);
	}
	exit(errno);
}
