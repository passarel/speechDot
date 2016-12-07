#include "utils.h"

void print_errno(const char *method_name) {
	fprintf(stdout, "Oh dear, something went wrong with %s()! %s\n", method_name, strerror(errno));
}

void make_blocking(int fd) {
	int flags = fcntl(fd, F_GETFL); // get flags
	if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0)
		print_errno("fcntl");
}

void read_one_byte(int fd) {
    printf("enabling socket by reading 1 byte...\n");
    char c;
    if (read(fd, &c, 1) < 0)
    	print_errno("read");
    printf("socket is now enabled\n");
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

