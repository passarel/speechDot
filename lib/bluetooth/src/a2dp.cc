#include "a2dp.h"

namespace a2dp {

	struct decode_args_t {
		char *in_buf;
		int in_buf_len;
		char *out_buf;
		int out_buf_len;
		sbc_t *sbc;
		uv_work_t request;
		Nan::Persistent<Function> callback;
	};

	sbc_t *sbc_new(a2dp_sbc_t *config) {
		sbc_t *sbc = new sbc_t;
		sbc_init(sbc, 0L);
        switch (config->frequency) {
            case SBC_SAMPLING_FREQ_16000:
                sbc->frequency = SBC_FREQ_16000;
                break;
            case SBC_SAMPLING_FREQ_32000:
                sbc->frequency = SBC_FREQ_32000;
                break;
            case SBC_SAMPLING_FREQ_44100:
                sbc->frequency = SBC_FREQ_44100;
                break;
            case SBC_SAMPLING_FREQ_48000:
                sbc->frequency = SBC_FREQ_48000;
                break;
            default:
                printf("[error] sbc_new() -> unsupported frequency val: %d", config->frequency);
        }
        switch (config->channel_mode) {
            case SBC_CHANNEL_MODE_MONO:
                sbc->mode = SBC_MODE_MONO;
                break;
            case SBC_CHANNEL_MODE_DUAL_CHANNEL:
                sbc->mode = SBC_MODE_DUAL_CHANNEL;
                break;
            case SBC_CHANNEL_MODE_STEREO:
                sbc->mode = SBC_MODE_STEREO;
                break;
            case SBC_CHANNEL_MODE_JOINT_STEREO:
                sbc->mode = SBC_MODE_JOINT_STEREO;
                break;
            default:
            	printf("[error] sbc_new() -> unsupported channel_mode val: %d", config->channel_mode);
        }
        switch (config->allocation_method) {
            case SBC_ALLOCATION_SNR:
                sbc->allocation = SBC_AM_SNR;
                break;
            case SBC_ALLOCATION_LOUDNESS:
                sbc->allocation = SBC_AM_LOUDNESS;
                break;
            default:
            	printf("[error] sbc_new() -> unsupported allocation_method val: %d", config->allocation_method);
        }
        switch (config->subbands) {
            case SBC_SUBBANDS_4:
                sbc->subbands = SBC_SB_4;
                break;
            case SBC_SUBBANDS_8:
                sbc->subbands = SBC_SB_8;
                break;
            default:
            	printf("[error] sbc_new() -> unsupported subbands val: %d", config->subbands);
        }
        switch (config->block_length) {
            case SBC_BLOCK_LENGTH_4:
                sbc->blocks = SBC_BLK_4;
                break;
            case SBC_BLOCK_LENGTH_8:
                sbc->blocks = SBC_BLK_8;
                break;
            case SBC_BLOCK_LENGTH_12:
                sbc->blocks = SBC_BLK_12;
                break;
            case SBC_BLOCK_LENGTH_16:
                sbc->blocks = SBC_BLK_16;
                break;
            default:
            	printf("[error] sbc_new() -> unsupported block_length val: %d", config->block_length);
        }
        sbc->bitpool = config->max_bitpool;
        printf("sbc_new() -> SBC parameters: allocation=%u, subbands=%u, blocks=%u, bitpool=%u",
                    sbc->allocation, sbc->subbands, sbc->blocks, sbc->bitpool);
		return sbc;
	}

	void decode_async(uv_work_t *req) {
		decode_args_t *decode_args = static_cast<decode_args_t *>(req->data);
		assert(decode_args->in_buf_len == 595);
		decode_args->out_buf_len = 2560;
		decode_args->out_buf = (char *) malloc(decode_args->out_buf_len);
		size_t total_written = 0;
		while (decode_args->in_buf_len > 0) {
	        size_t written;
	        ssize_t decoded;
	        decoded = sbc_decode(decode_args->sbc,
								decode_args->in_buf, decode_args->in_buf_len,
								decode_args->out_buf, decode_args->out_buf_len,
								&written);
	        total_written += written;
	        assert(decoded == 119);
	        assert(written == 512);
            decode_args->in_buf += decoded;
            decode_args->in_buf_len -= decoded;
            decode_args->out_buf += written;
            decode_args->in_buf_len -= written;
		}
		assert(total_written == 2560);
		// reset out_buf pointer to begining
		decode_args->out_buf -= total_written;
		decode_args->out_buf_len = 2560;
	}

	void decode_async_after(uv_work_t *req, int status) {
		decode_args_t *decode_args = static_cast<decode_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Function> cb = Nan::New(decode_args->callback);
		Local<Value> decodedData = Nan::NewBuffer(decode_args->out_buf, decode_args->out_buf_len).ToLocalChecked();
		const unsigned argc = 1;
		Local<Value> argv[argc] = { decodedData };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		decode_args->callback.Reset();
		delete decode_args;
	}

	NAN_METHOD(Decode) {
		decode_args_t *decode_args = new decode_args_t;
		decode_args->in_buf = (char *) node::Buffer::Data(info[0].As<v8::Object>());
		decode_args->in_buf_len = info[1]->Int32Value();
		decode_args->request.data = decode_args;
		uv_queue_work(uv_default_loop(), &decode_args->request, decode_async, decode_async_after);
	}

	NAN_METHOD(SbcNew) {
		a2dp_sbc_t *config = reinterpret_cast<a2dp_sbc_t *>(UnwrapPointer(info[0]));
		sbc_t *sbc = sbc_new(config);
		info.GetReturnValue().Set(WrapPointer(sbc).ToLocalChecked());
	}

	NAN_METHOD(SbcFree) {
		sbc_t *sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[0]));
		sbc_finish(sbc);
		delete sbc;
	}

	NAN_METHOD(SetupSocket) {
		int fd = info[0]->Int32Value();
		make_nonblocking(fd);
		set_priority(fd, 6);
	}

	static void init(Local<Object> exports) {
		Nan::SetMethod(exports, "sbcNew", SbcNew);
		Nan::SetMethod(exports, "sbcFree", SbcFree);
		Nan::SetMethod(exports, "setupSocket", SetupSocket);
		Nan::SetMethod(exports, "read", ReadAsync);
		Nan::SetMethod(exports, "decode", Decode);
	}

	NODE_MODULE(a2dp, init);

}
