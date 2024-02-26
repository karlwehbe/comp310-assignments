CC=gcc
# Ensure CFLAGS is set correctly with -D options
CFLAGS=-g -D FRAMESIZE=$(framesize) -D VARMEMSIZE=$(varmemsize)

# List of source files
SOURCES=shell.c interpreter.c shellmemory.c kernel.c pcb.c ready_queue.c
# Convert source filenames to object filenames
OBJECTS=$(SOURCES:.c=.o)

# Target to build the executable
myshell: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Generic rule for building object files from C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean the build
clean: 
	rm -f myshell $(OBJECTS)