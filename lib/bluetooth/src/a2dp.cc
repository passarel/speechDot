#include "a2dp.h"

namespace a2dp {

	const int RTP_HEADER_SIZE = 13;

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
        printf("sbc_new() -> SBC parameters: allocation=%u, subbands=%u, blocks=%u, bitpool=%u, frequency=%u \n",
                    sbc->allocation, sbc->subbands, sbc->blocks, sbc->bitpool, sbc->frequency);
		return sbc;
	}

	void decode(sbc_args_t *sbc_args) {
		// move pointer over the rtp header bytes
		sbc_args->in_buf += RTP_HEADER_SIZE;
		sbc_args->in_buf_len -= RTP_HEADER_SIZE;
		size_t total_written = 0;
		for (int i=0; i<5; i++) {
	        size_t written;
	        ssize_t decoded;
	        decoded = sbc_decode(sbc_args->sbc,
								sbc_args->in_buf, sbc_args->in_buf_len,
								sbc_args->out_buf, sbc_args->out_buf_len,
								&written);
	        total_written += written;
	        sbc_args->in_buf += decoded;
	        sbc_args->in_buf_len -= decoded;
	        sbc_args->out_buf += written;
	        sbc_args->out_buf_len -= written;
		}
		sbc_args->out_buf -= total_written;
		sbc_args->out_buf_len = total_written;
	}

	void read_and_decode_async(uv_work_t *req) {
		sbc_args_t *sbc_args = static_cast<sbc_args_t *>(req->data);
		int bytes = read(sbc_args->fd, sbc_args->in_buf, sbc_args->in_buf_len);
		if (bytes > 0) {
			decode(sbc_args);
		} else {
			printf("a2dp: Failed to read MTU_SIZE bytes from socket");
		}
	}

	void read_and_decode_async_after(uv_work_t *req, int status) {
		sbc_args_t *sbc_args = static_cast<sbc_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Value> written = Nan::New<Int32>(sbc_args->out_buf_len);
		Local<Function> cb = Nan::New(sbc_args->callback);
		const unsigned argc = 1;
		Local<Value> argv[argc] = { written };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		sbc_args->callback.Reset();
		delete sbc_args;
	}

	NAN_METHOD(ReadAndDecode) {
		sbc_args_t *sbc_args = new sbc_args_t;
		sbc_args->fd = info[0]->Int32Value();
		sbc_args->sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[1]));
		sbc_args->in_buf = (char *) node::Buffer::Data(info[2].As<v8::Object>());
		sbc_args->in_buf_len = info[3]->Int32Value();
		sbc_args->out_buf = (char *) node::Buffer::Data(info[4].As<v8::Object>());
		sbc_args->out_buf_len = info[5]->Int32Value();
		sbc_args->callback.Reset(info[6].As<Function>());
		sbc_args->request.data = sbc_args;
		uv_queue_work(uv_default_loop(), &sbc_args->request, read_and_decode_async, read_and_decode_async_after);
	}

	NAN_METHOD(SbcNew) {
		a2dp_sbc_t *config = reinterpret_cast<a2dp_sbc_t *>(UnwrapPointer(info[0]));
		sbc_t *sbc = sbc_new(config);
		info.GetReturnValue().Set(WrapPointer(sbc).ToLocalChecked());
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
		Nan::SetMethod(exports, "readAndDecode", ReadAndDecode);
		Nan::SetMethod(exports, "poll", PollAsync);
	}

	NODE_MODULE(a2dp, init);

}
