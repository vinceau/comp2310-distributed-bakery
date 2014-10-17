# the compiler
CC = gcc

# compiler flags:
#   -Wall        turns on most compiler warnings
#   -Wextra      adds extra warnings
#   -std=c99     compile to the c99 standard
#   -g           adds debugging information to the executable file

CFLAGS = -Wall -Wextra -std=c99 -g -I $$COMP2310/include bakery.a

# the build target executable
TARGET = distBakery

all: $(TARGET) 

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

clean:
	$(RM) $(TARGET) 
