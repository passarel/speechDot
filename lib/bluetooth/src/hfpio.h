extern "C"
{
	#include "utils.h"
}
#include <unistd.h>
#include <v8.h>
#include <nan.h>
#include <sbc/sbc.h>
#include "node_pointer.h"

namespace hfpio {

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

}

