#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define RAKE_FILE_DIR "rakefiles/"
#define MAX_LINE_LEN 128

struct Rake_File{
    int port;
    char *hosts;
    int total_actionsets;
    struct Actionset *actset;
} rake_file;
struct Actionset{
    int total_actions;
    struct Action *act;
};
struct Action{
    bool remote;
    char *command;
    int total_files;
    char **required_files;
};

int main(int argc, char const *argv[])
{
    if(argc <= 1){
        printf("Must provide rakefile\n");
        return EXIT_FAILURE;
    }
    else if(argc > 2){
        printf("Please provide only one rakefile\n");
        return EXIT_FAILURE;
    }

    char *temp_argv1;
    temp_argv1 = malloc(strlen(argv[1]) * sizeof(char));
    strcpy(temp_argv1, argv[1]);
    char *rake_file_address;
    rake_file_address = malloc((strlen(argv[1]) + strlen(RAKE_FILE_DIR)) * sizeof(char));
    if(rake_file_address == NULL){
        fprintf(stderr, "Fatal: failed to allocate memory for rake file address.\n");
        return EXIT_FAILURE;
    }
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

    char line[MAX_LINE_LEN];
    bool in_actionset = false;
    int actionset_number;
    while (fgets(line, MAX_LINE_LEN, fp)){
        // Ignore comments
        if(line[0] == '#'){
            continue;
        }
        // Read PORT number and store into structure
        if(strstr(line, "PORT") != NULL){
            int port_number = strtol(strtok(line, "PORT = "), NULL, 10);
            // Debug line below
            printf("%i\n", port_number);
            rake_file.port = port_number;
        }

        // if(strstr(line, "HOST") != NULL){
        //     char *host_name = strtok(line, "HOST = ");
        //     rake_file.hosts = malloc()
        // }

        // if(strstr(line, "actionset") != NULL){
        //     // Worrying way to convert char to int
        //     // Also still unused for now
        //     actionset_number = line[9] - '0';
        // }

        // Detect indentation
        // Assume one indentation is 4 spaces!
        int temp_indentation_size = 0;
        for(size_t i = 0; i < strlen(line); i++){
            if(line[i] == ' '){
                temp_indentation_size++;
                printf("%i\n", temp_indentation_size);
            }
            else{
                break;
            }
        }

        // Do something with the lines knowing its indentation
        if(temp_indentation_size == 4){
            printf("%s\n", line);

        }
    }

    free(rake_file_address);
    return 0;
}
