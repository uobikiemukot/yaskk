#define _XOPEN_SOURCE 600
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "conf.h"

#define SIGWINCH 28

enum ctrl_chars {
	NUL = 0x00,
	ETX = 0x05,
	BS = 0x08,  /* backspace */
	LF = 0x0A,  /* LF */
	FF = 0x0C,
	ESC = 0x1B,
	CR = 0x0D,
	SO = 0x0E,  /* ctrl + n */
	DLE = 0x10, /* ctrl + p */
	SPACE = 0x20,
	DEL = 0x7F,
};

enum misc {
	DEBUG = true,
	RESET = 0x00,
	BUFSIZE = 1024,
	SELECT_TIMEOUT = 20000,
	INPUT_LIMIT = 6,
	NHASH = 512,
	MULTIPLIER = 37, // or 31
	KEYSIZE = 256,
	VALSIZE = 1024,
	MAX_ARGS = 256,
};

enum mode {
	MODE_ASCII = 0x01,
	MODE_HIRA = 0x02,
	MODE_KATA = 0x04,
	MODE_COOK = 0x08,
	MODE_APPEND = 0x10,
	MODE_SELECT = 0x20,
	MODE_DICT = 0x40,
};

enum select {
	SELECT_EMPTY = -1,
	SELECT_LOADED = 0,
};

const char *mode_str[] = {
	[MODE_ASCII] = "ascii",
	[MODE_HIRA] = "hira",
	[MODE_KATA] = "kata",

	[MODE_HIRA | MODE_DICT] = "hira|dict",
	[MODE_HIRA | MODE_DICT] = "hira|dict",
	[MODE_HIRA | MODE_DICT] = "hira|dict",

	[MODE_HIRA | MODE_COOK] = "hira|cook",
	[MODE_HIRA | MODE_SELECT] = "hira|select",
	[MODE_HIRA | MODE_APPEND] = "hira|append",

	[MODE_HIRA | MODE_COOK | MODE_DICT] = "hira|cook|dict",
	[MODE_HIRA | MODE_SELECT | MODE_DICT] = "hira|select|dict",
	[MODE_HIRA | MODE_APPEND | MODE_DICT] = "hira|append|dict",

	[MODE_KATA | MODE_COOK] = "kata|cook",
	[MODE_KATA | MODE_SELECT] = "kata|select",
	[MODE_KATA | MODE_APPEND] = "kata|append",

	[MODE_KATA | MODE_COOK | MODE_DICT] = "kata|cook|dict",
	[MODE_KATA | MODE_SELECT | MODE_DICT] = "kata|select|dict",
	[MODE_KATA | MODE_APPEND | MODE_DICT] = "kata|append|dict",
};

struct list_t {
	char c;
	struct list_t *next;
};

struct hash_t {
	int count;
	char *key, *values[MAX_ARGS];
	struct hash_t *next;
};

struct parm_t {
	int argc;
	char *argv[MAX_ARGS];
};

struct entry_t {
	char *key;
	long offset;
};

struct table_t {
	struct entry_t *entries;
	int count;
};

struct triplet_t {
	char *key, *hira, *kata;
};

struct map_t {
	struct triplet_t *triplets;
	int count;
};

struct skk_t {
	int fd;                               /* master of pseudo terminal */
	int mode;                             /* input mode */
	int pwrote, kwrote;                   /* count of wroted characters */
	int select;                           /* candidate select status */
	char entry[BUFSIZE];                  /* line buffer for dictionary */
	char stored_key[BUFSIZE];             /* stored key for dict mode */
	struct map_t rom2kana;                /* romaji to kana map */
	struct table_t okuri_ari, okuri_nasi; /* convert table */
	struct hash_t *user_dict[NHASH];      /* hash table of user dict */
	struct list_t *key;                   /* keyword string for table lookup */
	struct list_t *append;                /* append string for append mode */
	struct list_t *preedit;               /* preedit string for map lookup */
	struct list_t *dict;                  /* value for dict mode */
	struct parm_t parm;                   /* data for candidate */
};
