SRCS := server/aesdsocket.c

OBJS := $(SRCS:.c=.o)

CFLAGS ?= -Wall -Werror -ggdb -O0

LDFLAGS ?= -lpthread -lrt

TARGET ?= aesdsocket
CROSS_COMPILE?=
CC ?= $(CROSS_COMPILE)gcc


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean