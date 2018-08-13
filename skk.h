/* skk init/die functions */
bool skk_init(struct skk_t *skk)
{
	skk->mode = MODE_ASCII;

	skk->select     = NULL;
	skk->index      = 0;

	skk->append.ch = '\0';
	memset(skk->append.str, '\0', LINE_LEN_MAX);

	return true;
}

/* parse functions */
bool skk_preedit(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch)
{
	struct entry_t *entry;
	char preedit_str[LINE_LEN_MAX];

	/* add new character */
	if (ch != '\0')
		append_char(edit, (uint32_t) ch);

	/* there is no character in line buffer */
	if (preedit_len(edit) == 0)
		return false;

	/* get UTF-8 string from next linebuffer */
	if (get_preedit_chars(edit, preedit_str) <= 0)
		return false;
	logging(DEBUG, "skk_preedit() preedit_str:%s len:%d\n", preedit_str, preedit_len(edit));

	/* search word from dictionary */
	if ((entry = dict_search(dict, preedit_str, strlen(preedit_str))) != NULL) {
		if (VERBOSE) {
			logging(DEBUG, "entry:%s\n", entry->word);
			print_args(&entry->args);
		}

		if(strlen(entry->word) == strlen(preedit_str)) {
			logging(DEBUG, "completely matched!\n");
			remove_preedit_chars(edit);
			insert_preedit_chars(edit, (skk->mode == MODE_SQUARE) ? entry->args.v[1]: entry->args.v[0]);

			/* special case: double consonant (tt, kk, bb et al) -> U+3063(or U+30C3) + single consonant */
			if (strlen(preedit_str) > 1 && preedit_str[0] == preedit_str[1] && preedit_str[0] != 'n')
				append_char(edit, (uint32_t) preedit_str[0]);
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
			replace_char(edit, preedit_cursor(edit) + 1, 'n');
			skk_preedit(skk, dict, edit, '\0');

			/* restore single consonant */
			append_char(edit, preedit_str[1]);
		} else {
			logging(DEBUG, "unmatched! remove first preedit char\n");
			remove_char(edit, preedit_cursor(edit));
			skk_preedit(skk, dict, edit, '\0');
		}
	}
	return false;
}

