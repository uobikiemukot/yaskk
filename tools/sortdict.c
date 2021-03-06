#include "../yaskk.h"
#include "../conf.h"
#include "../util.h"

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

/* dict load functions */
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

int main()
{
	char *cp, lbuf[BUFSIZE];
	struct dict_t dict;
	struct dict_entry_t entry, *ep;

	/* allocate memory */
	dict.table       = (struct dict_entry_t *) ecalloc(INIT_ENTRY, sizeof(struct dict_entry_t));
	dict.table_size  = INIT_ENTRY;
	dict.entry_count = 0;

	/* load */
	while (fgets(lbuf, BUFSIZE, stdin) != NULL) {
		/* reallocate, if data is lager than INIT_ENTRY */
		if (dict.entry_count >= dict.table_size) {
			dict.table = (struct dict_entry_t *) realloc(dict.table, sizeof(struct dict_entry_t) * dict.table_size * 2);
			dict.table_size *= 2;
		}

		/* remove newline at eol */
		if ((cp = strchr(lbuf, '\n')) != NULL)
			*cp = '\0';

		/* skip comment */
		if (lbuf[0] == '#' && lbuf[1] != '\t')
			continue;
		
		if (add_entry(lbuf, strlen(lbuf), &entry)) {
			dict.table[dict.entry_count] = entry;
			dict.entry_count += 1;
		}
	}

	/* sort */
	dict_sort(dict.table, dict.entry_count);

	/* dump */
	for (int i = 0; i < dict.entry_count; i++) {
		ep = &dict.table[i];
		printf("%s", ep->keyword);
		for (int j = 0; j < ep->candidate.argc; j++)
			printf("\t%s", ep->candidate.argv[j]);
		printf("\n");
	}

	/* release */
	for (int i = 0; i < dict.entry_count; i++)
		free(dict.table[i].lbuf);
	free(dict.table);

	return EXIT_SUCCESS;
}
