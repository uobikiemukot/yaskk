/* line edit functions
 *
 * struct line_t {
 *     uint32_t cells[MAX_CELLS]; // UCS2 codepoint
 *     struct cursor_t {
 *         int insert;
 *         int preedit;
 *     };
 * };
 *
 * cells[MAX_CELLS]: line buffer of editable character
 *                   each cell stands for UCS2 codepoint
 * +----------+----------+----------+   +----------------------+
 * | cells[0] | cells[1] | cells[2] |...| cells[MAX_CELLS - 1] |
 * +----------+----------+----------+   +----------------------+
 *
 * struct corsor_t:
 * - insert : instert position of next character
 * - preedit: first position of preedit character
 *
 * MODE_ASCII:
 *
 * +---+--------+                     +--------+
 * | a | (null) |  -- after update -> | (null) |
 * +---+--------+                     +--------+
 *         ^                              ^
 *         | insert                       | insert
 *
 * after line update, line buffer is immediately flushed (set need_flash flag)
 * insert and preedit are always "0"
 *
 * MODE_CURSIVE or MODE_SQUARE:
 *
 * +---+---+--------+
 * | s | h | (null) |
 * +---+---+--------+
 *   ^         ^
 *   |         | cursor.insert
 *   | cursor.preedit
 *
 * preedit is always "0"
 *
 * MODE_COOK:
 *
 * +---+----+---+--------+
 * | ▽ | あ | c | (null) |
 * +---+----+---+--------+
 *            ^     ^
 *            |     | insert
 *            | preedit
 */

/* utility functions */
void line_init(struct line_t *line)
{
	memset(line->cells, '\0', sizeof(uint32_t) * MAX_CELLS);
	line->cursor.insert  = 0;
	line->cursor.preedit = 0;
}

int line_length(struct line_t *line)
{
	return line->cursor.insert;
}

int preedit_length(struct line_t *line)
{
	return (line->cursor.insert - line->cursor.preedit);
}

int cook_length(struct line_t *line)
{
	return (line->cursor.preedit - 1); /* ignore cook/select mark */
}

void line_show(struct line_t *line)
{
	size_t size;
	char utf[UTF8_LEN_MAX + 1];

	fprintf(stderr, "cursor insert:%d preedit:%d\n", line->cursor.insert, line->cursor.preedit);

	if (line->cursor.insert == 0) {
		fprintf(stderr, "| (nul) |\n");
		return;
	}

	for (int i = 0; i < line->cursor.insert; i++) {
		if ((size = utf8_encode(line->cells[i], utf)) > 0)
			fprintf(stderr, "| %s ", utf);
	}
	fprintf(stderr, "|\n");
}

void move_cursor(struct cursor_t *cursor, int offset, uint32_t ucs)
{
	/*
	 * if ucs is not ascii character, update preedit cursor too.
	 *     cook character must be multibyte UTF-8 sequence (non ascii character), so update preedit cursor.
	 */
	if (ucs > 0x7F || ucs == MARK_SELECT || ucs == MARK_COOK || ucs == MARK_APPEND)
		cursor->preedit += offset;

	/*
	 * always increment insert cursor
	 */
	cursor->insert += offset;

	assert(cursor->insert >= 0);
	assert(cursor->insert < MAX_CELLS);
	assert(cursor->preedit >= 0);
	assert(cursor->preedit < MAX_CELLS);
	assert(cursor->preedit <= cursor->insert);
}

/* remove functions */
void remove_ucs_char(struct line_t *line, int index)
{
	uint32_t ucs;

	assert(index >= 0);
	assert(index < line->cursor.insert);

	ucs = line->cells[index];

	if (index < (line->cursor.insert - 1)) /* not last char */
		memmove(line->cells + index, line->cells + index + 1, sizeof(uint32_t) * (line->cursor.insert - index - 1));

	move_cursor(&line->cursor, -1, ucs);
}

void remove_chars(struct line_t *line, int from, int to)
{
	int count = 0, length = to - from + 1;

	if (length > line_length(line)
		|| from < 0 || to < 0
		|| from >= line->cursor.insert
		|| to >= line->cursor.insert)
		return;

	/*
	assert(length <= line_length(line));
	assert(from >= 0);
	assert(from < line->cursor.insert);
	assert(to >= 0);
	assert(to < line->cursor.insert);
	*/

	while (count < length) {
		remove_ucs_char(line, from);
		count++;
	}
}

void remove_all_chars(struct line_t *line)
{
	line->cursor.insert  = 0;
	line->cursor.preedit = 0;
}

