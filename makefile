CC = gcc
CFLAGS += -Wall -std=c99 -pedantic
LDFLAGS +=

HDR = *.h
DST = yaskk
SRC = yaskk.c

all: $(DST)

$(DST): $(SRC) $(HDR)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(DST)
