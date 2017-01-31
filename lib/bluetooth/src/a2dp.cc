#include "a2dp.h"

namespace a2dp {

	NAN_METHOD(SbcConfigFromByteArray) {
		unsigned char *buf = (unsigned char *) node::Buffer::Data(info[0].As<v8::Object>());
		a2dp_sbc_t *sbc_config;
		sbc_config = (a2dp_sbc_t *) buf;

		v8::Local<Object> obj = Nan::New<Object>();
		obj->Set(Nan::New("channelMode").ToLocalChecked(), Nan::New<Integer>(sbc_config->channel_mode));
		obj->Set(Nan::New("frequency").ToLocalChecked(), Nan::New<Integer>(sbc_config->frequency));
		obj->Set(Nan::New("allocationMethod").ToLocalChecked(), Nan::New<Integer>(sbc_config->allocation_method));
		obj->Set(Nan::New("subbands").ToLocalChecked(), Nan::New<Integer>(sbc_config->subbands));
		obj->Set(Nan::New("blockLength").ToLocalChecked(), Nan::New<Integer>(sbc_config->block_length));
		obj->Set(Nan::New("minBitpool").ToLocalChecked(), Nan::New<Integer>(sbc_config->min_bitpool));
		obj->Set(Nan::New("maxBitpool").ToLocalChecked(), Nan::New<Integer>(sbc_config->max_bitpool));
		info.GetReturnValue().Set(obj);
	}

	/*
	void process(int fd, sbc_t *sbc, a2dp_sbc_t *sbc_config, int read_mtu, int write_mtu) {
		// from a2dp_process_push
		for (;;) {
			bool found_tstamp = false;
			uint64_t tstamp;
	        struct rtp_header *header;
	        struct rtp_payload *payload;
	        const void *p;
	        void *d;
	        ssize_t l;
	        size_t to_write, to_decode;
	        size_t total_written = 0;
	        void* buffer;
	        size_t buffer_size;
	        if (read_mtu > write_mtu) buffer_size = read_mtu;
	        else buffer_size = write_mtu;
	        buffer = malloc(buffer_size);
	        header = (rtp_header*) buffer;
	        payload = (struct rtp_payload*) ((uint8_t*) buffer + sizeof(*header));
	        l = read(fd, buffer, buffer_size);
	        if (l < 0) {
	        	print_errno("read");
	        	break;
	        }
	        // TODO: get timestamp from rtp
	        if (!found_tstamp) {
	        	tstamp = rtclock_now();
	        }
		}
	}
	*/

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

	NAN_METHOD(SetupStream) {
		int fd = info[0]->Int32Value();
		make_nonblocking(fd);
		set_priority(fd, 6);
	}

	static void init(Local<Object> exports) {

		Nan::SetMethod(exports, "sbcConfigFromByteArray", SbcConfigFromByteArray);

		Nan::SetMethod(exports, "sbcNew", SbcNew);

		Nan::SetMethod(exports, "sbcFree", SbcFree);

		Nan::SetMethod(exports, "setupStream", SetupStream);
	}

	NODE_MODULE(a2dp, init);

}
