# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -O3
LDFLAGS =

# Targets
TARGET = main
OBJS = main.o chunk.o vm.o debug.o memory.o value.o compiler.o scanner.o

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
main.o: common.h chunk.h vm.h debug.h
chunk.o: chunk.h memory.h common.h value.h
vm.o: common.h debug.h compiler.h vm.h chunk.h value.h
debug.o: debug.h value.h chunk.h
memory.o: memory.h common.h
value.o: value.h memory.h common.h
compiler.o: common.h compiler.h scanner.h
scanner.o: common.h scanner.h

# Clean up build artifacts
clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: all clean