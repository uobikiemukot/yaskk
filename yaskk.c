#include "common.h"
#include "misc.h"
#include "load.h"
#include "parse.h"

bool loop_flag = true;
bool window_resized = false;

void handler(int signo)
{
	extern bool loop_flag;

	if (signo == SIGCHLD)
		loop_flag = false;
	else if (signo == SIGWINCH)
		window_resized = true;
}

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
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handler;
	sigact.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGWINCH, &sigact, NULL);

	skk->key = skk->preedit = skk->append = NULL;
	skk->pwrote = skk->kwrote = 0;
	skk->mode = MODE_ASCII;
	skk->select = SELECT_EMPTY;

	skk->okuri_ari.count = skk->okuri_nasi.count = 0;
	skk->okuri_ari.entries = skk->okuri_nasi.entries = NULL;
	skk->rom2kana.count = 0;
	skk->rom2kana.triplets = NULL;

	hash_init(skk->user_dict);

	load_map(map_file, &skk->rom2kana);
	load_dict(dict_file, &skk->okuri_ari, &skk->okuri_nasi);
	load_user(user_file, skk->user_dict);
}

void die(struct termios *save_tm)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGWINCH, &sigact, NULL);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, save_tm);
}

void check_fds(fd_set *fds, struct timeval *tv, int stdin, int master)
{
	FD_ZERO(fds);
	FD_SET(stdin, fds);
	FD_SET(master, fds);
	tv->tv_sec = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(master + 1, fds, NULL, NULL, tv);
}

int main(int argc, char *argv[])
{
	char buf[BUFSIZE];
	char *cmd;
	ssize_t size;
	pid_t pid;
	fd_set fds;
	struct skk_t skk;
	struct timeval tv;
	struct winsize wsize;

	/* init */
	init(&skk);
	set_rawmode(STDIN_FILENO, &skk.save_tm);

	/* fork */
	cmd = (argc < 2) ? exec_cmd: argv[1];
	ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);

	pid = eforkpty(&skk.fd, NULL, &skk.save_tm, &wsize);
	if (pid == 0) /* child */
		eexecvp(cmd, (char *const []){cmd, NULL});

	/* main loop */
	while (loop_flag) {
		check_fds(&fds, &tv, STDIN_FILENO, skk.fd);
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > INPUT_LIMIT)
				write(skk.fd, buf, size);
			else if (size > 0)
				parse(&skk, buf, size);
		}
		if (FD_ISSET(skk.fd, &fds)) {
			size = read(skk.fd, buf, BUFSIZE);
			if (size > 0)
				write(STDOUT_FILENO, buf, size);
		}
		if (window_resized) {
			window_resized = false;
			ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
			ioctl(skk.fd, TIOCSWINSZ, &wsize);
		}
	}
	die(&skk.save_tm);

	return EXIT_SUCCESS;
}
