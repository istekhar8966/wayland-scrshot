CC = gcc
CFLAGS = -Wall -Wextra -Werror

TARGET = main
SRC = main.c wayland.c protocol.c

all:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
