CC = gcc
CFLAGS += -Wall -std=c99 -pedantic
LDFLAGS +=
OS = $(shell uname)

ifeq ($(OS), Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
else ifeq ($(OS), FreeBSD)
	CFLAGS += -D__BSD_VISIBLE
endif

HDR = *.h
DST = yaskk
SRC = yaskk.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
