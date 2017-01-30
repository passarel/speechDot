#include "utils.h"

void print_errno(const char *method_name) {
	fprintf(stdout, "Oh dear, something went wrong with %s()! %s\n", method_name, strerror(errno));
}

void make_blocking(int fd) {
	int flags = fcntl(fd, F_GETFL); // get flags
	if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0)
		print_errno("fcntl");
}

void enable_timestamps(int fd) {
	int one = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &one, sizeof(one)) < 0)
		print_errno("setsockopt");

}

void set_priority(int fd, int priority) {
    if (setsockopt(fd, SOL_SOCKET, SO_PRIORITY, (const void *) &priority, sizeof(priority)) < 0)
    	print_errno("setsockopt");
}

void make_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL); // get flags
	if (fcntl(fd, F_SETFL, flags & O_NONBLOCK) < 0)
		print_errno("fcntl");
}

void enable_socket(int fd) {
	if (recv(fd, NULL, 0, 0)<  0)
		printf("Defered setup failed: %d (%s)\n", errno, strerror(errno));
}

short poll_func(int fd, short events, int timeout) {
	struct pollfd pfd[1];
    pfd[0].fd = fd;
    pfd[0].events = events;
    int status = poll(pfd, 1, timeout);
	if (status < 0)
		print_errno("poll");
	if (status <= 0)
		return (short) status;
	else
		return pfd[0].revents;
}

struct timeval *rtclock_get(struct timeval *tv) {

	struct timespec ts;

#ifdef CLOCK_MONOTONIC
// No locking or atomic ops for no_monotonic here
static bool no_monotonic = false;

if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
	no_monotonic = true;

if (no_monotonic)
#endif // CLOCK_MONOTONIC
    assert(clock_gettime(CLOCK_REALTIME, &ts) == 0);
	assert(tv);

	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / ((unsigned long long) 1000ULL);

	return tv;
}

