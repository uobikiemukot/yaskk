CC = gcc
CFLAGS += -std=c99 -pedantic -Wall
LDFLAGS +=

HDR = ../*.h
DST = yasf yass
SRC = yaskk.c

all: $(DST)

yasf: yass.c $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

yass: yass.c $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
