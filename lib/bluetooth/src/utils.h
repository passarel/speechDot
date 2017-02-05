#include <uv.h>
#include <poll.h>
#include <stdbool.h>
#include <unistd.h>
#include <v8.h>
#include <nan.h>
#include <sbc/sbc.h>
#include "node_pointer.h"

namespace utils {

	using namespace node;
	using namespace v8;
	using namespace std;

	struct poll_args_t {
		int fd;
		short events;
		short revents;
		int timeout;
	    uv_work_t request;
	    Nan::Persistent<Function> callback;
	};

	struct socket_io_args_t {
		int return_val;
		int fd;
		char *buf;
		int buf_len;
	    uv_work_t request;
	    Nan::Persistent<Function> callback;
	};

	struct sbc_args_t {
		char *in_buf;
		int in_buf_len;
		char *out_buf;
		int out_buf_len;
		sbc_t *sbc;
		uv_work_t request;
		Nan::Persistent<Function> callback;

		// optional, incase you want read and encode/decode in one go.
		int fd;
	};

	void print_errno(const char *method_name);

	void make_blocking(int fd);

	void make_nonblocking(int fd);

	void enable_socket(int fd);

	void set_priority(int fd, int priority);

	short poll_func(int fd, short events, int timeout);

	NAN_METHOD(PollAsync);

	NAN_METHOD(ReadAsync);

	NAN_METHOD(ReadSync);

	NAN_METHOD(CloseFd);

	NAN_METHOD(SbcFree);
}


