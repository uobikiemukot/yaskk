/* load functions */
static inline int not_tab(int c)
{
	return (c != '\t') ? true: false;
}

bool add_entry(char *lbuf, int len, struct entry_t *entry)
{
	char *cp;
	struct args_t *args = &entry->args;

	if (len == 0)
		return false;

	/* allocate line buffer */
	args->buf = (char *) ecalloc(1, len + 1);
	strncpy(args->buf, lbuf, len);

	/* format: composition word TAB candidate1 TAB candidate2 TAB candidate3... */
	if ((cp = strchr(args->buf, '\t')) == NULL)
		goto error;

	*cp = '\0';
	entry->word = args->buf;

	reset_args(args);
	parse_args(cp + 1, args, '\t', not_tab);

	/* illegal entry */
	if (args->len <= 0)
		goto error;

	return true;

error:
	free(args->buf);
	return false;
}

bool load_file(const char *file, struct entry_t **entry, int *mem_size, int *size)
{
	char *cp, lbuf[BUFSIZE];
	FILE *fp;
	struct entry_t *ep, e;

	fp = efopen(file, "r");

	while (fgets(lbuf, BUFSIZE, fp) != NULL) {
		/* reallocate, if data is lager than INIT_ENTRY */
		if (*size >= *mem_size) {
			ep = (struct entry_t *) realloc(*entry, sizeof(struct entry_t) * *mem_size * 2);
			if (ep == NULL) {
				free(*entry);
				efclose(fp);
				return false;
			}
			*entry = ep;
			*mem_size *= 2;
		}

		/* remove newline at eol */
		if ((cp = strchr(lbuf, '\n')) != NULL)
			*cp = '\0';

		if (add_entry(lbuf, strlen(lbuf), &e)) {
			(*entry)[*size] = e;
			*size += 1;
		}
	}
	efclose(fp);

	return true;
}

void sort_candidate(struct args_t *args, int index)
{
	char *cp;

	/* already first candidate or index out of range */
	if (index == 0 || index >= args->len)
		return;

	/* sort */
	for (int i = index; i > 0; i--) {
		cp             = args->v[i - 1];
		args->v[i - 1] = args->v[i];
		args->v[i]     = cp;
	}
}

bool output_dict(struct dict_t *dict)
{
	int fd;
	FILE *fp;
	char tmp_str[] = "/tmp/yaskk_XXXXXX";
	struct entry_t *ep;

	if ((fd = mkstemp(tmp_str)) < 0) {
		logging(ERROR, "mkstemp(): failed\n");
		return false;
	}

	if ((fp = fdopen(fd, "w")) == NULL) {
		logging(ERROR, "fdopen(): failed\n");
		return false;
	}

	for (int i = 0; i < dict->size; i++) {
		logging(DEBUG, "i:%d\n", i);
		ep = &dict->entry[i];
		fprintf(fp, "%s", ep->word);
		for (int j = 0; j < ep->args.len; j++) {
			fprintf(fp, "\t%s", ep->args.v[j]);
			if (j == (ep->args.len - 1))
				fprintf(fp, "\n");
		}
	}

	fclose(fp);

	if (rename(tmp_str, skkdict_file) < 0) {
		logging(ERROR, "rename(): failed\n");
		return false;
	}

	return true;
}

struct entry_t *dict_load(const char *file, int *mem_size, int *size)
{
	struct entry_t *entry;

	/* at first, allocate INIT_ENTRY size */
	entry = (struct entry_t *) ecalloc(INIT_ENTRY, sizeof(struct entry_t));
	if (entry == NULL)
		return NULL;

	/* load roma2kana/skkdict files */
	*mem_size = INIT_ENTRY;
	*size     = 0;
	if (!load_file(file, &entry, mem_size, size))
		return NULL;

	/*
	struct entry_t *entry;
	logging(DEBUG, "mem_size:%d size:%d\n", *mem_size, *size);
	for (int i = 0; i < *size; i++) {
		entry = &entry[i];
		logging(DEBUG, "i:%d word:%s\n", i, entry->word);
		for (int j = 0; j < entry->args.len; j++)
			logging(DEBUG, "\targs.v[%d]:%s\n", j, entry->args.v[j]);
	}
	*/

	return entry;
}

void dict_die(struct dict_t *dict)
{
	for (int i = 0; i < dict->size; i++)
		free(dict->entry[i].args.buf);
	free(dict->entry);
}

/* sort functions (merge sort) */
static inline void merge(struct entry_t *table1, int size1, struct entry_t *table2, int size2)
{
	int i, j, count;
	struct entry_t *merged;

	merged = (struct entry_t *) ecalloc(size1 + size2, sizeof(struct entry_t));
	count = i = j = 0;

	while (i < size1 || j < size2) {
		if (j == size2)
			merged[count++] = table1[i++];
		else if (i == size1)
			merged[count++] = table2[j++];
		else if (strcmp(table1[i].word, table2[j].word) < 0)
			merged[count++] = table1[i++];
		else
			merged[count++] = table2[j++];
	}

	memcpy(table1, merged, sizeof(struct entry_t) * (size1 + size2));
	free(merged);
}

void dict_sort(struct entry_t *table, int size)
{
	struct entry_t tmp;
	int median;

	if (size <= 1) {
		return;
	} else if (size == 2) {
		if (strcmp(table[0].word, table[1].word) > 0) {
			tmp      = table[0];
			table[0] = table[1];
			table[1] = tmp;
		}
	} else {
		median = size / 2;
		dict_sort(table, median);
		dict_sort(table + median, size - median);
		merge(table, median, table + median, size - median);
	}
}

/* search functions (binary search)  */
enum {
	COMPARE_ALL_STRING = 0,
};

struct entry_t *dict_search(struct dict_t *dict, const char *word, int len)
{
	int ret;
	int lower, upper, median;

	assert(word);
	//logging(DEBUG, "dict_search() word:%s len:%d\n", word, len);

	lower = 0;
	upper = dict->size - 1;

	while (lower <= upper) {
		median = (lower + upper) / 2;
		//logging(DEBUG, "lower:%u upeer:%u median:%u\n", lower, upper, median);

		assert(0 <= median && median < dict->size);
		assert(dict->entry[median].word);

		ret = (len == COMPARE_ALL_STRING) ?
			strcmp(word, dict->entry[median].word):       /* compare whole string */
			strncmp(word, dict->entry[median].word, len); /* compare n-characters */
		//logging(DEBUG, "strncmp() compare:%s ret:%d\n", dict->entry[median].word, ret);

		if (ret == 0)
			return &dict->entry[median];
		else if (ret < 0)
			upper = median - 1;
		else
			lower = median + 1;
	}
	return NULL;
}
