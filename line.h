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
 * after line update, line buffer is immediately flushed (set need_clear flag)
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

/* fuctnios for line_t */
void init_line(struct line_t *line)
{
	memset(line->cells, '\0', sizeof(uint32_t) * MAX_CELLS);
	line->cursor.insert  = 0;
	line->cursor.preedit = 0;
}

void show_line(struct line_t *line)
{
	size_t size;
	char utf8[UTF8_LEN_MAX + 1]; // nul terminated

	logging(DEBUG, "cursor (insert: %d, preedit: %d)\n", line->cursor.insert, line->cursor.preedit);

	if (line->cursor.insert == 0) {
		logging(DEBUG, "| (null) |\n");
		return;
	}

	for (int i = 0; i < line->cursor.insert; i++) {
		if ((size = utf8_encode(line->cells[i], utf8)) > 0)
			logging(DEBUG, "| %s ", utf8);
	}
	logging(DEBUG, "|\n");
}

bool accessible_index(struct line_t *line, int index)
{
	/*
	void *trace[BUFSIZE];
	int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
	backtrace_symbols_fd(trace, n, STDERR_FILENO);
	*/

	if (0 <= index && index < line->cursor.insert)
		return true;

	logging(DEBUG, "accessible_index(): index out of range (index: %d, insert: %d) | ", index, line->cursor.insert);

	return false;
}

bool insertable_index(struct line_t *line, int index)
{
	if (0 <= index && index <= line->cursor.insert)
		return true;

	logging(DEBUG, "insertable_index(): index out of range (index: %d, insert: %d) | ", index, line->cursor.insert);
	return false;
}

int line_len(struct edit_t *edit)
{
	return edit->next.cursor.insert;
}

int preedit_len(struct edit_t *edit)
{
	return (edit->next.cursor.insert - edit->next.cursor.preedit);
}

int cook_len(struct edit_t *edit)
{
	return (edit->next.cursor.preedit - 1); /* ignore cook/select mark */
}

int preedit_cursor(struct edit_t *edit)
{
	return edit->next.cursor.preedit;
}

int insert_cursor(struct edit_t *edit)
{
	return edit->next.cursor.insert;
}

/* functions for cursor_t */
bool move_cursor(struct cursor_t *cursor, int offset, uint32_t ucs2)
{
	int preedit = cursor->preedit, insert = cursor->insert;

	/*
	 * if ucs is not ascii character, update preedit cursor too.
	 *     cook character must be multibyte UTF-8 sequence (non ascii character), so update preedit cursor.
	 */
	if (ucs2 > 0x7F || ucs2 == MARK_SELECT || ucs2 == MARK_COOK || ucs2 == MARK_APPEND)
		preedit += offset;

	/*
	 * always increment insert cursor
	 */
	insert += offset;

	if ((insert < 0 || insert >= MAX_CELLS) || (preedit < 0 || preedit >= MAX_CELLS) || preedit > insert) {
		logging(DEBUG, "move_cursor(): cursor out of range (preedit: %d, insert: %d, MAX_CELLS: %d) | ", preedit, insert, MAX_CELLS);
		return false;
	}

	cursor->preedit = preedit;
	cursor->insert  = insert;
	return true;
}

/* functions for edit_t */
void init_edit(struct edit_t *edit)
{
	init_line(&edit->current);
	init_line(&edit->next);
	edit->need_clear = false;
	edit->fd = -1;
}

void show_edit(struct edit_t *edit)
{
	logging(DEBUG, "current line:\n");
	show_line(&edit->current);
	logging(DEBUG, "next line:\n");
	show_line(&edit->next);
	logging(DEBUG, "need_clear: %s\n", edit->need_clear ? "true": "false");
}

bool remove_char(struct edit_t *edit, int index)
{
	uint32_t ucs2;
	struct line_t *line = &edit->next;

	if (!accessible_index(line, index)) {
		logging(DEBUG, "remove_char() | ");
		return false;
	}

	ucs2 = line->cells[index];

	/*
	 * if not last char, shrink cells
	 */
	if (index < (line->cursor.insert - 1))
		memmove(line->cells + index, line->cells + index + 1, sizeof(line->cells[0]) * (line->cursor.insert - index - 1));

	if (!move_cursor(&line->cursor, -1, ucs2)) {
		logging(DEBUG, "remove_char(): (ucs2: U+%.4X, offset: %d) | ", ucs2, -1);
		return false;
	}

	return true;
}

bool remove_last_char(struct edit_t *edit)
{
	struct line_t *line = &edit->next;
	return remove_char(edit, line->cursor.insert - 1);
}

int remove_chars(struct edit_t *edit, int from, int to)
{
	int count = 0, len = to - from + 1;
	struct line_t *line = &edit->next;

	if (!accessible_index(line, from) || !accessible_index(line, to)) {
		logging(DEBUG, "remove_chars(): (from: %d, to: %d) | ", from, to);
		return -1;
	}

	while (count < len) {
		if (!remove_char(edit, from)) {
			logging(DEBUG, "remove_chars(): (count: %d) | ", count);
			break;
		}
		count++;
	}
	return count;
}

int remove_preedit_chars(struct edit_t *edit)
{
	struct line_t *line = &edit->next;
	return remove_chars(edit, line->cursor.preedit, line->cursor.insert - 1);
}

void remove_all(struct edit_t *edit)
{
	edit->next.cursor = (struct cursor_t){ .preedit = 0, .insert = 0 };
}

