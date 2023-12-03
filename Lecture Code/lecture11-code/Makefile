CFLAGS = -Wall -Werror -g
CC = gcc $(CFLAGS)

block_sigint: block_sigint.c
	$(CC) -o $@ $^

clean:
	rm -f block_sigint
