/* misc */
int utf8_strlen(const char c[], int n)
{
	int i, ret = 0;
	uint8_t code;

	for (i = 0; i < n; i++) {
		code = c[i];
		if (code <= 0x7F || (0xC2 <= code && code <= 0xFD)) 
			ret++;
	}

	return ret;
}

void utf8_delete_char(struct list_t **list)
{
	char code;

	while (*list != NULL) {
		code = list_back(list);
		list_erase_back(list);

		if (utf8_strlen(&code, 1) > 0)
			return;
	}
}

/* write */
int write_list(int fd, struct list_t **list, int size)
{
	char str[size + 1];

	memset(str, '\0', size + 1);
	list_front_n(list, str, size);
	ewrite(fd, str, size);

	return utf8_strlen(str, size);
}

int write_str(int fd, const char *str, int size)
{
	ewrite(fd, str, size);
	return utf8_strlen(str, size);
}

/* mode */
void change_mode(struct skk_t *skk, int mode)
{
	if (mode == MODE_ASCII || mode == MODE_HIRA || mode == MODE_KATA)
		skk->mode = mode;
	else if (mode == ~MODE_COOK || mode == ~MODE_SELECT || mode == ~MODE_APPEND)
		skk->mode &= mode;
	else if (mode == MODE_COOK) {
		skk->mode |= MODE_COOK;
		skk->mode &= ~MODE_SELECT;
		skk->mode &= ~MODE_APPEND;
	}
	else if (mode == MODE_SELECT) {
		skk->mode &= ~MODE_COOK;
		skk->mode |= MODE_SELECT;
		skk->mode &= ~MODE_APPEND;
	}
	else if (mode == MODE_APPEND) {
		skk->mode &= ~MODE_COOK;
		skk->mode &= ~MODE_SELECT;
		skk->mode |= MODE_APPEND;
	}

	if (DEBUG)
		fprintf(stderr, "\tmode changed:%s\n", mode_str[skk->mode]);
}

/* search */
struct triplet_t *map_lookup(struct map_t *mp, const char *key, int len)
{
	int low, mid, high, ret;

	low = 0;
	high = mp->count - 1;

	if (DEBUG)
		fprintf(stderr, "\tmap lookup key:%s len:%d\n", key, len);

	while (low <= high) {
		mid = (low + high) / 2;
		ret = strncmp(key, mp->triplets[mid].key, len);

		if (ret == 0)
			return &mp->triplets[mid];
		else if (ret < 0) /* s1 < s2 */
			high = mid - 1;
		else
			low = mid + 1;
	}

	return NULL;
}

struct entry_t *table_lookup(struct table_t *tp, const char *key)
{
	int low, mid, high, ret;

	low = 0;
	high = tp->count - 1;

	if (DEBUG)
		fprintf(stderr, "\ttable lookup key:%s\n", key);

	while (low <= high) {
		mid = (low + high) / 2;
		ret = strcmp(key, tp->entries[mid].key);

		if (ret == 0)
			return &tp->entries[mid];
		else if (ret < 0) /* s1 < s2 */
			high = mid - 1;
		else
			low = mid + 1;
	}

	return NULL;
}

/* candidate */
int not_slash(int c)
{
	if (c != 0x2F && !iscntrl(c))
		return 1; /* true */
	else
		return 0; /* false */
}

bool get_candidate(struct candidate_t *candidate, struct entry_t *ep)
{
	char buf[BUFSIZE], key[BUFSIZE];

	if (fseek(candidate->fp, ep->offset, SEEK_SET) < 0
		|| fgets(buf, BUFSIZE, candidate->fp) == NULL
		|| sscanf(buf, "%s %s", key, candidate->entry) != 2)
		return false;

	reset_parm(&candidate->parm);
	parse_entry(candidate->entry, &candidate->parm, '/', not_slash);

	return (candidate->parm.argc > 0) ? true: false;
}

