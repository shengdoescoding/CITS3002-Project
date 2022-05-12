#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "rake_client_c.h"

#define RAKE_FILE_DIR "rakefiles/"
#define MAX_LINE_LEN 128

struct Rake_File{
    int port;
    int total_hosts;
    char **hosts;
    int total_actionsets;   // Determine at the end of the wile loop by -1 from the last actionset number
    struct Actionset **actset;
} rake_file;
struct Actionset{
    int total_actions;
    struct Action **acts;
} actionset;
struct Action{
    bool remote;
    char *command;
    int total_files;
    char **required_files;
};

void init_rake_file(){
    rake_file.port = -1;
    rake_file.total_hosts = 0;
    rake_file.total_actionsets = 0;
    rake_file.hosts = NULL;
    rake_file.actset = NULL;
}

void init_actionset(){
    actionset.total_actions = 0;
    actionset.acts = NULL;
}

struct Action * init_action(){
    struct Action *action;
    action = malloc(sizeof(struct Action));
    action->remote = false;
    action->command = NULL;
    action->required_files = NULL;
    action->total_files = 0;
    return action;
}

void mem_alloc_check(const void *p, char *var_name){
    if(p == NULL){
        fprintf(stderr, "Fatal: failed to allocate memory for %s", var_name);
        exit(EXIT_FAILURE);
    }
}

