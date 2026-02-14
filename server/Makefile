CC=$(CROSS_COMPILE)gcc

all: aesdsocket
default: aesdsocket

aesdsocket : aesdsocket.o
	$(CC) -Wall -Werror -ggdb -O0 aesdsocket.o -o aesdsocket

aesdsocket.o : aesdsocket.c
	$(CC) -Wall -Werror -c aesdsocket.c

clean :
	rm -f aesdsocket aesdsocket.o