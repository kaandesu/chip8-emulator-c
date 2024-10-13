
# Makefile for compiling main.c with Raylib

# Compiler
CC = gcc

# Output executable name
TARGET = main

# Source files
SRC = main.c

# Include and library paths
RAYLIB_INCLUDE = /opt/homebrew/opt/raylib/include
RAYLIB_LIB = /opt/homebrew/opt/raylib/lib

# Frameworks
FRAMEWORKS = -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL

# Compiler flags
CFLAGS = -I$(RAYLIB_INCLUDE)

# Linker flags
LDFLAGS = -L$(RAYLIB_LIB) -lraylib $(FRAMEWORKS)

# Default target
all: $(TARGET)

run: $(TARGET)
	@ ./$(TARGET)

# Build the target
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS) $(LDFLAGS)

# Clean target to remove the compiled executable
clean:
	rm -f $(TARGET)
