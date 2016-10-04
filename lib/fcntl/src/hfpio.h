#include <v8.h>
#include <string>
#include <nan.h>
#include <sbc/sbc.h>

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

	struct pcm_args_t {
		int return_val;
		char *buf;
		int buf_len;
		snd_pcm_t *pcm;
	    uv_work_t request;
	    Nan::Persistent<Function> callback;
	};

	/*
	struct sbc_io_args_t {
		int written;
		int processed;
		sbc_t *sbc;
		char *in_buf;
		int in_buf_len;
		char *out_buf;
		int out_buf_len;
	    uv_work_t request;
	    Nan::Persistent<Function> callback;
	};
	*/
}
