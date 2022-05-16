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

void execute_actionset() {
	pid_t PID;
	int child_exit_status;
	for (int i = 0; i < rake_file.total_actionsets; i++) {
		int terminated_child = 0;
		for (int j = 0; j < rake_file.actsets[i]->total_actions; j++) {
			int nwords;
			char **command_args = strsplit(rake_file.actsets[i]->acts[j]->command, &nwords);
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
				}
			} else if (PID > 0) {
				free_words(command_args);
			} else {
				perror("Failed to fork");
			}
		}
		while(terminated_child != rake_file.actsets[i]->total_actions){
			if(wait(&child_exit_status)){
				// if fail do something... 
				printf("Child process exited with %d status\n", WEXITSTATUS(child_exit_status));
				terminated_child++;
			}
		}
	}
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

int sendall(int s, char *buf, int *len){
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
	char buf[1024];

	for(int i = 0; i < rake_file.total_hosts; i++){
		newfd = socket(AF_INET, SOCK_STREAM, 0);
		printf("newfd = %i\n", newfd);
		if(newfd < 0){
			perror("Fatal: Failed to create socket\n");
		}
		if(newfd > fdmax){
			fdmax = newfd + 1;
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
		}
	}

	for(int i = 0; i <= fdmax; i++){
		if(FD_ISSET(i, &master)){
			printf("socket desc = %i\n", i);
		}
	}

	for(;;){
		read_fd = master;
		write_fd = master;
		if(select(fdmax, &read_fd, &write_fd, NULL, NULL) < 0){
			perror("Failed in selecting socket");
			break;
		}

		for(int i = 0; i <= fdmax; i++){
			// If something is ready to read
			// if(FD_ISSET(i, &read_fd)){
			// 	int nbytes = recv(i, buf, sizeof(buf), 0);
			// 	if (nbytes <= 0){
			// 		if(nbytes == 0){
			// 			printf("Disconnected from socket %d\n", i);
			// 		}
			// 		else{
			// 			perror("Error: failed to recieve");
			// 		}
			// 		close(i);
			// 		FD_CLR(i, &master);
			// 	}
			// 	else{
			// 		// For now, write everything you recieve back to other connected sockets
			// 		// except for the one you recieved from
			// 		for(int j = 0; j <= fdmax; j++){
			// 			// if j is ready to write
			// 			if(FD_ISSET(j, &write_fd)){
			// 				int len = strlen(buf);
			// 				int send_stat = sendall(j, buf, &len);
			// 				if(send_stat < 0){
			// 					perror("Error: Failed to send");
			// 				}
			// 			}
			// 		}
			// 	}
			// }
			if(FD_ISSET(i, &write_fd)){
				printf("IN");
				printf("%i\n", i);
				char *str = "THIS IS A MESSAGE";
				int len = strlen(str);
				int send_stat = sendall(i, str, &len);
				if(send_stat < 0){
					perror("Error: Failed to send");
				}
			}
		}
	}

	// int socket_desc[rake_file.total_hosts];
	// struct sockaddr_in server[rake_file.total_hosts];
	// for (int i = 0; i < rake_file.total_hosts; i++)
	// {
	// 	socket_desc[i] = socket(AF_INET, SOCK_STREAM, 0);
	// 	if(socket_desc[i] < 0){
	// 		perror("Fatal: Failed to create socket\n");
	// 	}

	// 	server[i].sin_family = AF_INET;
	// 	{	
	// 		char *colon = strchr(rake_file.hosts[i], ':');
	// 		if(colon != NULL){
	// 			char *host_no_port = strip_port(rake_file.hosts[i]);
	// 			printf("host no port = %s\n", host_no_port);
	// 			server[i].sin_addr.s_addr = inet_addr(host_no_port);
	// 			free(host_no_port);
	// 			server[i].sin_port = htons(atoi(colon + 1));
	// 		}
	// 		else{
	// 			server[i].sin_port = htons(rake_file.port);
	// 			server[i].sin_addr.s_addr = inet_addr(rake_file.hosts[i]);
	// 		}
	// 	}
	// 	int conn_stat = connect(socket_desc[i], (struct sockaddr *) &server[i], sizeof(server[i]));
	// 	if(conn_stat < 0){
	// 		printf("Connecting to Address = %s, Port = %i\n", rake_file.hosts[i], ntohs(server[i].sin_port));
	// 		perror("Failed to connect to server");
	// 	}
	// 	else{
	// 		printf("Connected to Address = %s, Port = %i\n", rake_file.hosts[i], ntohs(server[i].sin_port));
	// 	}
	// }
	

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
