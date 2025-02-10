# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS =

# Targets
TARGET = main
OBJS = main.o chunk.o memory.o debug.o value.o

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
main.o: main.c common.h chunk.h memory.h debug.h value.h
chunk.o: chunk.c chunk.h common.h memory.h value.h
memory.o: memory.c memory.h common.h
debug.o: debug.c debug.h chunk.c chunk.h value.h
value.o: value.c value.h memory.h common.h 

# Clean up build artifacts
clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: all clean