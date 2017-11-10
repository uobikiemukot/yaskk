/* skk init/die functions */
void skk_init(struct skk_t *skk)
{
	skk->fd   = -1;
	skk->mode = MODE_ASCII;

	line_init(&skk->current);
	line_init(&skk->next);
	skk->need_flush  = false;

	skk->dict.table = dict_load(skkdict_file, &skk->dict.table_size, &skk->dict.entry_count);
	skk->select     = NULL;
	skk->index      = 0;

	skk->append.ch = '\0';
	memset(skk->append.str, '\0', LINE_LEN_MAX);
}

void skk_die(struct skk_t *skk)
{
	for (int i = 0; i < skk->dict.entry_count; i++)
		free(skk->dict.table[i].lbuf);
	free(skk->dict.table);
}

/* parse functions */
bool skk_preedit(struct skk_t *skk, uint8_t ch)
{
	struct dict_entry_t *entry;
	char preedit_str[LINE_LEN_MAX];

	/* add new character */
	if (ch != '\0')
		append_ucs_char(&skk->next, skk->next.cursor.insert, (uint32_t) ch);

	/* there is no character in line buffer */
	if (preedit_length(&skk->next) == 0)
		return false;

	/* get UTF-8 string from next linebuffer */
	if (get_utf8_str(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1, preedit_str) <= 0)
		return false;
	logging(DEBUG, "skk_preedit() preedit_str:%s length:%d\n", preedit_str, preedit_length(&skk->next));

	/* search keyword from dictionary */
	if ((entry = dict_search(&skk->dict, preedit_str, strlen(preedit_str))) != NULL) {
		if (VERBOSE) {
			logging(DEBUG, "entry:%s\n", entry->keyword);
			print_arg(&entry->candidate);
		}

		if(strlen(entry->keyword) == strlen(preedit_str)) {
			logging(DEBUG, "completely matched!\n");
			remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
			append_utf8_str(&skk->next, skk->next.cursor.preedit,
				(skk->mode == MODE_SQUARE) ? entry->candidate.argv[1]: entry->candidate.argv[0]);

			/* special case: double consonant (tt, kk, bb et al) -> U+3063(or U+30C3) + single consonant */
			if (strlen(preedit_str) > 1 && preedit_str[0] == preedit_str[1] && preedit_str[0] != 'n')
				append_ucs_char(&skk->next, skk->next.cursor.insert, preedit_str[0]);
			else
				return true;
		} else {
			logging(DEBUG, "partly matched!\n");
		}
	} else {
		/* special case: 'n' + single consonant (k, s, t et all) -> 'n' + 'n' + single consonant */
		if (strlen(preedit_str) == 2 && preedit_str[0] == 'n') {
			logging(DEBUG, "unmatched! but preedit_str[0] == 'n', append another 'n'\n");

			/* parse 'n' + 'n' firstly, remove single consonant  */
			swap_ucs_char(&skk->next, skk->next.cursor.preedit + 1, 'n');
			skk_preedit(skk, '\0');

			/* restore single consonant */
			append_ucs_char(&skk->next, skk->next.cursor.insert, preedit_str[1]);
		} else {
			logging(DEBUG, "unmatched! remove first preedit char\n");
			remove_ucs_char(&skk->next, skk->next.cursor.preedit);
			skk_preedit(skk, '\0');
		}
	}
	return false;
}

