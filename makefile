CC = gcc
CFLAGS = -Wall -std=c99 -pedantic \
	-march=native -Ofast -flto -pipe -Wall
LDFLAGS =

HDR = ../*.h
DST = yasf yass sortdic sortmap

all: $(DST)

yasf: yasf.c $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

yass: yass.c $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

sortdic: sortdic.c $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

sortmap: sortmap.c $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
