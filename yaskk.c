#include "yaskk.h"
#include "conf.h"
#include "util.h"
#include "dict.h"
#include "utf8.h"
#include "line.h"
#include "skk.h"

void sig_handler(int signo)
{
	if (signo == SIGCHLD) {
		wait(NULL);
		LoopFlag = false;
	} else if (signo == SIGWINCH) {
		WindowResized = true;
	}
}

void set_rawmode(int fd, struct termios *save_tm)
{
	struct termios tm;

	tm = *save_tm;
	tm.c_iflag = tm.c_oflag = 0;
	tm.c_cflag &= ~CSIZE;
	tm.c_cflag |= CS8;
	tm.c_lflag &= ~(ECHO | ISIG | ICANON);
	tm.c_cc[VMIN]  = 1; /* min data size (byte) */
	tm.c_cc[VTIME] = 0; /* time out */
	etcsetattr(fd, TCSAFLUSH, &tm);
}

void tty_init(struct termios *save_tm)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);
	esigaction(SIGWINCH, &sigact, NULL);

	etcgetattr(STDIN_FILENO, save_tm);
	set_rawmode(STDIN_FILENO, save_tm);
}

void tty_die(struct termios *save_tm)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGWINCH, &sigact, NULL);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, save_tm);
	fflush(stdout);
}

void fork_and_exec(int *master, const char *cmd, char *const argv[])
{
	pid_t pid;
	struct winsize wsize;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize) < 0)
		logging(ERROR, "ioctl: TIOCWINSZ failed\n");

	pid = eforkpty(master, NULL, NULL, &wsize);
	if (pid == 0) /* child */
		eexecvp(cmd, argv);
		//eexecvp(exec_cmd, (const char *[]){exec_cmd, NULL});
}

void check_fds(fd_set *fds, int stdin, int master)
{
	struct timeval tv;

	FD_ZERO(fds);
	FD_SET(stdin, fds);
	FD_SET(master, fds);
	tv.tv_sec  = 0;
	tv.tv_usec = SELECT_TIMEOUT;
	eselect(master + 1, fds, NULL, NULL, &tv);
}

int main(int argc, char *const argv[])
{
	uint8_t buf[BUFSIZE];
	const char *cmd;
	ssize_t size;
	fd_set fds;
	struct winsize wsize;
	struct termios save_tm;
	struct skk_t skk;

	/* init */
	setlocale(LC_ALL, "");
	tty_init(&save_tm);
	skk_init(&skk);

	/*
	int i;
	struct dict_entry_t *entry;
	for (i = 0; i < skk.dict.entry_count; i++) {
		entry = &skk.dict.table[i];
		if (dict_search(&skk.dict, entry->keyword, strlen(entry->keyword)) != NULL)
			fprintf(stderr, "keyword:%s found\n", entry->keyword);
		else
			fprintf(stderr, "keyword:%s not found\n", entry->keyword);
	}
	*/

	/* fork */
	//argv[0] = (argc < 2) ? shell_cmd: argv[1];
	cmd = (argc < 2) ? shell_cmd: argv[1];
	fork_and_exec(&skk.fd, cmd, argv + 1);

	/* main loop */
	while (LoopFlag) {
		check_fds(&fds, STDIN_FILENO, skk.fd);

		/* data arrived from stdin */
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);

			if (size < INPUT_THRESHLD)
				parse(&skk, buf, size);
			else /* large data: maybe not manual input */
				ewrite(skk.fd, buf, size);
		}

		/* data arrived from pseudo terminal */
		if (FD_ISSET(skk.fd, &fds)) {
			size = read(skk.fd, buf, BUFSIZE);
			if (size > 0)
				ewrite(STDOUT_FILENO, buf, size);
		}

		/* if receive SIGWINCH, reach here */
		if (WindowResized) {
			WindowResized = false;
			ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
			ioctl(skk.fd, TIOCSWINSZ, &wsize);
		}
	}

	/* normal exit */
	skk_die(&skk);
	tty_die(&save_tm);

	return EXIT_SUCCESS;
}
