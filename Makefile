# Compiler to use
CC = tcc

# Compiler flags
CFLAGS = -lm -lcurl

# Target binary name
TARGET = solfetch

# Source file
SRC = solfetch.c

# Default target
all: $(TARGET)

# Rule to build the binary
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS)

# Clean up generated files
clean:
	rm -f $(TARGET)

# Phony targets to avoid conflicts with files named 'all' or 'clean'
.PHONY: all clean
