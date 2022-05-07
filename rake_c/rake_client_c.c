#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAKE_FILE_DIR "../rakefiles/"

int main(int argc, char const *argv[])
{
    if(argc <= 1){
        perror("Must provide rakefile");
        return EXIT_FAILURE;
    }
    else if(argc > 2){
        perror("Please provide only one rakefile");
        return EXIT_FAILURE;
    }

    char *temp;
    temp = malloc(strlen(argv[1]) * sizeof(char));
    strcpy(temp, argv[1]);
    char *rake_file;
    rake_file = malloc((strlen(argv[1]) + strlen(RAKE_FILE_DIR)) * sizeof(char));
    strcat(rake_file, RAKE_FILE_DIR);
    strcat(rake_file, temp);
    free(temp);
    // DEBUG FOR THIS SEGMENT
    printf("%s\n", rake_file);
    
    FILE *fp = fopen(rake_file, "r");
    if (fp == NULL){
        perror("Unable to open file");
    }

    /**Opportunity here to do some file modification to optimise reading
     * e.g. clear white spaces at the end of lines, clear empty lines etc.
     **/

    

    return 0;
}
