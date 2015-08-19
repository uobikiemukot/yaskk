CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c99 -pedantic -O3 -pipe -s
LDFLAGS ?=

HDR = *.h
DST = yaskk
SRC = yaskk.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