void skk_select(struct skk_t *skk, uint8_t ch)
{
	char keyword[LINE_LEN_MAX + 1];
	struct dict_entry_t *entry;

	if (skk->select == NULL) {
		/* get keyword for dict lookup */
		if (skk->append.ch != '\0') /* from MODE_COOK */
			snprintf(keyword, LINE_LEN_MAX, "%s%c", skk->restore, skk->append.ch);
		else                        /* from MODE_APPEND */
			snprintf(keyword, LINE_LEN_MAX, "%s", skk->restore);
		logging(DEBUG, "skk_select() keyword:%s\n", keyword);

		if ((entry = dict_search(&skk->dict, keyword, COMPARE_ALL_STRING)) != NULL) {
			skk->select = &entry->candidate;
			if (VERBOSE) {
				logging(DEBUG, "entry:%s\n", entry->keyword);
				print_arg(skk->select);
			}

			logging(DEBUG, "candidate found!\n");
			skk->index = 0;
		}
	} else {
		if (ch == KEY_NEXT || ch == CTRL_N) {
			skk->index = (skk->index + 1) % skk->select->argc;
		} else if (ch == KEY_PREV || ch == CTRL_P) {
			if (skk->index <= 0)
				skk->index = skk->select->argc - 1;
			else
				skk->index--;
		}
	}

	if (skk->select != NULL) {
		/* remove cook str */
		if (line_length(&skk->next) > 1)
			remove_chars(&skk->next, 1, skk->next.cursor.insert - 1);

		append_utf8_str(&skk->next, 1, skk->select->argv[skk->index]);
		if (skk->append.ch != '\0')
			append_utf8_str(&skk->next, skk->next.cursor.insert, skk->append.str);
	}
}

/* mode helper function */
void change_mode(struct skk_t *skk, enum skk_mode_t next)
{
	logging(DEBUG, "change_mode() current:%s next:%s\n", mode2str[skk->mode], mode2str[next]);
	skk->mode = next;
}

void clear_select_status(struct skk_t *skk)
{
	skk->select = NULL;
	skk->append.ch = '\0';
	memset(skk->append.str, '\0', LINE_LEN_MAX);
}

/* prototype */
void mode_ascii(struct skk_t *skk, uint8_t ch);
void mode_cursive_square(struct skk_t *skk, uint8_t ch);
void mode_cook(struct skk_t *skk, uint8_t ch);
void mode_select(struct skk_t *skk, uint8_t ch);
void mode_append(struct skk_t *skk, uint8_t ch);

void (*mode_func[])(struct skk_t *skk, uint8_t ch) = {
	[MODE_ASCII]   = mode_ascii,
	[MODE_CURSIVE] = mode_cursive_square,
	[MODE_SQUARE]  = mode_cursive_square,
	[MODE_COOK]    = mode_cook,
	[MODE_SELECT]  = mode_select,
	[MODE_APPEND]  = mode_append,
};

/* mode functions */
void mode_ascii(struct skk_t *skk, uint8_t ch)
{
	/* check mode change */
	if (ch == KEY_CURSIVE) {
		change_mode(skk, MODE_CURSIVE);
		return;
	}

	/* check ctrl char */
	if (ch == BS) {
		send_backspace(skk->fd, 1);
		return;
	} else if (iscntrl(ch)) {
		ewrite(skk->fd, &ch, 1);
		return;
	}

	/* line update */
	append_ucs_char(&skk->next, skk->next.cursor.insert, ch);
	skk->need_flush = true;
	//append_preedit_char(skk, ch);
}