char *trim_white_space(char *str){
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) {
        return str;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

bool prefix(const char *str, const char *pre){
    return strncmp(pre, str, strlen(pre)) == 0;
}

void add_port(const char *line){
    int nwords;
    char **port = strsplit(line, &nwords);
    int port_number = atoi(port[2]);
    rake_file.port = port_number;
    free_words(port);
}

void add_command(struct Action *action, const char *line_no_whitespace){
    if(prefix(line_no_whitespace, "remote-cc") == true){
        action->remote = true;
        action->command = realloc(action->command, (strlen(line_no_whitespace) - 7) * sizeof(char) + 1);
        mem_alloc_check(action->command, "action.command");
        strcpy(action->command, &line_no_whitespace[7]);
    }
    else{
        action->remote = false;
        action->command = realloc(action->command, strlen(line_no_whitespace) * sizeof(char) + 1);
        mem_alloc_check(action->command, "action.command");
        strcpy(action->command, line_no_whitespace);
    }
}

void add_action_to_actionset(struct Action *action){
    actionset.acts = realloc(actionset.acts, sizeof(struct Action *) * actionset.total_actions);
    mem_alloc_check(actionset.acts, "actuibset.acts");
    actionset.acts[actionset.total_actions - 1] = malloc(sizeof(struct Action));
    mem_alloc_check(actionset.acts[actionset.total_actions - 1], "actionset.acts[actionset.total_actions - 1]");
    actionset.acts[actionset.total_actions - 1] = action;
}

void add_hosts(const char *line){
    int nwords;
    char **hosts = strsplit(line, &nwords);
    rake_file.total_hosts = nwords - 2;
    rake_file.hosts = malloc(rake_file.total_hosts * sizeof(char*));
    mem_alloc_check(rake_file.hosts, "rake_file.hosts");
    int i = 0;
    for(int j = 2; j < nwords; j++){
        rake_file.hosts[i] = malloc(strlen(hosts[j]) * sizeof(char) + 1);
        mem_alloc_check(rake_file.hosts[i], "rake_file.hosts[i]");
        strcpy(rake_file.hosts[i], hosts[j]);
        i++;
    }
    free_words(hosts);
}

int main(int argc, char const *argv[])
{
    if(argc <= 1){
        printf("Must provide rakefile\n");
        exit(EXIT_FAILURE);
    }
    else if(argc > 2){
        printf("Please provide only one rakefile\n");
        exit(EXIT_FAILURE);
    }

    char *temp_argv1;
    temp_argv1 = malloc(strlen(argv[1]) * sizeof(char) + 1);
    mem_alloc_check(temp_argv1, "temp_argv1");
    strcpy(temp_argv1, argv[1]);
    char *rake_file_address;
    rake_file_address = malloc((strlen(argv[1]) + strlen(RAKE_FILE_DIR)) * sizeof(char) + 1);
    mem_alloc_check(rake_file_address, "rake_file_address");
    strcpy(rake_file_address, RAKE_FILE_DIR);
    strcat(rake_file_address, temp_argv1);
    free(temp_argv1);
    
    init_rake_file();
    init_actionset();

    FILE *fp = fopen(rake_file_address, "r");
    if (fp == NULL){
        perror("Unable to open file");
        return EXIT_FAILURE;
    }
    char line[MAX_LINE_LEN];
    int actionset_number = 0;
    while (fgets(line, MAX_LINE_LEN, fp)){
        // Detect indentation
        // Assume one indentation is 4 spaces!
        int temp_indentation_size = 0;
        for(size_t i = 0; i < strlen(line); i++){
            if(line[i] == ' '){
                temp_indentation_size++;
            }
            else{
                break;
            }
        }

        char *line_no_whitespace = trim_white_space(line);

        // Ignore comments
        if(line_no_whitespace[0] == '#'){
            continue;
        }

        // Read PORT number and store into structure
        if(strstr(line, "PORT") != NULL && temp_indentation_size == 0){
            add_port(line);
        }

        // Read HOST and store into structure
        if(strstr(line, "HOST") && temp_indentation_size == 0){
            add_hosts(line);
        }

        // Detect actionset
        if(strstr(line, "actionset") != NULL && temp_indentation_size == 0){
            actionset_number = line[9] - '0';
        }

        // Detect command
        if(temp_indentation_size == 4){
            struct Action *action = init_action();        
            actionset.total_actions++;
            add_command(action, line_no_whitespace);
            // Store into actionset
            add_action_to_actionset(action);
        }

        if(temp_indentation_size == 8){
            // Populate action in actionset with required files
            int nwords;
            char **required_files = strsplit(line, &nwords);
            actionset.acts[actionset.total_actions - 1]->total_files = nwords - 1;
            actionset.acts[actionset.total_actions - 1]->required_files = malloc(actionset.acts[actionset.total_actions - 1]->total_files * sizeof(char*));
            mem_alloc_check(actionset.acts[actionset.total_actions - 1]->required_files, "actionset.acts[actionset.total_actions - 1]->required_files");

            int i = 0;
            for(int j = 1; j < nwords; j++){
                actionset.acts[actionset.total_actions - 1]->required_files[i] = malloc(strlen(required_files[j]) * sizeof(char) + 1);
                mem_alloc_check(actionset.acts[actionset.total_actions - 1]->required_files[i], "actionset.acts[actionset.total_actions - 1]->required_files[i]");
                strcpy(actionset.acts[actionset.total_actions - 1]->required_files[i], required_files[j]);
                i++;
            }

            free_words(required_files);
        }
    }

    // Debug
    printf("port = %i\n", rake_file.port);
    for (int i = 0; i < rake_file.total_hosts; i++)
    {
        printf("hosts = %s\n", rake_file.hosts[i]);
    }
    for (int i = 0; i < actionset.total_actions; i++)
    {
        printf("actionset acts commands = %s\n", actionset.acts[i]->command);
        for (int j = 0; j < actionset.acts[i]->total_files; j++)
        {
            printf("actionset acts total files = %s\n", actionset.acts[i]->required_files[j]);
        }
        
    }
    

    // Clean up
    fclose(fp);

    for(int i = 0; i < actionset.total_actions; i++){
        for(int j = 0; j < actionset.acts[i]->total_files; j++){
            free(actionset.acts[i]->required_files[j]);
        }
        free(actionset.acts[i]->required_files);
        free(actionset.acts[i]->command);
    }

    if(rake_file.hosts != NULL){
        for(int i = 0; i < rake_file.total_hosts; i++){
            free(rake_file.hosts[i]);
        }
        free(rake_file.hosts);
    }
    free(rake_file_address);
    return 0;
}
