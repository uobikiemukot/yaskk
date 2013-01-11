#include "common.h"
#include "misc.h"
#include "parse.h"
#include "load.h"

int master;

void init(struct skk_t *skk)
{
	master = STDOUT_FILENO;
	skk->key = skk->preedit = skk->append = NULL;
	skk->pwrote = skk->kwrote = 0;
	skk->mode = MODE_ASCII;
	skk->select = SELECT_EMPTY;

	skk->okuri_ari.count = skk->okuri_nasi.count = 0;
	skk->okuri_ari.entries = skk->okuri_nasi.entries = NULL;
	skk->rom2kana.count = 0;
	skk->rom2kana.triplets = NULL;

	load_map(&skk->rom2kana);
	skk->candidate.fp = load_dict(&skk->okuri_ari, &skk->okuri_nasi);
}

int main(int argc, char *argv[])
{
	struct skk_t skk;
	char buf[BUFSIZE];
	ssize_t size;

	/* init */
	init(&skk);

	/* main loop */
	while ((size = read(STDIN_FILENO, buf, BUFSIZE)) > 0)
		parse(&skk, buf, size);

	return EXIT_SUCCESS;
}
