#ifndef rake_client_h
#define rake_client_h

int CURR_ACTSET_INDEX;
int CURR_ACT_INDEX;
char **strsplit(const char *str, int *nwords);
void free_words(char **words);
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define RAKE_FILE_DIR "rakefiles/"
#define MAX_LINE_LEN 128

struct Rake_File {
	int port;
	int total_hosts;
	char **hosts;
	int total_actionsets;
	struct Actionset **actsets;
} rake_file;

struct Actionset {
	int total_actions;
	struct Action **acts;
};

struct Action {
	bool remote;
	char *command;
	int total_files;
	char **required_files;
};

#endif