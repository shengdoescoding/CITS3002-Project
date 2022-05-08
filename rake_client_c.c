#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
    struct Action **act;
} actionset;
struct Action{
    bool remote;
    char *command;
    int total_files;
    char **required_files;
} action;

void mem_alloc_check(const void *p, char *var_name){
    if(p == NULL){
        fprintf(stderr, "Fatal: failed to allocate memory for %s", var_name);
        exit(EXIT_FAILURE);
    }
}

void init_rake_file(){
    rake_file.port = -1;
    rake_file.total_hosts = -1;
    rake_file.total_actionsets = -1;
    rake_file.hosts = NULL;
    rake_file.actset = NULL;
}

void init_actionset(){
    actionset.total_actions = -1;
    actionset.act = NULL;
}

void init_action(){
    action.remote = false;
    action.command = malloc(1);
    mem_alloc_check(action.command, "action.command");
    action.total_files = -1;
    action.required_files = NULL;
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
    // // DEBUG FOR THIS SEGMENT
    // printf("%s\n", rake_file_address);
    
    FILE *fp = fopen(rake_file_address, "r");
    if (fp == NULL){
        perror("Unable to open file");
        return EXIT_FAILURE;
    }

    /**Opportunity here to do some file modification to optimise reading
     * e.g. clear white spaces at the end of lines, clear empty lines etc.
     **/
    init_rake_file();
    char line[MAX_LINE_LEN];
    int actionset_number = 0;
    bool in_action = false;
    while (fgets(line, MAX_LINE_LEN, fp)){
        // Ignore comments
        if(line[0] == '#'){
            continue;
        }
        // Read PORT number and store into structure
        if(strstr(line, "PORT") != NULL){
            int nwords;
            char **words = strsplit(line, &nwords);
            int port_number = atoi(words[2]);
            rake_file.port = port_number;
        }
        // Read HOST and store into structure
        if(strstr(line, "HOST") != NULL){
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
        if(strstr(line, "actionset") != NULL){
            init_actionset();
            // Worrying way to convert char to int, might need to be changed
            actionset_number = line[9] - '0';
            in_action = false;
        }

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
        // Do something with the line knowing its indentation
        if(temp_indentation_size == 4 && in_action == false){
            init_action();
            if(strstr(line, "remote") != NULL){
                action.remote = false;
                free(action.command);
                action.command = malloc(strlen(line) * sizeof(char));
                mem_alloc_check(action.command, "action.command");
                strcpy(action.command, line);
                in_action = true;
                actionset.total_actions++;
            }
        }
        if(temp_indentation_size == 8){
            int nwords;
            char **required_files = strsplit(line, &nwords);
            action.total_files = nwords - 1;
            action.required_files = malloc(action.total_files * sizeof(char*));
            mem_alloc_check(action.required_files, "action.required_files");
            int i = 0;
            for(int j = 1; j < nwords; j++){
                action.required_files[i] = malloc(strlen(required_files[j]) * sizeof(char));
                mem_alloc_check(action.required_files[i], "action.required_files[i]");
                strcpy(action.required_files[i], required_files[j]);
                i++;
            }
            if(actionset.act == NULL){
                actionset.act = malloc(sizeof(struct Action*));
                mem_alloc_check(actionset.act, "actionset.act");

            }
            actionset.act[actionset.total_actions] = malloc(sizeof(&action));
            mem_alloc_check(actionset.act, "actionset.act");
            actionset.act[actionset.total_actions] = &action;
            in_action = false;
        }
        // if(temp_indentation_size == 4 && in_action == true){
        //     if(actionset.act == NULL){
        //         actionset.act = malloc(sizeof(action));
        //         mem_alloc_check(actionset.act, "actionset.act");
        //     }
        //     init_action();
        // }

    }

    // Debug loop
    // for (int i = 0; i < actionset.act[0]->total_files; i++){
    //     printf("%s", actionset.act[0]->required_files[i]);
    // }

    for (int i = 0; i < actionset.total_actions; i++){
        free(actionset.act[i]);
    }
    free(actionset.act);
    for(int i = 0; i < action.total_files; i++){
        free(action.required_files[i]);
    }
    free(action.required_files);
    free(action.command);
    for(int i = 0; i < rake_file.total_hosts; i++){
        free(rake_file.hosts[i]);
    }
    free(rake_file.hosts);
    free(rake_file_address);
    return 0;
}
