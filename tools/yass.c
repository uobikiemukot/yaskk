#include "../common.h"
#include "../util.h"
#include "../list.h"
#include "../hash.h"
#include "../load.h"
#include "../misc.h"
#include "../parse.h"

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

void die(struct termios *save_tm)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, save_tm);
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
	uint8_t buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct skk_t skk;
	struct timeval tv;
	struct termios save_tm;

	/* init */
	init_skk(&skk);
	set_rawmode(STDIN_FILENO, &save_tm);

	/* main loop */
	while (1) {
		check_fds(&fds, &tv, STDIN_FILENO);
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > INPUT_LIMIT)
				write(skk.fd, buf, size);
			else if (size > 0)
				parse(&skk, buf, size);
		}
	}
	die(&save_tm);

	return EXIT_SUCCESS;
}
