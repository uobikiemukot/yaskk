bool check_ascii(struct skk_t *skk, uint8_t *c)
{
	if (*c == LF) {
		change_mode(skk, MODE_HIRA);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	return false;
}

bool check_dict_cook(struct skk_t *skk, uint8_t *c)
{
	if (*c == CR) {
		if (list_size(&skk->dict) > 0) {
			change_mode(skk, ~MODE_DICT);
			fix_candidate(skk, &skk->dict, list_size(&skk->dict));
			reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, &skk->dict, NULL});
			return true;
		}
		else {
			change_mode(skk, ~MODE_DICT);
			reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, &skk->dict, NULL});
			list_push_front_n(&skk->key, skk->stored_key, strlen(skk->stored_key));
			return true;
		}
	}
	else if (*c == ESC) {
		change_mode(skk, ~MODE_DICT);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, &skk->dict, NULL});
		list_push_front_n(&skk->key, skk->stored_key, strlen(skk->stored_key));
		return true;
	}
	else if (*c == SPACE) {
		change_mode(skk, MODE_SELECT);
		reset_buffer((struct list_t **[]){&skk->preedit, NULL});
	}
	else if (isupper(*c)) {
		*c = tolower(*c);
		if (list_size(&skk->key) > 0) {
			change_mode(skk, MODE_APPEND);
			if (list_size(&skk->preedit) > 0
				&& (*c == 'a' || *c == 'i' || *c == 'u' || *c == 'e' || *c == 'o'))
				list_push_back(&skk->key, list_front(&skk->preedit));
			else
				list_push_back(&skk->key, *c);
		}
	}
	return false;
}

bool check_dict_append(struct skk_t *skk, uint8_t *c)
{
	if (*c == LF) {
		change_mode(skk, MODE_COOK);
		list_copy(&skk->dict, &skk->key);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == ESC) {
		change_mode(skk, MODE_COOK);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (isupper(*c))
		*c = tolower(*c);

	return false;
}

bool check_dict_select(struct skk_t *skk, uint8_t *c)
{
	if (*c == DLE || *c == SO || *c == SPACE || *c == 'x')
		; /* through */
	if (*c == CR) {
	}
	else if (*c == LF) {
		reset_candidate(skk, list_size(&skk->key), true);
		change_mode(skk, MODE_COOK);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == ESC) {
		reset_candidate(skk, list_size(&skk->key), false);
		change_mode(skk, MODE_COOK);
		if (islower(list_back(&skk->key)))
			list_erase_back(&skk->key);
		reset_buffer((struct list_t **[]){&skk->append, &skk->preedit, NULL});
		return true;
	}
	else {
		reset_candidate(skk, list_size(&skk->key), true);
		change_mode(skk, MODE_COOK);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});

		if (isupper(*c)) {
			change_mode(skk, MODE_COOK);
			*c = tolower(*c);
		}
	}
	return false;
}

