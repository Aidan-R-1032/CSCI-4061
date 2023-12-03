CFLAGS = -Wall -Werror -g
CC = gcc $(CFLAGS)

all: question_client_tcp question_server_tcp

question_client_tcp: question_client_tcp.c
	$(CC) -o $@ $^

question_server_tcp: question_server_tcp.c
	$(CC) -o $@ $^

clean:
	rm -f question_client_tcp question_server_tcp
