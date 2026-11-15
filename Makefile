CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Iinclude
SRCS = src/*.c

all:
	$(CC) $(CFLAGS) $(SRCS) -o minihttpd

clean:
	rm -f minihttpd