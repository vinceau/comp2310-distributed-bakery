# the compiler
CC = gcc

# compiler flags:
#   -Wall        turns on most compiler warnings
#   -Wextra      adds extra warnings
#   -g           adds debugging information to the executable file

CFLAGS = -Wall -Wextra -g -I $$COMP2310/include bakery.a

# the build target executable
TARGET = distBakery

all: $(TARGET) 

$(TARGET): $(TARGET).c
	$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

clean:
	$(RM) $(TARGET) 
