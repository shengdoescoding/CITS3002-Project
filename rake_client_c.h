#ifndef rake_client_c_h
#define rake_client_c_h

char **strsplit(const char *str, int *nwords);
void free_words(char **words);

#endif