/* append functions */
void append_ucs_char(struct line_t *line, int index, uint32_t ucs)
{
	assert(index >= 0);
	assert(index <= line->cursor.insert);

	if (index < line->cursor.insert) /* not append to tail */
		memmove(line->cells + index + 1, line->cells + index, sizeof(uint32_t) * (line->cursor.insert - index));

	line->cells[index] = ucs;
	move_cursor(&line->cursor, +1, ucs);
}

int append_utf8_str(struct line_t *line, int index, const char *utf8_str)
{
	uint32_t ucs = '\0';
	int size, count = 0;
	const char *cp;

	cp = utf8_str;
	while (*cp != '\0') {
		size = utf8_decode(cp, &ucs);
		cp  += size;

		if (ucs != MALFORMED_CHARACTER) {
			append_ucs_char(line, index + count, ucs);
			count++;
		}
	}
	return count;
}

/* swap functions */
void swap_ucs_char(struct line_t *line, int index, uint32_t ucs)
{
	assert(index >= 0);
	assert(index < line->cursor.insert);

	line->cells[index] = ucs;
}

/* get string functions */
uint32_t get_ucs_char(struct line_t *line, int index)
{
	if (line_length(line) == 0)
		return '\0';

	assert(index >= 0);
	assert(index < line->cursor.insert);

	return line->cells[index];
}

int get_utf8_str(struct line_t *line, int from, int to, char *utf8_buf)
{
	char *cp;
	int size;

	//logging(DEBUG, "get_utf8_str(): from:%d to:%d\n", from, to);
	//line_show(line);

	/*
	assert(from >= 0);
	assert(from < line->cursor.insert);
	assert(to >= 0);
	assert(to < line->cursor.insert);
	*/

	if (from < 0 || to < 0
		|| from >= line->cursor.insert
		|| to >= line->cursor.insert) {
		return 0;
	}

	cp = utf8_buf;
	for (int i = from; i <= to; i++) {
		if ((size = utf8_encode(line->cells[i], cp)) > 0)
			cp += size;
	}
	return (cp - utf8_buf);
}

/* update function */
void send_backspace(int fd, int count)
{
	extern const char *backspace; /* global */

	for (int i = 0; i < count; i++)
		ewrite(fd, backspace, strlen(backspace));
}

void line_update(struct line_t *current, struct line_t *next, bool *need_flush, int fd)
{
	int pos, diff;
	char utf[UTF8_LEN_MAX + 1];
	size_t size;

	/*
	 * step1: check first different character between currentline and nextline
	 *
	 * current line (what you see on your displayed now):
	 * | a | b | c | d | e | (nul) |
	 *                         ^
	 *                         | current line cursor (index:5)
	 * next line:
	 * | a | b | d | e |(nul) |
	 *           ^        ^
	 *           |        | next line cursor (index:4)
	 *           | first different char (index:2)
	 *
	 * pos of first different character: 2
	 *
	 * step2: remove unmatched characters
	 *
	 * number of unmatched character
	 * = currentline cursor (index:5) - first different char (index:2) = 3
	 * So, send backspace 3 times (remove c, d and e)
	 *
	 * current display status
	 * | a | b | (null) |
	 *
	 * step3: write new characters and change next line status to current line status
	 *
	 * add characters from index:2 (pos of first different char) to index:3 (next line cursor - 1)
	 * (append d and e)
	 *
	 * current line (next line is the same status):
	 * | a | b | d | e | (null) |
	 *
	 * (step4: display flushing)
	 *
	 * if need_flush flag is set, remove all characters of current/next line
	 *
	 */

	/* step1 */
	for (pos = 0; pos < current->cursor.insert; pos++) {
		if (current->cells[pos] != next->cells[pos]
			|| pos >= next->cursor.insert)
			break;
	}
	logging(DEBUG, "pos of first different character:%d\n", pos);

	/* step2 */
	diff = current->cursor.insert - pos;
	logging(DEBUG, "number of send backspace:%d\n", diff);
	if (diff > 0)
		send_backspace(fd, diff);

	/* step3 */
	logging(DEBUG, "update current line from %d to %d\n", pos, next->cursor.insert - 1);
	for (; pos < next->cursor.insert; pos++) {
		current->cells[pos] = next->cells[pos]; /* uposdate current line buffer */
		if ((size = utf8_encode(next->cells[pos], utf)) > 0)
			ewrite(fd, utf, size);
	}

	/* step4 */
	if (*need_flush) {
		logging(DEBUG, "do flush...\n");
		*need_flush = false;
		remove_all_chars(current);
		remove_all_chars(next);
	} else {
		current->cursor = next->cursor;
		/* let string as NUL terminate */
		//current->cells[current->cursor.insert] = (uint32_t) '\0';
		//next->cells[next->cursor.insert]       = (uint32_t) '\0';
	}
}
