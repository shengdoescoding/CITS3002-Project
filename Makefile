CC=gcc
CFLAGS=-I. -Wall -Wextra -Wpedantic -g3 -O0
DEPS = protocol.h rake-c.h
OBJ = protocol.o rake-c.o strsplit.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rake-c: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJ).o