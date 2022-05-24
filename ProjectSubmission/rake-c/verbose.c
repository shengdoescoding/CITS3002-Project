#include "verbose.h"

bool VERBOSE = false;

void set_verbose(bool setting) {
    VERBOSE = setting;
}

int verbose(const char * restrict format, ...) {
    if( !VERBOSE )
        return 0;

    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);

    return ret;
}