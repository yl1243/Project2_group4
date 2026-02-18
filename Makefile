#!/usr/bin/make -f

# variable to store the compiler name

CC := gcc

# variable to store the final executable file name

TARGETS := testcases

# variable to store compilation flags

CFLAGS := -O2 -Wall -Wextra -Werror

# If you type "make" without any specific target
# everything specified after "all" will be built.

all: $(TARGETS)

# Rules to create the executable files.

testcases: testcases.c thread_manager.c linked_list.c
	$(CC) $(CFLAGS) -o testcases testcases.c thread_manager.c linked_list.c -lpthread

# .PHONY: clean indicates that "clean" is just
# not a target file to build.
# We can use "make clean" command to delete
# object files and executable files to start
# the compilation fresh.

.PHONY: clean
clean:
	rm -rf *.o $(TARGETS)