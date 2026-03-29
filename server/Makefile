CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -ggdb -O0
LDFLAGS ?= -lpthread -lrt
TARGET ?= aesdsocket

all: aesdsocket
default: aesdsocket

aesdsocket : aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o $(TARGET) $(INCLUDES) $(LDFLAGS)

aesdsocket.o : aesdsocket.c
	$(CC) -Wall -Werror -c aesdsocket.c

clean :
	rm -f aesdsocket aesdsocket.o