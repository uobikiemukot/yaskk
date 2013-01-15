CC = gcc
CFLAGS += -Wall -std=c99 -pedantic
LDFLAGS +=
OS = $(shell uname)

ifeq ($(OS), Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
endif

HDR = *.h
DST = yaskk
SRC = yaskk.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
