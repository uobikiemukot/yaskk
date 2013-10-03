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
	//return 0; // for DEBUG
}

struct hash_t *hash_lookup(struct hash_t *symtable[], char *key, char *val)
{
	unsigned int hkey;
	struct hash_t *hp;
	struct hash_value_t *vp;

	hkey = hash_key((uint8_t *) key);
	for (hp = symtable[hkey]; hp != NULL; hp = hp->next) {
		if (strcmp((char *) key, (char *) hp->key) == 0) {
			for (vp = hp->values; vp != NULL; vp = vp->next) {
				if (strcmp((char *) val, (char *) vp->str) == 0) // matched: key/value found
					return hp;
			}
			break;
		}
	}

	return hp;
}

bool hash_create(struct hash_t *symtable[], char *key, char *val)
{
	unsigned int hkey;
	struct hash_t *hp;
	struct hash_value_t *vp, *vp_prev, *new_value;

	hkey = hash_key((uint8_t *) key);
	for (hp = symtable[hkey]; hp != NULL; hp = hp->next) {
		if (strcmp((char *) key, (char *) hp->key) == 0) {
			vp_prev = NULL;
			for (vp = hp->values; vp != NULL; vp = vp->next) {
				if (strcmp((char *) val, (char *) vp->str) == 0) { /* matched: key/value found */
					if (vp_prev != NULL) { /* sort */
						vp_prev->next = vp->next;
						vp->next = hp->values;
						hp->values = vp;
					}
					return false;
				}
				vp_prev = vp;
			}
			break;
		}
	}

	if (hp == NULL) { /* key/value not found */
		hp = (struct hash_t *) emalloc(sizeof(struct hash_t));

		hp->key = (char *) emalloc(strlen(key) + 1);
		new_value = (struct hash_value_t *) emalloc(sizeof(struct hash_value_t));
		new_value->str = (char *) emalloc(strlen(val) + 1);
		new_value->next = NULL;
		hp->values = new_value;

		safe_strncpy(hp->key, key, strlen(key) + 1);
		safe_strncpy(new_value->str, val, strlen(val) + 1);

		hp->next = symtable[hkey];
		symtable[hkey] = hp;
		return true;
	}
	else { /* key found but value not found */
		new_value = (struct hash_value_t *) emalloc(sizeof(struct hash_value_t));
		new_value->str = (char *) emalloc(strlen(val) + 1);
		safe_strncpy(new_value->str, val, strlen(val) + 1);

		vp_prev->next = new_value;
		new_value->next = NULL;

		return true;
	}
}

void hash_clear(struct hash_t *symtable[])
{
	int i;
	struct hash_t *hp, *hp_next;
	struct hash_value_t *vp, *vp_next;

	for (i = 0; i < MAX_ARGS; i++) {
		for (hp = symtable[i]; hp != NULL; hp = hp_next) {
			hp_next = hp->next;
			free(hp->key);
			for (vp = hp->values; vp != NULL; vp = vp_next) {
				vp_next = vp->next;
				free(vp->str);
				free(vp);
			}
			free(hp);
		}
		symtable[i] = NULL;
	}
}