void reset_candidate(struct skk_t *skk, bool fixed)
{
	int select = skk->select, count = skk->candidate.parm.argc;
	char **values = skk->candidate.parm.argv;

	if (fixed && (0 <= select && select < count)) {
		write_str(skk->fd, values[select], strlen(values[select]));
		write_list(skk->fd, &skk->append, list_size(&skk->append));
	}

	skk->select = SELECT_EMPTY;
}

/* buffer */
void erase_buffer(struct skk_t *skk)
{
	int i;

	for (i = 0; i < skk->pwrote; i++)
		write_str(skk->fd, backspace, 1);

	for (i = 0; i < skk->kwrote; i++)
		write_str(skk->fd, backspace, 1);

	skk->pwrote = skk->kwrote = 0;
}

void redraw_buffer(struct skk_t *skk)
{
	const char *str;

	if (skk->mode & MODE_COOK) {
		skk->kwrote = write_str(skk->fd, mark_cook, strlen(mark_cook));
		skk->kwrote += write_list(skk->fd, &skk->key, list_size(&skk->key));
	}
	else if (skk->mode & MODE_APPEND) {
		skk->kwrote = write_str(skk->fd, mark_cook, strlen(mark_cook));
		skk->kwrote += write_list(skk->fd, &skk->key, list_size(&skk->key) - 1);
		skk->kwrote += write_str(skk->fd, mark_append, strlen(mark_append));
		skk->kwrote += write_list(skk->fd, &skk->append, list_size(&skk->append));
	}
	else if (skk->mode & MODE_SELECT) {
		skk->kwrote = write_str(skk->fd, mark_select, strlen(mark_select));
		str = skk->candidate.parm.argv[skk->select];
		skk->kwrote += write_str(skk->fd, str, strlen(str));
		skk->kwrote += write_list(skk->fd, &skk->append, list_size(&skk->append));
	}

	skk->pwrote = write_list(skk->fd, &skk->preedit, list_size(&skk->preedit));

	if (DEBUG)
		fprintf(stderr, "\trefresh preedit pwrote:%d kwrote:%d\n", skk->pwrote, skk->kwrote);
}

bool write_buffer(struct skk_t *skk, struct triplet_t *tp, bool is_double_consonant)
{
	int len;
	char *val;

	val = (skk->mode & MODE_HIRA || skk->mode & MODE_COOK || skk->mode & MODE_APPEND) ?
		tp->hira: tp->kata;
	len = strlen(val);	

	if (skk->mode & MODE_COOK)
		list_push_back_n(&skk->key, val, len);
	else if (skk->mode & MODE_APPEND) {
		list_push_back_n(&skk->append, val, len);
		if (!is_double_consonant) {
			change_mode(skk, MODE_SELECT);
			return true;
		}
	}
	else
		write_str(skk->fd, val, len);

	return false;
}

void delete_buffer(struct skk_t *skk, char c)
{
	if (DEBUG)
		fprintf(stderr, "key:%d append:%d preedit:%d\n",
			list_size(&skk->key), list_size(&skk->append), list_size(&skk->preedit));
	
	if (list_size(&skk->preedit) > 0) {
		list_erase_back(&skk->preedit);
		if (skk->mode & MODE_APPEND && list_size(&skk->append) == 0) {
			change_mode(skk, MODE_COOK);
			list_erase_back(&skk->key);
		}
	}
	else if (list_size(&skk->append) > 0) {
		utf8_delete_char(&skk->append);
		if (list_size(&skk->append) == 0) {
			change_mode(skk, MODE_COOK);
			list_erase_back(&skk->key);
		}
	}
	else if (list_size(&skk->key) > 0)
		utf8_delete_char(&skk->key);
	else if (skk->mode & MODE_COOK)
		change_mode(skk, ~MODE_COOK);
	else
		write_str(skk->fd, &c, 1);
}

void reset_buffer(struct skk_t *skk)
{
	list_erase_front_n(&skk->key, list_size(&skk->key));
	list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
	list_erase_front_n(&skk->append, list_size(&skk->append));
}
