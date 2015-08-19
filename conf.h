/* misc */
char * const shell_cmd = "/bin/bash";

/* load files */
const char *skkdict_file  = "/path/to/dict";

/* mark: these strings must be ONE character and non ascii */
/*
const char *mark_cook   = "▽";
const char *mark_select = "▼";
const char *mark_append = "*";
*/

/* escape sequence */
const char *backspace = "\010";

/* key */
enum key {
	KEY_ASCII     = 'l',
	KEY_CURSIVE   = CTRL_J,
	KEY_TOGGLE    = 'q',
	KEY_SELECT    = SPACE,
	//KEY_BACKSPACE = CTRL_H,
	KEY_NEXT      = SPACE,
	KEY_PREV      = 'x',
};

enum mark {
	MARK_COOK =   0x25BD, /* ▽ */
	MARK_SELECT = 0x25BC, /* ▼ */
	MARK_APPEND = 0x2A,   /* * */
};

enum {
	VERBOSE = false,
};
