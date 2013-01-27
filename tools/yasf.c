#include "../common.h"
#include "../util.h"
#include "../list.h"
#include "../hash.h"
#include "../load.h"
#include "../misc.h"
#include "../parse.h"

int main(int argc, char *argv[])
{
	uint8_t buf[BUFSIZE];
	ssize_t size;
	struct skk_t skk;

	/* init */
	init_skk(&skk);

	/* main loop */
	while ((size = read(STDIN_FILENO, buf, BUFSIZE)) > 0)
		parse(&skk, buf, size);

	return EXIT_SUCCESS;
}
