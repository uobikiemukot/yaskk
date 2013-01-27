/* init */
void init_skk(struct skk_t *skk)
{
	skk->key = skk->preedit = skk->append = NULL;
	skk->pwrote = skk->kwrote = 0;
	skk->mode = MODE_ASCII;
	skk->select = SELECT_EMPTY;

	skk->okuri_ari.count = skk->okuri_nasi.count = 0;
	skk->okuri_ari.entries = skk->okuri_nasi.entries = NULL;
	skk->rom2kana.count = 0;
	skk->rom2kana.triplets = NULL;

	hash_init(skk->user_dict);

	load_map(map_file, &skk->rom2kana);
	load_dict(dict_file, &skk->okuri_ari, &skk->okuri_nasi);
	load_user(user_file, skk->user_dict);
}

/* misc */
int utf8_strlen(const uint8_t c[], int n)
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
	uint8_t code;

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

	if (size == 0)
		return 0;

	list_getstr(list, str, size);
	ewrite(fd, str, size);

	return utf8_strlen((uint8_t *) str, size);
}

int write_str(int fd, const char *str, int size)
{
	if (size == 0)
		return 0;

	ewrite(fd, str, size);
	return utf8_strlen((uint8_t *) str, size);
}

/* mode */
void change_mode(struct skk_t *skk, int mode)
{
	if (mode == MODE_ASCII || mode == MODE_HIRA || mode == MODE_KATA)
		skk->mode = mode;
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
	else if (mode == ~MODE_COOK || mode == ~MODE_SELECT || mode == ~MODE_APPEND)
		skk->mode &= mode;
	else {
		if (DEBUG)
			fprintf(stderr, "\tunknown mode change:0x%.4X\n", skk->mode);
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

	//if (DEBUG)
		//fprintf(stderr, "\tmap lookup key:%s len:%d\n", key, len);

	while (low <= high) {
		mid = (low + high) / 2;
		ret = strncmp((char *) key, (char *) mp->triplets[mid].key, len);

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

	//if (DEBUG)
		//fprintf(stderr, "\ttable lookup key:%s\n", key);

	while (low <= high) {
		mid = (low + high) / 2;
		ret = strcmp((char *) key, (char *) tp->entries[mid].key);

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
void increase_candidate(struct skk_t *skk)
{
	skk->select++;
	if (skk->select >= skk->parm.argc)
		skk->select = SELECT_LOADED; /* dictionary mode not implemented */
}

void decrease_candidate(struct skk_t *skk)
{
	skk->select--;
	if (skk->select < 0)
		skk->select = skk->parm.argc - 1;
}

bool get_candidate(struct skk_t *skk, struct entry_t *ep)
{
	char buf[BUFSIZE], key[BUFSIZE];
	FILE *fp;

	if ((fp = fopen(dict_file, "r")) == NULL
		|| fseek(fp, ep->offset, SEEK_SET) < 0
		|| fgets((char *) buf, BUFSIZE, fp) == NULL
		|| sscanf((char *) buf, "%s %s", key, skk->entry) != 2)
		return false;
	parse_entry(skk->entry, &skk->parm, '/', not_slash);

	if (DEBUG)
		fprintf(stderr, "\tcandidate num:%d\n", skk->parm.argc);

	fclose(fp);

	return (skk->parm.argc > 0) ? true: false;
}

void sort_candidate(struct skk_t *skk, char *key)
{
	int i, j;
	struct parm_t new;
	struct hash_t *hp;

	if (DEBUG)
		fprintf(stderr, "\tsort candidate key:%s\n", key);

	if ((hp = hash_lookup(skk->user_dict, key, "")) == NULL)
		return;

	if (DEBUG)
		fprintf(stderr, "\thash entry:%d table entry:%d\n", hp->count, skk->parm.argc);
	reset_parm(&new);
	//for (i = hp->count - 1; i >= 0; i--) {
	for (i = 0; i < hp->count; i++) {
		new.argv[new.argc++] = hp->values[i];
		for (j = 0; j < skk->parm.argc; j++) {
			if (strcmp((char *) hp->values[i], (char *) skk->parm.argv[j]) == 0)
				skk->parm.argv[j] = "";
		}
	}

	for (i = 0; i < skk->parm.argc; i++) {
		if (strcmp((char *) skk->parm.argv[i], "") != 0)
			new.argv[new.argc++] = skk->parm.argv[i];
	}

	skk->parm = new;
	if (DEBUG) {
		fprintf(stderr, "\tsort candidate: argc:%d\n", skk->parm.argc);
		for (i = 0; i < skk->parm.argc; i++)
			fprintf(stderr, "\t\targv[%d]:%s\n", i, skk->parm.argv[i]);
	}
}

void register_candidate(struct skk_t *skk, char *key, char *val)
{
	int fd;
	FILE *fp;
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = getpid();

	if ((fp = fopen(user_file, "a")) == NULL
		|| (fd = fileno(fp)) < 0
		|| fcntl(fd, F_SETLKW, &lock) < 0)
		return;

	if (hash_create(skk->user_dict, key, val))
		fprintf(fp, "%s /%s/\n", key, val);

	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);

	fclose(fp);
}

void reset_candidate(struct skk_t *skk, int size, bool fixed)
{
	int select = skk->select, argc = skk->parm.argc;
	char **argv = skk->parm.argv, str[size + 1];

	if (fixed && (0 <= select && select < argc)) {
		write_str(skk->fd, argv[select], strlen(argv[select]));
		write_list(skk->fd, &skk->append, list_size(&skk->append));
		register_candidate(skk, list_getstr(&skk->key, str, size), argv[select]);
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
		skk->kwrote += write_str(skk->fd, skk->parm.argv[skk->select], strlen(skk->parm.argv[skk->select]));
		skk->kwrote += write_list(skk->fd, &skk->append, list_size(&skk->append));
	}

	skk->pwrote = write_list(skk->fd, &skk->preedit, list_size(&skk->preedit));

	if (DEBUG)
		fprintf(stderr, "\trefresh preedit pwrote:%d kwrote:%d key:%d append:%d preedit:%d\n",
			skk->pwrote, skk->kwrote, list_size(&skk->key), list_size(&skk->append), list_size(&skk->preedit));
}

bool write_buffer(struct skk_t *skk, struct triplet_t *tp, bool is_double_consonant)
{
	int len;
	char *val;

	val = (skk->mode & MODE_HIRA || skk->mode & MODE_COOK || skk->mode & MODE_APPEND) ? tp->hira: tp->kata;
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
		fprintf(stderr, "\tdelete buffer key:%d append:%d preedit:%d\n",
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

void reset_buffer(struct list_t **list[])
{
	int i;

	for (i = 0; list[i] != NULL; i++)
		list_erase_front_n(list[i], list_size(list[i]));
}
