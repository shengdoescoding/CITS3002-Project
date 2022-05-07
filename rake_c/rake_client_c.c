#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAKE_FILE_DIR "../rakefiles/"
#define MAX_LINE_LEN 128

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

    char *temp;
    temp = malloc(strlen(argv[1]) * sizeof(char));
    strcpy(temp, argv[1]);
    char *rake_file;
    rake_file = malloc((strlen(argv[1]) + strlen(RAKE_FILE_DIR)) * sizeof(char));
    if(rake_file == NULL){
        fprintf(stderr, "Fatal: failed to allocate memory for rake_file_address.\n");
        return EXIT_FAILURE;
    }
    strcat(rake_file, RAKE_FILE_DIR);
    strcat(rake_file, temp);
    free(temp);
    // DEBUG FOR THIS SEGMENT
    printf("%s\n", rake_file);
    
    FILE *fp = fopen(rake_file, "r");
    if (fp == NULL){
        perror("Unable to open file");
        return EXIT_FAILURE;
    }

    /**Opportunity here to do some file modification to optimise reading
     * e.g. clear white spaces at the end of lines, clear empty lines etc.
     **/

    char **rake_file_info;
    char line[MAX_LINE_LEN];
    int indentation_size = 0;
    while (fgets(line, MAX_LINE_LEN, fp)){
        // Ignore comments
        if(line[0] == '#'){
            continue;
        }

        // Detect indentation
        int temp_indentation_size = 0;
        for(int i = 0; i < strlen(line); i++){
            if(line[i] == ' '){
                temp_indentation_size++;
            }
            else{
                break;
            }
        }
        if(indentation_size == 0){
            indentation_size = temp_indentation_size;
        }

        // Do something with the lines knowing its indentation
        if(temp_indentation_size == 0){
            printf("%s\n", line);

        }
    }

    free(rake_file);
    return 0;
}
