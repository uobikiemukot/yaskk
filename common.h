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
	CTRL_G = 0x07,
	CTRL_H = 0x08,
	CTRL_J = 0x0A,
	CTRL_L = 0x0C,
	CTRL_M = 0x0D,
	CTRL_N = 0x0E,
	CTRL_P = 0x10,
	ESC = 0x1B,
	SPACE = 0x20,
	DEL = 0x7F,
};

enum misc {
	DEBUG = false,
	RESET = 0x00,
	BUFSIZE = 1024,
	SELECT_TIMEOUT = 20000,
	INPUT_LIMIT = 6,
	MAX_ARGS = 512,
	TABLE_SIZE = 512,
	MULTIPLIER = 37, // or 31
};

enum mode {
	MODE_ASCII = 0x01,
	MODE_HIRA = 0x02,
	MODE_KATA = 0x04,
	MODE_COOK = 0x08,
	MODE_APPEND = 0x10,
	MODE_SELECT = 0x20,
};

enum select {
	SELECT_EMPTY = -1,
	SELECT_LOADED = 0,
};

const char *mode_str[] = {
	[MODE_ASCII] = "ascii",
	[MODE_HIRA] = "hira",
	[MODE_KATA] = "kata",

	[MODE_HIRA | MODE_COOK] = "hira|cook",
	[MODE_HIRA | MODE_SELECT] = "hira|select",
	[MODE_HIRA | MODE_APPEND] = "hira|append",

	[MODE_KATA | MODE_COOK] = "kata|cook",
	[MODE_KATA | MODE_SELECT] = "kata|select",
	[MODE_KATA | MODE_APPEND] = "kata|append",
};

struct list_t {
	char c;
	struct list_t *next;
};

struct hash_value_t {
	char *str;
	struct hash_value_t *next;
};

struct hash_t {
	//int count;
	char *key;
	struct hash_value_t *values;
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
	struct map_t rom2kana;                /* romaji to kana map */
	struct table_t okuri_ari, okuri_nasi; /* convert table */
	struct hash_t *user_dict[TABLE_SIZE]; /* hash table size of user dict */
	struct list_t *key;                   /* keyword string for display */
	struct list_t *append;                /* append string for append mode */
	struct list_t *preedit;               /* preedit string for map lookup */
	struct parm_t parm;                   /* data for candidate */
};
