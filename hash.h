void safe_strncpy(char *dst, const char *src, size_t n)
{
	strncpy((char *) dst, (char *) src, n);
	if (n > 0)
		dst[n - 1]= '\0';
}

void hash_init(struct hash_t *symtable[])
{
	int i;

	for (i = 0; i < MAX_ARGS; i++)
		symtable[i] = NULL;
}

unsigned int hash_key(uint8_t *cp)
{
	unsigned int ret = 0;

	while (*cp != '\0')
		ret = MULTIPLIER * ret + *cp++;

	return (ret % MAX_ARGS);
	//return 0;
}

struct hash_t *hash_lookup(struct hash_t *symtable[], char *key, char *val)
{
	int i;
	unsigned int hkey;
	struct hash_t *hp;

	hkey = hash_key((uint8_t *) key);
	for (hp = symtable[hkey]; hp != NULL; hp = hp->next) {
		if (strcmp((char *) key, (char *) hp->key) == 0) {
			for (i = 0; i < hp->count; i++) {
				if (strcmp((char *) val, (char *) hp->values[i]) == 0) /* matched */
					return hp;
			}
			break;
		}
	}

	return hp;
}

bool hash_create(struct hash_t *symtable[], char *key, char *val)
{
	int i;
	unsigned int hkey;
	struct hash_t *hp;

	hkey = hash_key((uint8_t *) key);
	for (hp = symtable[hkey]; hp != NULL; hp = hp->next) {
		if (strcmp((char *) key, (char *) hp->key) == 0) {
			for (i = 0; i < hp->count; i++) {
				if (strcmp((char *) val, (char *) hp->values[i]) == 0) /* matched */
					return false;
			}
			break;
		}
	}

	if (hp == NULL) {
		hp = (struct hash_t *) emalloc(sizeof(struct hash_t));
		hp->count = 0;

		hp->key = (char *) emalloc(strlen(key) + 1);
		hp->values[hp->count] = (char *) emalloc(strlen(val) + 1);

		safe_strncpy(hp->key, key, strlen(key) + 1);
		safe_strncpy(hp->values[hp->count], val, strlen(val) + 1);
		hp->count++;

		hp->next = symtable[hkey];
		symtable[hkey] = hp;
		return true;
	}
	else {
		hp->values[hp->count] = (char *) emalloc(strlen(val) + 1);
		safe_strncpy(hp->values[hp->count], val, strlen(val) + 1);
		hp->count++;

		return true;
	}
}
