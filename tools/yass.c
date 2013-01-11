#include "../common.h"
#include "../misc.h"
#include "../parse.h"
#include "../load.h"

int master = -1; /* master of pseudo terminal */
struct termios save_tm;

void set_rawmode(int fd, struct termios *save_tm)
{
	struct termios tm;

	tcgetattr(fd, save_tm);
	tm = *save_tm;
	tm.c_iflag = tm.c_oflag = RESET;
	tm.c_cflag &= ~CSIZE;
	tm.c_cflag |= CS8;
	tm.c_lflag &= ~(ECHO | ISIG | ICANON);
	tm.c_cc[VMIN] = 1;  /* min data size (byte) */
	tm.c_cc[VTIME] = 0; /* time out */
	tcsetattr(fd, TCSAFLUSH, &tm);
}

void init(struct skk_t *skk)
{
	extern struct termios save_tm;

	set_rawmode(STDIN_FILENO, &save_tm);

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

void die()
{
	extern struct termios save_tm;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_tm);
}

void check_fds(fd_set *fds, struct timeval *tv, int stdin)
{
	FD_ZERO(fds);
	FD_SET(stdin, fds);
	tv->tv_sec = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(stdin + 1, fds, NULL, NULL, tv);
}

int main(int argc, char *argv[])
{
	extern int master;
	char buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct skk_t skk;

	/* init */
	if (atexit(die) != 0)
		fatal("atexit");
	init(&skk);

	/* main loop */
	while (1) {
		check_fds(&fds, &tv, STDIN_FILENO);
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > INPUT_LIMIT)
				write(master, buf, size);
			else if (size > 0)
				parse(&skk, buf, size);
		}
	}

	return EXIT_SUCCESS;
}