bool check_dict_kana(struct skk_t *skk, uint8_t *c)
{
	if ((list_back(&skk->preedit) == 'z') && (*c == SPACE || *c == 'L')) 
		; /* for "z " and "zL" */
	else if (*c == ESC) { /* for vi */
		change_mode(skk, MODE_ASCII);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
	}
	else if (*c == SPACE) {
		reset_buffer((struct list_t **[]){&skk->preedit, NULL});
		write_str(skk->fd, (char *) c, 1);
		return true;
	}
	else if (*c == 'l' && list_back(&skk->preedit) != 'z') {
		change_mode(skk, MODE_ASCII);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == 'q') {
		change_mode(skk, (skk->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (isupper(*c)) {
		change_mode(skk, MODE_COOK);
		*c = tolower(*c);
	}
	return false;
}

bool check_cook(struct skk_t *skk, uint8_t *c)
{
	if (*c == LF) {
		change_mode(skk, ~MODE_COOK);
		write_list(skk->fd, &skk->key, list_size(&skk->key));
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == 'L') {
		load_user(user_file, skk->user_dict);
		return true;
	}
	else if (*c == ESC) {
		change_mode(skk, ~MODE_COOK);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == SPACE) {
		change_mode(skk, MODE_SELECT);
		reset_buffer((struct list_t **[]){&skk->append, &skk->preedit, NULL});
	}
	else if (isupper(*c)) {
		*c = tolower(*c);
		if (list_size(&skk->key) > 0) {
			change_mode(skk, MODE_APPEND);
			if (list_size(&skk->preedit) > 0
				&& (*c == 'a' || *c == 'i' || *c == 'u' || *c == 'e' || *c == 'o'))
				list_push_back(&skk->key, list_front(&skk->preedit));
			else
				list_push_back(&skk->key, *c);
		}
	}
	return false;
}

bool check_append(struct skk_t *skk, uint8_t *c)
{
	if (*c == LF) {
		change_mode(skk, ~MODE_APPEND);
		list_erase_back(&skk->key);
		write_list(skk->fd, &skk->key, list_size(&skk->key));
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == ESC) {
		change_mode(skk, MODE_COOK);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (isupper(*c))
		*c = tolower(*c);
	return false;
}

bool check_select(struct skk_t *skk, uint8_t *c)
{
	if (*c == DLE || *c == SO || *c == SPACE || *c == 'x')
		; /* through */
	else if (*c == LF) {
		reset_candidate(skk, list_size(&skk->key), true);
		change_mode(skk, ~MODE_SELECT);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == ESC) {
		reset_candidate(skk, list_size(&skk->key), false);
		change_mode(skk, MODE_COOK);
		if (islower(list_back(&skk->key)))
			list_erase_back(&skk->key);
		reset_buffer((struct list_t **[]){&skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == 'l') {
		reset_candidate(skk, list_size(&skk->key), true);
		change_mode(skk, MODE_ASCII);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == 'q') {
		reset_candidate(skk, list_size(&skk->key), true);
		change_mode(skk, (skk->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else {
		reset_candidate(skk, list_size(&skk->key), true);
		change_mode(skk, ~MODE_SELECT);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL}); // key broken here

		if (isupper(*c)) {
			change_mode(skk, MODE_COOK);
			*c = tolower(*c);
		}
	}
	return false;
}

bool check_kana(struct skk_t *skk, uint8_t *c)
{
	if ((list_back(&skk->preedit) == 'z') && (*c == SPACE || *c == 'L')) 
		; /* for "z " and "zL" */
	else if (*c == ESC) { /* for vi */
		change_mode(skk, MODE_ASCII);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
	}
	else if (*c == SPACE) {
		reset_buffer((struct list_t **[]){&skk->preedit, NULL});
		write_str(skk->fd, (char *) c, 1);
		return true;
	}
	else if (*c == 'l' && list_back(&skk->preedit) != 'z') {
		change_mode(skk, MODE_ASCII);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (*c == 'q') {
		change_mode(skk, (skk->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA);
		reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
		return true;
	}
	else if (isupper(*c)) {
		change_mode(skk, MODE_COOK);
		*c = tolower(*c);
	}
	return false;
}

bool mode_check(struct skk_t *skk, uint8_t *c)
{
	if (skk->mode & MODE_ASCII)
		return check_ascii(skk, c);
	if ((skk->mode & MODE_DICT) && (skk->mode & MODE_COOK))
		return check_dict_cook(skk, c);
	else if ((skk->mode & MODE_DICT) && (skk->mode & MODE_APPEND))
		return check_dict_append(skk, c);
	else if ((skk->mode & MODE_DICT) && (skk->mode & MODE_SELECT))
		return check_dict_select(skk, c);
	else if ((skk->mode & MODE_DICT) && (skk->mode & MODE_HIRA || skk->mode & MODE_KATA))
		return check_dict_kana(skk, c);
	else if (skk->mode & MODE_COOK)
		return check_cook(skk, c);
	else if (skk->mode & MODE_APPEND)
		return check_append(skk, c);
	else if (skk->mode & MODE_SELECT)
		return check_select(skk, c);
	else if (skk->mode & MODE_HIRA || skk->mode & MODE_KATA)
		return check_kana(skk, c);
	else {
		if (DEBUG)
			fprintf(stderr, "\tunknown mode:0x%.8X\n", skk->mode);
	}

	return false;
}

void parse_control(struct skk_t *skk, uint8_t c)
{
	if (DEBUG)
		fprintf(stderr, "\tparse control c:0x%.2X\n", c);

	if (c == BS || c == DEL) /* for Mac OS X */
		delete_buffer(skk, c);
	else if (c == LF)
		; /* ignore */
	else if (skk->select >= SELECT_LOADED) {
		if (c == SO)
			increase_candidate(skk);
		else if (c == DLE)
			decrease_candidate(skk);
	}
	else
		write_str(skk->fd, (char *) &c, 1);
}

void parse_ascii(struct skk_t *skk, uint8_t c)
{
	write_str(skk->fd, (char *) &c, 1);
}

void parse_select(struct skk_t *skk, uint8_t c, int size)
{
	char str[size + 1];
	struct entry_t *ep;
	struct table_t *tp;

	list_getstr(&skk->key, str, size);

	if (DEBUG)
		fprintf(stderr, "\tparse select key:%s select:%d\n", str, skk->select);
	tp = (c == SPACE) ? &skk->okuri_nasi: &skk->okuri_ari;

	if (skk->select >= SELECT_LOADED) {
		if (c == SPACE)
			increase_candidate(skk);
		else if (c == 'x')
			decrease_candidate(skk);
	}
	else {
		reset_parm(&skk->parm);
		if ((ep = table_lookup(tp, str)) != NULL)
			get_candidate(skk, ep);
		sort_candidate(skk, str);

		if (skk->parm.argc > 0)
			skk->select = SELECT_LOADED;
		else {
			list_getstr(&skk->key, skk->stored_key, size);
			reset_buffer((struct list_t **[]){&skk->key, &skk->append, &skk->preedit, NULL});
			reset_candidate(skk, list_size(&skk->key), false);
			change_mode(skk, ~MODE_SELECT);
			change_mode(skk, MODE_DICT);
		}
	}
}

void parse_kana(struct skk_t *skk, int len)
{
	int size;
	char str[len + 1];
	bool is_double_consonant = false;
	struct triplet_t *tp;

	size = list_size(&skk->preedit);
	if (size < len || len <= 0) {
		if (DEBUG)
			fprintf(stderr, "\tstop! buffer size:%d compare length:%d\n", size, len);
		return;
	}

	list_getstr(&skk->preedit, str, len);

	if (DEBUG)
		fprintf(stderr, "\tparse kana compare length:%d list size:%d str:%s\n", len, size, str);

	if ((tp = map_lookup(&skk->rom2kana, str, KEYSIZE - 1)) != NULL) {
		if (DEBUG)
			fprintf(stderr, "\tmatched!\n");
		if (size >= 2 && (str[0] == str[1] && str[0] != 'n')) {
			is_double_consonant = true;
			list_push_front(&skk->preedit, str[0]);
		}

		if (write_buffer(skk, tp, is_double_consonant))
			parse_select(skk, NUL, list_size(&skk->key));

		list_erase_front_n(&skk->preedit, len);
	}
	else {
		if ((tp = map_lookup(&skk->rom2kana, str, len)) != NULL) {
			if (DEBUG)
				fprintf(stderr, "\tpartly matched!\n");
			parse_kana(skk, len + 1);
		}
		else {
			if (DEBUG)
				fprintf(stderr, "\tunmatched! ");
			if (str[0] == 'n') {
				if (DEBUG)
					fprintf(stderr, "add 'n'\n");
				list_push_front(&skk->preedit, 'n');
				parse_kana(skk, len);
			}
			else {
				if (DEBUG)
					fprintf(stderr, "erase front char\n");
				list_erase_front(&skk->preedit);
				parse_kana(skk, len - 1);
			}
		}
	}

}

void parse(struct skk_t *skk, uint8_t *buf, int size)
{
	int i;
	uint8_t c;

	if (DEBUG) {
		buf[size] = '\0';
		fprintf(stderr, "start parse buf:%s size:%d mode:%s\n", buf, size, mode_str[skk->mode]);
	}
	erase_buffer(skk);

	for (i = 0; i < size; i++) {
		c = buf[i];
		if (DEBUG)
			fprintf(stderr, "i:%d c:%c\n", i, c);

		if (mode_check(skk, &c))
			continue;

		if (c < SPACE || c >= DEL)
			parse_control(skk, c);
		else if (skk->mode & MODE_ASCII)
			parse_ascii(skk, c);
		else if (skk->mode & MODE_SELECT)
			parse_select(skk, c, list_size(&skk->key));
		else if (skk->mode & MODE_HIRA || skk->mode & MODE_KATA) {
			list_push_back(&skk->preedit, c);
			parse_kana(skk, 1);
		}
	}
	redraw_buffer(skk);
}