void mode_cook(struct skk_t *skk, uint8_t ch)
{
	char restore_str[LINE_LEN_MAX];

	/* check mode change */
	if (ch == KEY_CURSIVE) {
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		remove_ucs_char(&skk->next, 0);
		change_mode(skk, MODE_CURSIVE);
		skk->need_flush = true;
		return;
	} else if (ch == KEY_TOGGLE) {
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		toggle_cursive_square(&skk->next);
		return;
	} else if (ch == KEY_SELECT) {
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		swap_ucs_char(&skk->next, 0, MARK_SELECT);
		change_mode(skk, MODE_SELECT);

		/* store cook str */
		get_utf8_str(&skk->next, 1, skk->next.cursor.insert - 1, restore_str);
		snprintf(skk->restore, LINE_LEN_MAX, "%s", restore_str);

		/* pass through to mode_select */
		mode_select(skk, ch);
		return;
	} else if (isupper(ch) && cook_length(&skk->next) > 0) {
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		change_mode(skk, MODE_APPEND);

		/* store cook str */
		get_utf8_str(&skk->next, 1, skk->next.cursor.insert - 1, restore_str);
		snprintf(skk->restore, LINE_LEN_MAX, "%s", restore_str);

		/* save append char/pos */
		skk->append.pos = skk->next.cursor.insert;
		skk->append.ch  = tolower(ch);

		/* pass through to mode_append */
		append_ucs_char(&skk->next, skk->next.cursor.insert, MARK_APPEND);
		mode_append(skk, skk->append.ch);
		return;
	}

	/* check ctrl char */
	if (ch == BS) {
		if (preedit_length(&skk->next) > 0 || cook_length(&skk->next) > 0) {
			remove_ucs_char(&skk->next, skk->next.cursor.insert - 1);
		} else if (line_length(&skk->next) == 1) { /* only cook mark */
			remove_all_chars(&skk->next);
			change_mode(skk, MODE_CURSIVE);
		}
		return;
	} else if (iscntrl(ch)) {
		/* ignore control char */
		return;
	}

	/* line update */
	skk_preedit(skk, ch);
}

void mode_cursive_square(struct skk_t *skk, uint8_t ch)
{
	/* check mode change */
	if (get_ucs_char(&skk->next, 0) != 'z' && ch == KEY_ASCII) {
		/* zl used for 'RIGHTWARDS ARROW' (U+2192), so not change mode */
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		change_mode(skk, MODE_ASCII);
		return;
	} else if (ch == KEY_TOGGLE) {
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		change_mode(skk, (skk->mode == MODE_CURSIVE) ? MODE_SQUARE: MODE_CURSIVE); /* toggle mode */
		return;
	} else if (isupper(ch)) {
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		change_mode(skk, MODE_COOK);

		/* pass through to mode_cook */
		append_ucs_char(&skk->next, 0, MARK_COOK);
		mode_cook(skk, tolower(ch));
		return;
	} else if (ch == ESC || ch == CTRL_G) {
		/* for vi user */
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		change_mode(skk, MODE_ASCII);
	}

	/* check ctrl char */
	if (ch == BS) {
		if (preedit_length(&skk->next) <= 0)
			send_backspace(skk->fd, 1);
		else
			remove_ucs_char(&skk->next, skk->next.cursor.insert - 1);
		return;
	} else if (ch == CR) {
		if (preedit_length(&skk->next) == 0)
			ewrite(skk->fd, &ch, 1);
	} else if (iscntrl(ch)) {
		//ewrite(skk->fd, &ch, 1);
		return;
	}

	/* line update */
	skk_preedit(skk, ch);
	if (preedit_length(&skk->next) == 0) /* cook str exist */
		skk->need_flush = true;
}

void mode_select(struct skk_t *skk, uint8_t ch)
{
	/* check mode change */
	if (ch == ESC || ch == CTRL_G) {
		remove_all_chars(&skk->next);
		append_ucs_char(&skk->next, 0, MARK_COOK);
		append_utf8_str(&skk->next, 1, skk->restore);
		clear_select_status(skk);
		change_mode(skk, MODE_COOK);
		return;
	} else if (ch == KEY_ASCII || ch == KEY_TOGGLE || ch == KEY_CURSIVE) {
		remove_ucs_char(&skk->next, 0);
		clear_select_status(skk);
		skk->need_flush = true;
		change_mode(skk, (ch == KEY_ASCII) ? MODE_ASCII:
			(ch == KEY_TOGGLE) ? MODE_SQUARE: MODE_CURSIVE);
		return;
	} else if (isgraph(ch) && (ch != KEY_NEXT && ch != KEY_PREV)) {
		remove_ucs_char(&skk->next, 0);
		clear_select_status(skk);
		skk->need_flush = true;
		line_update(&skk->current, &skk->next, &skk->need_flush, skk->fd);

		/* pass through to mode_cursive_square */
		change_mode(skk, MODE_CURSIVE);
		mode_cursive_square(skk, ch);
		return;
	}

	/* check ctrl char */
	if (ch == CTRL_N || ch == CTRL_P) {
		/* through */
	} else if (iscntrl(ch)) {
		/* ignore control char */
		return;
	}

	/* line update */
	skk_select(skk, ch);
}

