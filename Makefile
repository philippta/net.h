all: test chat

test: net.h test.c
	cc -o test test.c && ./test

chat: net.h chat.c
	cc -o chat chat.c 
