/* load functions */
static inline int not_tab(int c)
{
	return (c != '\t') ? true: false;
}

bool add_entry(char *lbuf, int length, struct dict_entry_t *entry)
{
	char *cp;

	if (length == 0)
		return false;

	/* allocate line buffer */
	entry->lbuf = (char *) ecalloc(1, length + 1);
	strncpy(entry->lbuf, lbuf, length);

	/* format: keyword TAB candidate1 TAB candidate2 TAB candidate3... TAB */
	if ((cp = strchr(entry->lbuf, '\t')) == NULL)
		goto error;

	*cp = '\0';
	entry->keyword = entry->lbuf;

	parse_reset(&entry->candidate);
	parse_arg(cp + 1, &entry->candidate, '\t', not_tab);

	/* illegal entry */
	if (entry->candidate.argc <= 0)
		goto error;

	return true;
error:
	free(entry->lbuf);
	return false;
}

void load_file(const char *file, struct dict_entry_t **table, int *table_size, int *entry_count)
{
	char *cp, lbuf[BUFSIZE];
	FILE *fp;
	struct dict_entry_t entry;

	fp = efopen(file, "r");

	while (fgets(lbuf, BUFSIZE, fp) != NULL) {
		/* reallocate, if data is lager than INIT_ENTRY */
		if (*entry_count >= *table_size) {
			*table = (struct dict_entry_t *) realloc(*table, sizeof(struct dict_entry_t) * *table_size * 2);
			*table_size *= 2;
		}

		/* remove newline at eol */
		if ((cp = strchr(lbuf, '\n')) != NULL)
			*cp = '\0';

		if (add_entry(lbuf, strlen(lbuf), &entry)) {
			(*table)[*entry_count] = entry;
			*entry_count += 1;
		}
	}

	efclose(fp);
}

struct dict_entry_t *dict_load(const char *file, int *table_size, int *entry_count)
{
	struct dict_entry_t *table;

	/* at first, allocate INIT_ENTRY size */
	table		= (struct dict_entry_t *) ecalloc(INIT_ENTRY, sizeof(struct dict_entry_t));
	*table_size  = INIT_ENTRY;
	*entry_count = 0;

	/* load roma2kana/skkdict files */
	load_file(file, &table, table_size, entry_count);

	/*
	struct dict_entry_t *entry;
	logging(DEBUG, "table_size:%d entry count:%d\n", *table_size, *entry_count);
	for (int i = 0; i < *entry_count; i++) {
		entry = &table[i];
		logging(DEBUG, "i:%d keyword:%s\n", i, entry->keyword);
		for (int j = 0; j < entry->candidate.argc; j++)
			logging(DEBUG, "\tcandidate[%d]:%s\n", j, entry->candidate.argv[j]);
	}
	*/

	return table;
}

/* sort functions (merge sort) */
static inline void merge(struct dict_entry_t *table1, int size1, struct dict_entry_t *table2, int size2)
{
	int i, j, count;
	struct dict_entry_t *merged;

	merged = (struct dict_entry_t *) ecalloc(size1 + size2, sizeof(struct dict_entry_t));
	count = i = j = 0;

	while (i < size1 || j < size2) {
		if (j == size2)
			merged[count++] = table1[i++];
		else if (i == size1)
			merged[count++] = table2[j++];
		else if (strcmp(table1[i].keyword, table2[j].keyword) < 0)
			merged[count++] = table1[i++];
		else
			merged[count++] = table2[j++];
	}

	memcpy(table1, merged, sizeof(struct dict_entry_t) * (size1 + size2));
	free(merged);
}

void dict_sort(struct dict_entry_t *table, int size)
{
	struct dict_entry_t tmp;
	int median;

	if (size <= 1) {
		return;
	} else if (size == 2) {
		if (strcmp(table[0].keyword, table[1].keyword) > 0) {
			tmp	  = table[0];
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

struct dict_entry_t *dict_search(struct dict_t *dict, const char *keyword, int length)
{
	int ret;
	int lower, upper, median;

	assert(keyword);
	//logging(DEBUG, "dict_search() keyword:%s length:%d\n", keyword, length);

	lower = 0;
	upper = dict->entry_count - 1;

	while (lower <= upper) {
		median = (lower + upper) / 2;
		//logging(DEBUG, "lower:%u upeer:%u median:%u\n", lower, upper, median);

		assert(0 <= median && median < dict->table_size);
		assert(dict->table[median].keyword);

		ret = (length <= COMPARE_ALL_STRING) ? strcmp(keyword, dict->table[median].keyword):
			strncmp(keyword, dict->table[median].keyword, length);
		//logging(DEBUG, "strncmp() compare:%s ret:%d\n", dict->table[median].keyword, ret);

		if (ret == 0)
			return &dict->table[median];
		else if (ret < 0)
			upper = median - 1;
		else
			lower = median + 1;
	}
	return NULL;
}