/* insert functions */
bool insert_char(struct edit_t *edit, int index, uint32_t ucs)
{
	struct line_t *line = &edit->next;

	if (!insertable_index(line, index)) {
		logging(DEBUG, "insert_char() | ");
		return false;
	}

	/*
	 * not insert to tail, shift cells
	 */
	if (index < line->cursor.insert)
		memmove(line->cells + index + 1, line->cells + index, sizeof(line->cells[0]) * (line->cursor.insert - index));

	line->cells[index] = ucs;

	if (!move_cursor(&line->cursor, 1, ucs)) {
		logging(DEBUG, "insert_char(): (ucs: U+%.4X, offset: %d) | ", ucs, 1);
		return false;
	}

	return true;
}

bool append_char(struct edit_t *edit, uint32_t ucs)
{
	return insert_char(edit, edit->next.cursor.insert, ucs);
}

int insert_chars(struct edit_t *edit, int index, const char *utf8)
{
	int size, count = 0;
	uint32_t ucs2;
	const char *cp, *endp;

	if (utf8 == NULL) {
		logging(DEBUG, "insert_chars(): (utf8: NULL) | ");
		return -1;
	}

	cp   = utf8;
	endp = utf8 + strlen(utf8);

	while (*cp != '\0' && cp < endp) {
		if ((size = utf8_decode(cp, &ucs2)) < 0) {
			logging(DEBUG, "insert_chars(): (ucs2: U+%.4X) | ", ucs2);
			break;
		}
		cp  += size;

		if (!insert_char(edit, index + count, ucs2)) {
			logging(DEBUG, "insert_chars(): (index + count: %d + %d, ucs2: U+%.4X) | ", index, count, ucs2);
			break;
		}
		count++;
	}
	return count;
}

int append_chars(struct edit_t *edit, const char *utf8)
{
	return insert_chars(edit, edit->next.cursor.insert, utf8);
}

int insert_preedit_chars(struct edit_t *edit, char *utf8)
{
	struct line_t *line = &edit->next;
	return insert_chars(edit, line->cursor.preedit, utf8);
}

/* replace functions */
bool replace_char(struct edit_t *edit, int index, uint32_t ucs2)
{
	struct line_t *line = &edit->next;

	if (!accessible_index(line, index)) {
		logging(DEBUG, "replace_char() | ");
		return false;
	}

	line->cells[index] = ucs2;
	return true;
}

/* get functions */
uint32_t get_char(struct edit_t *edit, int index)
{
	struct line_t *line = &edit->next;

	if (!accessible_index(line, index)) {
		logging(DEBUG, "get_char() | ");
		return UTF8_MALFORMED_CHARACTER;
	}

	return line->cells[index];
}

int get_chars(struct edit_t *edit, int from, int to, char *utf8)
{
	struct line_t *line = &edit->next;
	char *cp;
	int size;

	if (utf8 == NULL) {
		logging(DEBUG, "get_chars(): (utf8: NULL) | ");
		return -1;
	}

	cp = utf8;
	for (int i = from; i <= to; i++) {
		if ((size = utf8_encode(line->cells[i], cp)) < 0) {
			logging(DEBUG, "get_chars(): (U+%.4X) | ", line->cells[i]);
			break;
		}
		cp += size;
	}

	return (cp - utf8);
}

int get_preedit_chars(struct edit_t *edit, char *utf8)
{
	struct line_t *line = &edit->next;
	return get_chars(edit, line->cursor.preedit, line->cursor.insert - 1, utf8);
}

int get_cook_chars(struct edit_t *edit, char *utf8)
{
	struct line_t *line = &edit->next;
	/* skip cook mark */
	return get_chars(edit, 1, line->cursor.insert - 1, utf8);
}

/* cursive/square conversion */
void toggle_cursive_square(struct edit_t *edit)
{
	struct line_t *line = &edit->next;

	for (int i = 0; i < line->cursor.insert; i++)
		line->cells[i] = convert_cursive_square(line->cells[i]);
}

/* update function */
void send_backspace(int fd, int count)
{
	extern const char *backspace; /* global */

	for (int i = 0; i < count; i++)
		ewrite(fd, backspace, strlen(backspace));
}

void line_update(struct edit_t *edit)
{
	int pos, diff;
	char utf8[UTF8_LEN_MAX + 1];
	size_t size;
	struct line_t *current = &edit->current, *next = &edit->next;

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
	 * if need_clear flag is set, remove all characters of current/next line
	 *
	 */

	/* step1 */
	for (pos = 0; pos < current->cursor.insert; pos++) {
		if (current->cells[pos] != next->cells[pos] || pos >= next->cursor.insert)
			break;
	}
	logging(DEBUG, "pos of first different character:%d\n", pos);

	/* step2 */
	diff = current->cursor.insert - pos;
	logging(DEBUG, "number of send backspace:%d\n", diff);
	if (diff > 0)
		send_backspace(edit->fd, diff);

	/* step3 */
	logging(DEBUG, "update current line from %d to %d\n", pos, next->cursor.insert - 1);
	for (; pos < next->cursor.insert; pos++) {
		current->cells[pos] = next->cells[pos]; /* uposdate current line buffer */
		if ((size = utf8_encode(next->cells[pos], utf8)) > 0)
			ewrite(edit->fd, utf8, size);
	}

	/* step4 */
	if (edit->need_clear) {
		logging(DEBUG, "do flush...\n");
		edit->need_clear = false;
		edit->current.cursor = edit->next.cursor = (struct cursor_t){ .preedit = 0, .insert = 0 };
	} else {
		current->cursor = next->cursor;
	}
}