void mode_append(struct skk_t *skk, uint8_t ch)
{
	char append_str[LINE_LEN_MAX];

	/* check mode change */
	if (ch == KEY_CURSIVE) {
		change_mode(skk, MODE_CURSIVE);
		remove_chars(&skk->next, skk->append.pos, skk->next.cursor.insert - 1);
		remove_ucs_char(&skk->next, 0);
		clear_select_status(skk);
		skk->need_flush = true;
		return;
	} else if (ch == ESC || ch == CTRL_G) {
		change_mode(skk, MODE_COOK);
		remove_chars(&skk->next, skk->append.pos, skk->next.cursor.insert - 1);
		swap_ucs_char(&skk->next, 0, MARK_COOK);
		clear_select_status(skk);
		return;
	}

	/* check ctrl char */
	if (iscntrl(ch)) {
		/* ignore control char */
		return;
	}

	/* line update */
	//logging(DEBUG, "mode_append()\n");
	//line_show(&skk->next);
	if (skk_preedit(skk, ch)) {
		remove_chars(&skk->next, skk->next.cursor.preedit, skk->next.cursor.insert - 1);
		change_mode(skk, MODE_SELECT);
		swap_ucs_char(&skk->next, 0, MARK_SELECT);

		/* store append str */
		get_utf8_str(&skk->next, skk->append.pos + 1, skk->next.cursor.insert - 1, append_str);
		snprintf(skk->append.str, LINE_LEN_MAX, "%s", append_str);
		//append_ucs_char(&skk->next, skk->next.cursor.insert, skk->append.ch);

		/* pass through to mode_select */
		change_mode(skk, MODE_SELECT);
		mode_select(skk, SPACE);
	}
}

void parse(struct skk_t *skk, uint8_t *buf, ssize_t size)
{
	ssize_t i;
	uint8_t ch;

	for (i = 0; i < size; i++) {
		ch = buf[i];
		logging(DEBUG, "parsing... ch:0x%.2X current mode:[%d]%s\n",
			ch, skk->mode, mode2str[skk->mode]);

		/* if "ch" is not ascii (0x00 - 0x7F), pass through */
		if (ch > 0x7F) {
			logging(DEBUG, "not ascii character(0x%.2X), pass through\n", ch);
			ewrite(skk->fd, &ch, 1);
			continue;
		}

		/* check mode change*/

		/* mode related process */
		if (mode_func[skk->mode])
			mode_func[skk->mode](skk, ch);

		/*
		do {
			if (mode_func[skk->mode])
				mode_func[skk->mode](skk, ch);
		} while (skk->mode_changed);
		*/

		/*
		if (skk->mode == MODE_ASCII)
			mode_ascii(skk, ch);
		if (skk->mode == MODE_COOK)
			mode_cook(skk, ch);
		if (skk->mode == MODE_SELECT)
			mode_select(skk, ch);
		if (skk->mode == MODE_APPEND)
			mode_append(skk, ch);
		if (skk->mode == MODE_CURSIVE || skk->mode == MODE_SQUARE)
			mode_cursive_square(skk, ch);
		*/
	}

	if (VERBOSE) {
		logging(DEBUG, "current line\n");
		line_show(&skk->current);
		logging(DEBUG, "next line\n");
		line_show(&skk->next);
	}

	line_update(&skk->current, &skk->next, &skk->need_flush, skk->fd);
}