void skk_select(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch)
{
	char word[LINE_LEN_MAX];
	struct entry_t *entry;

	if (skk->select == NULL) {
		/* get word for dict lookup */
		if (skk->append.ch != '\0') /* from MODE_COOK */
			snprintf(word, LINE_LEN_MAX, "%s%c", skk->restore, skk->append.ch);
		else                        /* from MODE_APPEND */
			snprintf(word, LINE_LEN_MAX, "%s", skk->restore);
		logging(DEBUG, "skk_select() word:%s\n", word);

		if ((entry = dict_search(dict, word, COMPARE_ALL_STRING)) != NULL) {
			skk->select = &entry->args;
			if (VERBOSE) {
				logging(DEBUG, "entry:%s\n", entry->word);
				print_args(skk->select);
			}

			logging(DEBUG, "candidate found!\n");
			skk->index = 0;
		}
	} else {
		if (ch == KEY_NEXT || ch == CTRL_N) {
			skk->index = (skk->index + 1) % skk->select->len;
		} else if (ch == KEY_PREV || ch == CTRL_P) {
			if (skk->index <= 0)
				skk->index = skk->select->len - 1;
			else
				skk->index--;
		}
	}

	if (skk->select != NULL) {
		/* remove cook str */
		if (line_len(edit) > 1)
			remove_chars(edit, 1, insert_cursor(edit) - 1);

		insert_chars(edit, 1, skk->select->v[skk->index]);
		if (skk->append.ch != '\0')
			append_chars(edit, skk->append.str);
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
void mode_ascii(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch);
void mode_cursive_square(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch);
void mode_cook(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch);
void mode_select(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch);
void mode_append(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch);

void (*mode_func[])(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch) = {
	[MODE_ASCII]   = mode_ascii,
	[MODE_CURSIVE] = mode_cursive_square,
	[MODE_SQUARE]  = mode_cursive_square,
	[MODE_COOK]    = mode_cook,
	[MODE_SELECT]  = mode_select,
	[MODE_APPEND]  = mode_append,
};

/* mode functions */
void mode_ascii(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch)
{
	(void) dict;

	/* check mode change */
	if (ch == KEY_CURSIVE) {
		change_mode(skk, MODE_CURSIVE);
		return;
	}

	/* check ctrl char */
	if (ch == BS) {
		send_backspace(edit->fd, 1);
		return;
	} else if (iscntrl(ch)) {
		ewrite(edit->fd, &ch, 1);
		return;
	}

	/* line update */
	append_char(edit, ch);
	edit->need_clear = true;
}

void mode_cook(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch)
{
	char restore_str[LINE_LEN_MAX];

	/* check mode change */
	if (ch == KEY_CURSIVE) {
		remove_preedit_chars(edit);
		remove_char(edit, 0);
		change_mode(skk, MODE_CURSIVE);
		edit->need_clear = true;
		return;
	} else if (ch == KEY_TOGGLE) {
		remove_preedit_chars(edit);
		toggle_cursive_square(edit);
		return;
	} else if (ch == KEY_SELECT) {
		remove_preedit_chars(edit);
		replace_char(edit, 0, MARK_SELECT);
		change_mode(skk, MODE_SELECT);

		/* store cook str */
		get_cook_chars(edit, restore_str);
		snprintf(skk->restore, LINE_LEN_MAX, "%s", restore_str);

		/* pass through to mode_select */
		mode_select(skk, dict, edit, ch);
		return;
	} else if (isupper(ch) && cook_len(edit) > 0) {
		remove_preedit_chars(edit);
		change_mode(skk, MODE_APPEND);

		/* store cook str */
		get_cook_chars(edit, restore_str);
		snprintf(skk->restore, LINE_LEN_MAX, "%s", restore_str);

		/* save append char/pos */
		skk->append.pos = insert_cursor(edit);
		skk->append.ch  = tolower(ch);

		/* pass through to mode_append */
		append_char(edit, MARK_APPEND);
		mode_append(skk, dict, edit, skk->append.ch);
		return;
	}

	/* check ctrl char */
	if (ch == BS) {
		if (preedit_len(edit) > 0 || cook_len(edit) > 0) {
			remove_last_char(edit);
		} else if (line_len(edit) == 1) { /* only cook mark */
			remove_all(edit);
			change_mode(skk, MODE_CURSIVE);
		}
		return;
	} else if (iscntrl(ch)) {
		/* ignore control char */
		return;
	}

	/* line update */
	skk_preedit(skk, dict, edit, ch);
}

void mode_cursive_square(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch)
{
	/* check mode change */
	if (get_char(edit, 0) != 'z' && ch == KEY_ASCII) {
		/* zl used for 'RIGHTWARDS ARROW' (U+2192), so not change mode */
		remove_preedit_chars(edit);
		change_mode(skk, MODE_ASCII);
		return;
	} else if (ch == KEY_TOGGLE) {
		remove_preedit_chars(edit);
		change_mode(skk, (skk->mode == MODE_CURSIVE) ? MODE_SQUARE: MODE_CURSIVE); /* toggle mode */
		return;
	} else if (isupper(ch)) {
		remove_preedit_chars(edit);
		change_mode(skk, MODE_COOK);

		/* pass through to mode_cook */
		insert_char(edit, 0, MARK_COOK);
		mode_cook(skk, dict, edit, tolower(ch));
		return;
	} else if (ch == ESC || ch == CTRL_G) {
		/* for vi user */
		remove_preedit_chars(edit);
		change_mode(skk, MODE_ASCII);
	}

	/* check ctrl char */
	if (ch == BS) {
		if (preedit_len(edit) <= 0)
			send_backspace(edit->fd, 1);
		else
			remove_last_char(edit);
		return;
	} else if (iscntrl(ch)) {
		return;
	}

	/* line update */
	skk_preedit(skk, dict, edit, ch);
	if (preedit_len(edit) == 0) /* cook str exist */
		edit->need_clear = true;
}

void mode_select(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch)
{
	/* check mode change */
	if (ch == ESC || ch == CTRL_G) {
		remove_all(edit);
		insert_char(edit, 0, MARK_COOK);
		insert_chars(edit, 1, skk->restore);
		clear_select_status(skk);
		change_mode(skk, MODE_COOK);
		return;
	} else if (ch == KEY_ASCII || ch == KEY_TOGGLE || ch == KEY_CURSIVE) {
		remove_char(edit, 0);
		sort_candidate(skk->select, skk->index);
		clear_select_status(skk);
		edit->need_clear = true;
		change_mode(skk,
			(ch == KEY_ASCII) ? MODE_ASCII:
			(ch == KEY_TOGGLE) ? MODE_SQUARE:
			MODE_CURSIVE);
		return;
	} else if (isgraph(ch) && (ch != KEY_NEXT && ch != KEY_PREV)) {
		remove_char(edit, 0);
		sort_candidate(skk->select, skk->index);
		clear_select_status(skk);
		edit->need_clear = true;
		line_update(edit);

		/* pass through to mode_cursive_square */
		change_mode(skk, MODE_CURSIVE);
		mode_cursive_square(skk, dict, edit, ch);
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
	skk_select(skk, dict, edit, ch);
}

void mode_append(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t ch)
{
	char append_str[LINE_LEN_MAX];

	/* check mode change */
	if (ch == KEY_CURSIVE) {
		change_mode(skk, MODE_CURSIVE);
		remove_chars(edit, skk->append.pos, insert_cursor(edit) - 1);
		remove_char(edit, 0);
		clear_select_status(skk);
		edit->need_clear = true;
		return;
	} else if (ch == ESC || ch == CTRL_G) {
		change_mode(skk, MODE_COOK);
		remove_chars(edit, skk->append.pos, insert_cursor(edit) - 1);
		replace_char(edit, 0, MARK_COOK);
		clear_select_status(skk);
		return;
	}

	/* check ctrl char */
	if (iscntrl(ch)) {
		/* ignore control char */
		return;
	}

	/* line update */
	if (skk_preedit(skk, dict, edit, ch)) {
		remove_preedit_chars(edit);
		change_mode(skk, MODE_SELECT);
		replace_char(edit, 0, MARK_SELECT);

		/* store append str */
		get_chars(edit, skk->append.pos + 1, insert_cursor(edit) - 1, append_str);
		snprintf(skk->append.str, LINE_LEN_MAX, "%s", append_str);

		/* pass through to mode_select */
		change_mode(skk, MODE_SELECT);
		mode_select(skk, dict, edit, SPACE);
	}
}

void parse(struct skk_t *skk, struct dict_t *dict, struct edit_t *edit, uint8_t *buf, ssize_t size)
{
	ssize_t i;
	uint8_t ch;

	for (i = 0; i < size; i++) {
		ch = buf[i];
		logging(DEBUG, "parsing... ch:0x%.2X current mode:%s\n", ch, mode2str[skk->mode]);

		/* if "ch" is not ascii (0x00 - 0x7F), pass through */
		if (ch > 0x7F) {
			logging(DEBUG, "not ascii character(0x%.2X), pass through\n", ch);
			ewrite(edit->fd, &ch, 1);
			continue;
		}

		/* check mode change*/

		/* mode related process */
		if (mode_func[skk->mode])
			mode_func[skk->mode](skk, dict, edit, ch);
	}

	if (VERBOSE)
		show_edit(edit);

	line_update(edit);
}
