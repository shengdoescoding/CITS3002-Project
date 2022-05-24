/**
 * @file rake-c.c
 * @author ShengTang Zhao (22980212)
 * @brief Simple client that connects to multiple servers to practice socket programming
 */

#include "rake-c.h"
#include "verbose.h"

/**
 * @brief Initialise the single global instance of Rake_File structure
 */
void init_rake_file() {
	rake_file.port = -1;
	rake_file.total_hosts = 0;
	rake_file.total_actionsets = 0;
	rake_file.hosts = NULL;
	rake_file.actsets = NULL;
}

/**
 * @brief Initialise an instance of Actionset structure
 * 
 * @param actionset An instance of Actionset structure
 */
void init_actionset(struct Actionset *actionset) {
	actionset->total_actions = 0;
	actionset->acts = NULL;
}

/**
 * @brief Initialise an instance of Action structure
 * 
 * @param action An instance of Action structure
 */
void init_action(struct Action *action) {
	action->remote = false;
	action->command = NULL;
	action->required_files = NULL;
	action->total_files = 0;
}

/**
 * @brief Checks if memory allocation was successful, terminate process if memory allocation failed
 * 
 * @param p Pointer to memory address that was supposed to have memory allocated to it.
 * @param var_name Variable name of the pointer to the memory address. 
 */
void mem_alloc_check(const void *p, char *var_name) {
	if (p == NULL) {
		fprintf(stderr, "Fatal: failed to allocate memory for %s", var_name);
        exit(EXIT_FAILURE);
	}
}

/**
 * @brief Strip leading and trailing white space from str
 * 
 * @param str String with possible leading or trailing whitespace
 * @return char* Trimed string
 */
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

/**
 * @brief Check if a string started with a prefix
 * 
 * @param str String to check for prefix
 * @param pre The prefix
 * @return true if pre exists in str
 * @return false if pre does not exist in str
 */
bool prefix(const char *str, const char *pre) {
	return strncmp(pre, str, strlen(pre)) == 0;
}

/**
 * @brief Add the default port number from provided rakefile to the single global instance of Rake_File structure
 * 
 * @param line containing the port number, in form PORT = ...
 */
void add_port(const char *line) {
	int nwords;
	char **port = strsplit(line, &nwords);
	int port_number = atoi(port[2]);
	rake_file.port = port_number;
	free_words(port);
}

/**
 * @brief Add command to an instance of Action structure
 * 
 * @param action is an instance of Action structure
 * @param line_no_whitespace is the line in provided rakefile that contains the command,
 * with its leading and trailing whitespace removed
 */
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

/**
 * @brief Add an instance of Actionset struct to the single global instance of Rake_File structure
 * 
 * @param actionset an instance of structure Actionset
 */
void add_actionset_to_rakefile(struct Actionset *actionset) {
	rake_file.actsets = realloc(rake_file.actsets, sizeof(struct Actionset *) * rake_file.total_actionsets);
	mem_alloc_check(rake_file.actsets, "rake_file.actsets");
	rake_file.actsets[CURR_ACTSET_INDEX] = actionset;
}

/**
 * @brief Add an instance of Action structure to an instance of Actionset structure
 * 
 * @param action an instance of Action structure
 */
void add_action_to_actionset(struct Action *action) {
	rake_file.actsets[CURR_ACTSET_INDEX]->acts = realloc(rake_file.actsets[CURR_ACTSET_INDEX]->acts, sizeof(struct Action *) * rake_file.actsets[CURR_ACTSET_INDEX]->total_actions);
	mem_alloc_check(rake_file.actsets[CURR_ACTSET_INDEX]->acts, "rake_file.actsets[CURR_ACTSET_INDEX]->acts");
	rake_file.actsets[CURR_ACTSET_INDEX]->acts[CURR_ACT_INDEX] = action;
}

/**
 * @brief Add the provided hosts to the single global instance of Rake_File structure
 * 
 * @param line from provided rakefile that contains the hosts, in form: HOST = ... ...:port
 */
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

/**
 * @brief Parse the Rakefile in current working directory to the 
 * single global instance of Rake_File structure
 */
void parse_rf() {
	FILE *fp = fopen("Rakefile", "r");
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

/**
 * @brief Execute all commands in an actionset in parallel.
 * It is commented out because it is not used, however parts of it
 * has been adapted into the function execute_command(const char *command)
 * 
 */
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

/**
 * @brief Execute locally the provided command
 * 
 * @param command command to be executed locally, e.g. "echo run command"
 * @return int the exit status of the child process
 */
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
        verbose("Child PID = %d executing...\n", getpid());
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

    return 1;
}

/**
 * @brief Strips the non-default port form the host
 * 
 * @param host the host containing the non-default port, 
 * e.g. "123.123.123:9999", with "":9999" being the non-default port
 * @return char* host without the non-default port
 */
char *strip_port(char *host){
	char *host_no_port;
	host_no_port = malloc(strlen(host) * sizeof(char) + 1);
	mem_alloc_check(host_no_port, "host_no_port");
	strcpy(host_no_port, host);
	char *colon_index = strchr(host_no_port, ':');
	*colon_index = '\0';
	return host_no_port;
}

