/* written by pedro martelletto in january 2014; public domain */

#include <sys/types.h>
#include <sys/event.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* filter flags */
struct fflag {
	unsigned int ff_val;
	const char *ff_name;
} vnode_fflags[] = {
	{ NOTE_DELETE,		"NOTE_DELETE" },
	{ NOTE_WRITE,		"NOTE_WRITE" },
	{ NOTE_EXTEND,		"NOTE_EXTEND" },
#ifdef __Bitrig__
	{ NOTE_TRUNCATE,	"NOTE_TRUNCATE" },
#endif
	{ NOTE_ATTRIB,		"NOTE_ATTRIB" },
	{ NOTE_LINK,		"NOTE_LINK" },
	{ NOTE_RENAME,		"NOTE_RENAME" },
	{ NOTE_REVOKE,		"NOTE_REVOKE" },
	{ 0,			NULL },
};

void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s <file> <ev1,ev2,...> <cmd>\n", __progname);
	exit(2);
}

unsigned int
construct_fflags(char *list, unsigned int mask)
{
	char *name;
	struct fflag *ffp;

	if (list == NULL)
		return mask;

	name = strsep(&list, ",");

	for (ffp = &vnode_fflags[0]; ffp->ff_val != 0; ffp++)
		if (!strcmp(name, ffp->ff_name)) {
			mask |= ffp->ff_val;
			return construct_fflags(list, mask);
		}

	errx(1, "unknown fflag %s", name);
}

int
open_file(const char *path)
{
	int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0)
		err(1, "open");
	return fd;
}

int
open_directory(const char *path)
{
	mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR);
	int fd = open(path, O_RDONLY | O_DIRECTORY);
	if (fd < 0)
		err(1, "open");
	return fd;
}

void
spawn_proc(char **argv)
{
	int status;

	pid_t pid = fork();
	if (pid < 0)
		err(1, "fork");
	if (pid == 0) {
		execvp(argv[3], &argv[3]);
		err(1, "execvp");
	}

	if (wait(&status) < 0)
		err(1, "wait");
	if (status)
		errx(1, "child failed");
}

void
prepare_event(struct kevent *kv, int fd, unsigned int fflags, void *cookie)
{
	/* avoid EV_SET() for clarity */
	kv->ident = fd;
	kv->filter = EVFILT_VNODE;
	kv->flags = EV_ADD | EV_ENABLE | EV_ONESHOT;
	kv->fflags = fflags;
	kv->data = 0;
	kv->udata = cookie;
}

void
verify_event(const struct kevent *kv, int fd, unsigned int fflags, void *cookie)
{
	if (kv->ident != (unsigned long)fd)
		errx(1, "ident mismatch (kv, fd) = (%lu, %d)", kv->ident, fd);
	if (kv->fflags != fflags)
		errx(1, "fflags mismatch (kv, fflags) = (0x%x, 0x%x)",
		    kv->fflags, fflags);
	if (kv->udata != cookie)
		errx(1, "udata mismatch (kv, cookie) = (%p, %p)", kv->udata,
		    cookie);
}

int
main(int argc, char **argv)
{
	int fd, kq, nevents;
	unsigned int fflags;
	struct timespec ts;
	struct kevent kv;
	void *cookie;

	if (argc < 4)
		usage();

	if (!strcmp(argv[1], "-d")) {
		fd = open_directory(argv[2]);
		argv++;
	} else 
		fd = open_file(argv[1]);

	fflags = construct_fflags(argv[2], 0);
	arc4random_buf(&cookie, sizeof(cookie));

	kq = kqueue();
	if (kq < 0)
		err(1, "kqueue");

	prepare_event(&kv, fd, fflags, cookie);

	nevents = kevent(kq, &kv, 1, NULL, 0, NULL);
	if (nevents < 0)
		err(1, "kevent");

	spawn_proc(argv);

	bzero(&kv, sizeof(kv));
	bzero(&ts, sizeof(ts));
	nevents = kevent(kq, NULL, 0, &kv, 1, &ts);
	if (nevents < 0)
		err(1, "kevent");
	if (nevents == 0)
		errx(3, "timeout");
	if (nevents != 1)
		errx(1, "kevent() = %d", nevents);

	verify_event(&kv, fd, fflags, cookie);

	exit(0);
}
