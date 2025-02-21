# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -O3
LDFLAGS = -Wall

# Targets
TARGET = main
OBJS = compiler.o memory.o chunk.o debug.o value.o object.o main.o table.o vm.o scanner.o

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dependencies
main.o: common.h chunk.h vm.h debug.h main.c
chunk.o: chunk.h memory.h common.h value.h chunk.c
vm.o: common.h debug.h compiler.h memory.h object.h vm.h chunk.h value.h table.h vm.c
debug.o: debug.h value.h chunk.h debug.c
memory.o: memory.h vm.h common.h object.h memory.c
value.o: value.h memory.h object.h common.h value.c
compiler.o: common.h compiler.h scanner.h debug.h vm.h object.h compiler.c
object.o: memory.h object.h table.h vm.h common.h value.h object.c
table.o: table.h value.h object.h memory.h common.h value.h table.c
scanner.o: common.h scanner.h scanner.c

# Clean up build artifacts
clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: all clean