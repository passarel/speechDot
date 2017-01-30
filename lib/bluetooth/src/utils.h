#include <uv.h>
#include <poll.h>
#include <unistd.h>
#include <stdbool.h>

void print_errno(const char *method_name);

void make_blocking(int fd);

void make_nonblocking(int fd);

void enable_socket(int fd);

void set_priority(int fd, int priority);

short poll_func(int fd, short events, int timeout);

void enable_timestamps(int fd);

