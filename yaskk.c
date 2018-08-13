#include "yaskk.h"
#include "conf.h"
#include "util.h"
#include "dict.h"
#include "utf8.h"
#include "line.h"
#include "skk.h"

void sig_handler(int signo)
{
	/* global variable */
	extern volatile sig_atomic_t childi_is_alive, window_resized;
	errno = 0;

	if (signo == SIGCHLD) {
		if (wait(NULL) < 0)
			logging(ERROR, "wait: %s\n", strerror(errno));
		child_is_alive = false;
	} else if (signo == SIGWINCH) {
		window_resized = true;
	}
}

int set_rawmode(int fd, struct termios *current_tio)
{
	struct termios tio;

	tio = *current_tio;
	tio.c_iflag = tio.c_oflag = 0;
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8;
	tio.c_lflag &= ~(ECHO | ISIG | ICANON);
	tio.c_cc[VMIN]  = 1; /* min data size (byte) */
	tio.c_cc[VTIME] = 0; /* time out */

	return etcsetattr(fd, TCSAFLUSH, &tio);
}

bool tty_init(struct termios *tio)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;

	if (esigaction(SIGCHLD, &sigact, NULL) < 0
		|| esigaction(SIGWINCH, &sigact, NULL) < 0)
		return false;

	if (etcgetattr(STDIN_FILENO, tio) < 0)
		logging(WARN, "cannot get current termios information\n");

	if (set_rawmode(STDIN_FILENO, tio) < 0)
		return false;

	return true;
}

void tty_die(struct termios *tio)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	esigaction(SIGCHLD, &sigact, NULL);
	esigaction(SIGWINCH, &sigact, NULL);

	etcsetattr(STDIN_FILENO, TCSAFLUSH, tio);
	efflush(stdout);
}

bool fork_and_exec(int *master, const char *cmd, char *const argv[])
{
	pid_t pid;
	struct winsize wsize;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize) < 0) {
		logging(ERROR, "ioctl: TIOCWINSZ failed\n");
		return false;
	}

	pid = eforkpty(master, NULL, NULL, &wsize);
	if (pid < 0)
		return false;
	else if (pid == 0) /* child */
		eexecvp(cmd, argv);

	return true;
}

int check_fds(fd_set *fds, int fd, int master)
{
	struct timeval tv;

	FD_ZERO(fds);
	FD_SET(fd, fds);
	FD_SET(master, fds);
	tv.tv_sec  = 0;
	tv.tv_usec = SELECT_TIMEOUT;
	return eselect(master + 1, fds, NULL, NULL, &tv);
}

int main(int argc, char *const argv[])
{
	uint8_t buf[BUFSIZE];
	const char *cmd;
	ssize_t size;
	fd_set fds;
	struct winsize wsize;
	struct termios tio;
	struct skk_t skk;
	struct edit_t edit;
	struct dict_t dict;

	/* init */
	setlocale(LC_ALL, "");
	if (!tty_init(&tio))
		goto tty_init_failed;

	if (!skk_init(&skk))
		goto skk_init_failed;

	if ((dict.entry = dict_load(skkdict_file, &dict.mem_size, &dict.size)) == NULL)
		goto dict_load_failed;

	init_edit(&edit);

	/* fork */
	cmd = (argc < 2) ? shell_cmd: argv[1];
	if (!fork_and_exec(&edit.fd, cmd, argv + 1))
		goto fork_failed;

	/* main loop */
	while (child_is_alive) {
		if (check_fds(&fds, STDIN_FILENO, edit.fd) == -1)
			continue;

		/* data arrived from stdin */
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);

			if (size < INPUT_THRESHLD)
				parse(&skk, &dict, &edit, buf, size);
			else /* large data: maybe not manual input */
				ewrite(edit.fd, buf, size);
		}

		/* data arrived from pseudo terminal */
		if (FD_ISSET(edit.fd, &fds)) {
			size = read(edit.fd, buf, BUFSIZE);
			if (size > 0)
				ewrite(STDOUT_FILENO, buf, size);
		}

		/* if receive SIGWINCH, reach here */
		if (window_resized) {
			window_resized = false;
			ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
			ioctl(edit.fd, TIOCSWINSZ, &wsize);
		}
	}

	/* normal exit */
	output_dict(&dict);
	dict_die(&dict);
	tty_die(&tio);

	return EXIT_SUCCESS;

fork_failed:
	dict_die(&dict);
dict_load_failed:
skk_init_failed:
	tty_die(&tio);
tty_init_failed:

	return EXIT_FAILURE;
}
