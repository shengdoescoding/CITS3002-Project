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
    struct Action *act;
} actionset;
struct Action{
    bool remote;
    char *command;
    int total_files;
    char **required_files;
} action;

void init_rake_file(){
    rake_file.port = -1;
    rake_file.total_hosts = 0;
    rake_file.total_actionsets = 0;
    rake_file.hosts = NULL;
    rake_file.actset = NULL;
}

void init_actionset(){
    actionset.total_actions = 0;
    actionset.act = NULL;
}

void init_action(){
    action.remote = false;
    action.command = NULL;
    if(action.total_files != 0){
        for(int i = 0; i < action.total_files; i++){
            free(action.required_files[i]);
        }
        free(action.required_files);
    }
    action.required_files = NULL;
    action.total_files = 0;
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
    temp_argv1 = malloc(strlen(argv[1]) * sizeof(char));    // Although sizeof(char) == 1 and is thus redundant, it will be kept regardless for readibility
    mem_alloc_check(temp_argv1, "temp_argv1");
    strcpy(temp_argv1, argv[1]);
    char *rake_file_address;
    rake_file_address = malloc((strlen(argv[1]) + strlen(RAKE_FILE_DIR)) * sizeof(char));
    mem_alloc_check(rake_file_address, "rake_file_address");
    strcat(rake_file_address, RAKE_FILE_DIR);
    strcat(rake_file_address, temp_argv1);
    free(temp_argv1);
    
    init_rake_file();
    init_actionset();
    init_action();

    FILE *fp = fopen(rake_file_address, "r");
    if (fp == NULL){
        perror("Unable to open file");
        return EXIT_FAILURE;
    }
    char line[MAX_LINE_LEN];
    int actionset_number = 0;
    bool in_action = false;
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
            int nwords;
            char **words = strsplit(line, &nwords);
            int port_number = atoi(words[2]);
            rake_file.port = port_number;
        }
        // Read HOST and store into structure
        if(strstr(line, "HOST") && temp_indentation_size == 0){
            int nwords;
            char **hosts = strsplit(line, &nwords);
            rake_file.total_hosts = nwords - 2;
            rake_file.hosts = malloc(rake_file.total_hosts * sizeof(char*));
            mem_alloc_check(rake_file.hosts, "rake_file.hosts");
            int i = 0;
            for(int j = 2; j < nwords; j++){
                rake_file.hosts[i] = malloc(strlen(hosts[j]) * sizeof(char));
                mem_alloc_check(rake_file.hosts[i], "rake_file.hosts[i]");
                strcpy(rake_file.hosts[i], hosts[j]);
                i++;
            }
        }
        // Detect actionset
        if(strstr(line, "actionset") != NULL && temp_indentation_size == 0){
            actionset_number = line[9] - '0';
            in_action = false;
        }

        // Do something with the line knowing its indentation
        if(temp_indentation_size == 4 && in_action == false){
            // in_action == false, reset action first before populating
            actionset.total_actions++;
            in_action = true;
            if(prefix(line_no_whitespace, "remote-") == true){
                action.remote = true;
                if(action.command != NULL){
                    action.command = realloc(action.command, (strlen(line_no_whitespace) - 7) * sizeof(char));
                }
                else{
                    action.command = malloc((strlen(line_no_whitespace) - 7) * sizeof(char));
                }
                mem_alloc_check(action.command, "action.command");
                strcpy(action.command, &line_no_whitespace[7]);
            }
            else{
                action.remote = false;
                if(action.command != NULL){
                    action.command = realloc(action.command, strlen(line_no_whitespace) * sizeof(char));
                }
                else{
                    action.command = malloc(strlen(line_no_whitespace) * sizeof(char));
                }
                mem_alloc_check(action.command, "action.command");
                strcpy(action.command, line_no_whitespace);
            }
        }
        if(temp_indentation_size == 8){
            // Populate action
            int nwords;
            char **required_files = strsplit(line, &nwords);
            action.total_files = nwords - 1;
            if(action.required_files != NULL){
                action.required_files = realloc(action.required_files, action.total_files * sizeof(char*));
            }
            else{
                action.required_files = malloc(action.total_files * sizeof(char*));
            }
            mem_alloc_check(action.required_files, "action.required_files");

            int i = 0;
            for(int j = 1; j < nwords; j++){
                action.required_files[i] = malloc(strlen(required_files[j]) * sizeof(char));
                mem_alloc_check(action.required_files[i], "action.required_files[i]");
                strcpy(action.required_files[i], required_files[j]);
                i++;
            }

            // // Add populated action to actionset
            // if(actionset.act == NULL){
            //     actionset.act = malloc(sizeof(action));
            //     mem_alloc_check(actionset.act, "actionset.act");
            // }
            // else{
            //     actionset.act = realloc(actionset.act, sizeof(actionset.act) + sizeof(action));
            //     mem_alloc_check(actionset.act, "actionset.act");
            // }
            // actionset.act[actionset.total_actions - 1] = action;
            // // printf("total_actions - 1 = %i\n", actionset.total_actions - 1);
            // // printf("command from actionset = %s\n", actionset.act[actionset.total_actions - 1].command);
            // // printf("address of actionset.act[actionset.total_actions - 1] = %p\n", actionset.act[actionset.total_actions - 1]);
            // in_action = false;

            // Free required_files, since num of files required for next action might change, losing pointers to larger index hence mem leak

            for(int i = 0; i < action.total_files; i++){
                free(action.required_files[i]);
                action.required_files[i] = NULL;
            }
        }

        // if(temp_indentation_size == 4 && in_action == true){
        //     if(actionset.act == NULL){
        //         actionset.act = malloc(sizeof(struct Action*));
        //         mem_alloc_check(actionset.act, "actionset.act");

        //     }
        //     else{
        //         actionset.act = realloc(actionset.act, sizeof(struct Action*));
        //         mem_alloc_check(actionset.act, "actionset.act");
        //     }
        //     actionset.act[actionset.total_actions - 1] = malloc(sizeof(&action));
        //     actionset.act[actionset.total_actions - 1], "actionset.act[actionset.total_actions]");
        //     actionset.act[actionset.total_actions - 1] = &action;
        //     in_action = false;
        // }

    }

    // Clean up action
    free(action.required_files);
    action.required_files = NULL;
    free(action.command);
    action.command = NULL;

    free(actionset.act);
    for(int i = 0; i < rake_file.total_hosts; i++){
        free(rake_file.hosts[i]);
    }
    free(rake_file.hosts);
    free(rake_file_address);
    return 0;
}
