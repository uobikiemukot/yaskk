bool mode_check(struct skk_t *skk, char *c)
{
	if (skk->mode & MODE_ASCII) {
		if (*c == LF) {
			change_mode(skk, MODE_HIRA);
			reset_buffer(skk);
			return true;
		}
	}
	else if (skk->mode & MODE_COOK) {
		if (*c == LF) {
			change_mode(skk, ~MODE_COOK);
			write_list(skk->fd, &skk->key, list_size(&skk->key));
			reset_buffer(skk);
			return true;
		}
		else if (*c == 'L') {
			load_user(user_file, skk->user_dict);
			return true;
		}
		else if (*c == ESC) {
			change_mode(skk, ~MODE_COOK);
			reset_buffer(skk);
			return true;
		}
		else if (*c == SPACE) {
			change_mode(skk, MODE_SELECT);
			list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
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
	}
	else if (skk->mode & MODE_APPEND) {
		if (*c == LF) {
			change_mode(skk, ~MODE_APPEND);
			list_erase_back(&skk->key);
			write_list(skk->fd, &skk->key, list_size(&skk->key));
			reset_buffer(skk);
			return true;
		}
		else if (*c == ESC) {
			change_mode(skk, MODE_COOK);
			list_erase_back(&skk->key);
			list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
			list_erase_front_n(&skk->append, list_size(&skk->append));
			return true;
		}
		else if (isupper(*c))
			*c = tolower(*c);
	}
	else if (skk->mode & MODE_SELECT) {
		if (*c == LF) {
			reset_candidate(skk, true);
			change_mode(skk, ~MODE_SELECT);
			reset_buffer(skk);
			return true;
		}
		else if (*c == ESC) {
			reset_candidate(skk, false);
			change_mode(skk, MODE_COOK);
			if (islower(list_back(&skk->key)))
				list_erase_back(&skk->key);
			list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
			list_erase_front_n(&skk->append, list_size(&skk->append));
			return true;
		}
		else if (*c == 'l') {
			reset_candidate(skk, true);
			change_mode(skk, MODE_ASCII);
			reset_buffer(skk);
			return true;
		}
		else if (*c == 'q') {
			reset_candidate(skk, true);
			change_mode(skk, (skk->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA);
			reset_buffer(skk);
			return true;
		}
		else if (*c != SPACE && *c != 'x') {
			reset_candidate(skk, true);
			change_mode(skk, ~MODE_SELECT);
			reset_buffer(skk);

			if (isupper(*c)) {
				change_mode(skk, MODE_COOK);
				*c = tolower(*c);
			}
		}
	}
	else if (skk->mode & MODE_HIRA || skk->mode & MODE_KATA) {
		if (*c == ESC) { /* for vi */
			change_mode(skk, MODE_ASCII);
			reset_buffer(skk);
		}
		else if (*c == SPACE && list_back(&skk->preedit) == 'z') /* for "z " */
			;
		else if (*c == SPACE) {
			list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
			write_str(skk->fd, c, 1);
			return true;
		}
		else if (*c == 'l' && list_back(&skk->preedit) != 'z') {
			change_mode(skk, MODE_ASCII);
			reset_buffer(skk);
			return true;
		}
		else if (*c == 'q') {
			change_mode(skk, (skk->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA);
			reset_buffer(skk);
			return true;
		}
		else if (*c == 'L' && list_back(&skk->preedit) == 'z') /* for "zL" */
			;
		else if (isupper(*c)) {
			change_mode(skk, MODE_COOK);
			*c = tolower(*c);
		}
	}
	return false;
}

void parse_control(struct skk_t *skk, char c)
{
	if (DEBUG)
		fprintf(stderr, "\tparse control c:0x%.2X\n", c);

	if (c == DEL) /* for Mac OS X */
		c = BS;

	if (c == BS)
		delete_buffer(skk, c);
	else if (c == LF)
		; /* ignore */
	else
		write_str(skk->fd, &c, 1);
}

void parse_ascii(struct skk_t *skk, char c)
{
	write_str(skk->fd, &c, 1);
}

void parse_select(struct skk_t *skk, char c, int size)
{
	struct entry_t *ep;
	struct table_t *tp;

	memset(skk->stored_key, '\0', BUFSIZE);
	list_front_n(&skk->key, skk->stored_key, size);

	if (DEBUG)
		fprintf(stderr, "\tparse select key:%s select:%d\n", skk->stored_key, skk->select);
	tp = (c == SPACE) ? &skk->okuri_nasi: &skk->okuri_ari;

	if (skk->select >= SELECT_LOADED) {
		if (c == SPACE) {
			skk->select++;
			if (skk->select >= skk->parm.argc)
				skk->select = SELECT_LOADED; /* dictionary mode: not implemented */
		}
		else if (c == 'x') {
			skk->select--;
			if (skk->select < 0) 
				skk->select = skk->parm.argc - 1;
		}
	}
	else {
		reset_parm(&skk->parm);
		if ((ep = table_lookup(tp, skk->stored_key)) != NULL)
			get_candidate(skk, ep);
		sort_candidate(skk, skk->stored_key);

		if (skk->parm.argc > 0)
			skk->select = SELECT_LOADED;
		else
			change_mode(skk, MODE_COOK);
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

	memset(str, '\0', len + 1);
	list_front_n(&skk->preedit, str, len);

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

void parse(struct skk_t *skk, char *buf, int size)
{
	int i;
	char c;

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