int main(int argc, char * const argv[]) {
    if(argc > 2){
        fprintf(stderr, "Fatal: please only provide -v for verbose mode\n");
        exit(EXIT_FAILURE);
    }
    else if (argc == 1){
        printf("No arguments provided, please use -v if verbose mode is required.\n");
    }

    int option;
    while((option = getopt(argc, argv, "v")) != -1){ //get option from the getopt() method
        switch(option){
            case 'v':
                set_verbose(true);
                break;
            case '?':
                fprintf(stderr, "Fatal: unrecognised argument, please only provide -v for verbose mode\n");
                exit(EXIT_FAILURE);
        }
    }

	init_rake_file();
	parse_rf();

	fd_set master;
	fd_set read_fd;
	fd_set write_fd;
	int fdmax = 0;
	int newfd;	// socket descriptor

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
            fprintf(stderr, "Error connecting to address %s, port %i: %s\n", rake_file.hosts[i], ntohs(server.sin_port), strerror(errno));
			exit(EXIT_FAILURE);
		}
		else{
			FD_SET(newfd, &master);
		}
	}

	// Used to access data from rake_file
	int current_actset = 0;
	int current_act = 0;

	// Used to keep track of host with lowest load
	uint32_t lowest_load = __INT32_MAX__;
	int lowest_load_socket = -1;

	// Buffer for data sent by server
	unsigned char int_data_bytes[SIZEOF_INT];
	
	// Used to terminate infinite event loop
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
				verbose("All actionset succesfully executed, shutting down all sockets...\n");
			}
			else if(command_exec_error){
				verbose("An error occured when executing remote commands, shutting down all sockets...\n");
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
						verbose("Sending load query to socket %i ...\n", i);
						if(send_instruction(i, ISLOADQUERY) < 0){
							perror("Error: could not send load query");
						}
						load_queried[i] = true;
						total_quries++;
					}

					if(all_act_sent == true && command_sent[i] == true){
						verbose("Sending execute all command request to %i\n", i);
						if(send_instruction(i, ALLCOMMANDSSENT) < 0){
							perror("Error: could not inform server that all commands were sent");
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
					if (data_type_int == 0){
						verbose("Socket %i closed unexpectedly by server", i);
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
						verbose("Socket %i returned server load = %i\n", i, server_load);
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
						verbose("Recieving file from server ...\n");
						nbytes = recv(i, int_data_bytes, SIZEOF_INT, 0);
						if(nbytes < 0){
							perror("Error: failed to recieve");
						}
						uint32_t file_name_len = unpack_uint32(int_data_bytes);

						char file_name[file_name_len + 1];	// +1 for null-terminator byte
						nbytes = recv(i, file_name, file_name_len, 0);
						if(nbytes < 0){
							perror("Error: failed to recieve");
						}
						file_name[file_name_len] = '\0';

						nbytes = recv(i, int_data_bytes, SIZEOF_INT, 0);
						if(nbytes < 0){
							perror("Error: failed to recieve");
						}
						uint32_t file_size = unpack_uint32(int_data_bytes);

						FILE *fp = fopen(file_name, "wb");
						if (fp == NULL) {
							perror("Unable to open file");
						}
						uint32_t remaining_data = file_size;
						char *buff = malloc(file_size);
						while((remaining_data > 0) && ((nbytes = recv(i, buff, file_size, 0)) > 0)){
							fwrite(buff, sizeof(char), nbytes, fp);
							remaining_data -= nbytes;
							verbose("Received %d bytes, %d bytes remaining\n", nbytes, remaining_data);
						}
						fclose(fp);
						free(buff);
						if(send_instruction(i, FILERECIEVED) < 0){
							perror("Error: could not send load query");
						}
					}
				}
			}
		
		if(all_act_sent == false && actsets_finished == false){
			if(all_act_sent == false && rake_file.actsets[current_actset]->acts[current_act]->remote == true && total_quries == 0 && FD_ISSET(lowest_load_socket, &write_fd)){
				if(rake_file.actsets[current_actset]->acts[current_act]->total_files == 0){
					verbose("Sending command /%s/ to socket %i\n", rake_file.actsets[current_actset]->acts[current_act]->command, lowest_load_socket);
					if(send_command(lowest_load_socket, current_actset, current_act) < 0){
						perror("Failed to send command");
					}
				}
				else{
        			verbose("Current command /%s/ requires files!\n", rake_file.actsets[current_actset]->acts[current_act]->command);
					for (int i = 0; i < rake_file.actsets[current_actset]->acts[current_act]->total_files; i++){
						verbose("Sending file %s to socket %i\n", rake_file.actsets[current_actset]->acts[current_act]->required_files[i], lowest_load_socket);
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
								if(data_type_int == FILERECIEVED){
                                    verbose("Server recieved file\n");
									break;
								}
							}
						}
					}
					verbose("All required file sent!\n Sending command /%s/ to socket %i\n", rake_file.actsets[current_actset]->acts[current_act]->command, lowest_load_socket);
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
