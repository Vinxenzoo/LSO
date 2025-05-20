CC = gcc
CFLAGS = -Wall -Wextra -O3
TARGET = client
OBJS = main.o function.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c function.h
	$(CC) $(CFLAGS) -c main.c

funzioni.o: function.c function.h datastructures.h
	$(CC) $(CFLAGS) -c function.c

clean:
	rm -f $(OBJS) $(TARGET)