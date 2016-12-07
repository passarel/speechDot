#include <v8.h>
#include <nan.h>
#include <alsa/asoundlib.h>
#include "node_pointer.h"

namespace alsa {

	using namespace node;
	using namespace v8;
	using namespace std;

	struct pcm_args_t {
		int return_val;
		char *buf;
		int buf_len;
		snd_pcm_t *pcm;
	    uv_work_t request;
	    Nan::Persistent<Function> callback;
	};

}


