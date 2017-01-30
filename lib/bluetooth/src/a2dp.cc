extern "C"
{
	#include "utils.h"
	#include "rtp.h"
}

#include <v8.h>
#include <nan.h>
#include <sbc/sbc.h>

typedef struct {
	uint8_t channel_mode:4;
	uint8_t frequency:4;
	uint8_t allocation_method:2;
	uint8_t subbands:2;
	uint8_t block_length:4;
	uint8_t min_bitpool;
	uint8_t max_bitpool;
} __attribute__ ((packed)) a2dp_sbc_t;

/*
typedef struct sbc_info {
    sbc_t sbc;
    bool sbc_initialized;
    size_t codesize, frame_length;
    uint16_t seq_num;
    uint8_t min_bitpool;
    uint8_t max_bitpool;

    void* buffer;
    size_t buffer_size;
} sbc_info_t;
*/

namespace a2dp {

	using namespace node;
	using namespace v8;
	using namespace std;

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

	void sbc_set_bitpool(sbc_t *sbc, a2dp_sbc_t *sbc_config, uint8_t bitpool) {
		printf("!! ----- GOT HERE ----- sbc_set_bitpool() \n");
	    if (sbc->bitpool == bitpool)
	        return;

	    if (bitpool > sbc_config->max_bitpool)
	        bitpool = sbc_config->max_bitpool;
	    else if (bitpool < sbc_config->min_bitpool)
	        bitpool = sbc_config->min_bitpool;

	    sbc->bitpool = bitpool;
	}

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

	        //l = pa_read(u->stream_fd, sbc_info->buffer, sbc_info->buffer_size, &u->stream_write_type);
		}



	}

	void setup_stream(int fd, sbc_t *sbc, a2dp_sbc_t *sbc_config) {
		printf("!! ----- GOT HERE ----- setup_stream() \n");
		make_nonblocking(fd);
		set_priority(fd, 6);
		enable_timestamps(fd);
		sbc_set_bitpool(sbc, sbc_config, sbc_config->max_bitpool);

		while (true) {
			short pval = poll_func(fd, POLLIN, -1);
			if (pval == 0 || (pval & ~POLLIN)) {
				printf("Poll val of %d is not POLLIN, exiting...", pval);
				break;
			}

		}

	}

	void sbc_io(int fd, int mtu_read, int mtu_write, sbc_t *sbc, a2dp_sbc_t *sbc_config) {
		printf("!! ----- GOT HERE ----- sbc_io() \n");

		setup_stream(fd, sbc, sbc_config);

	}

	static void init(Local<Object> exports) {
		Nan::SetMethod(exports, "sbcConfigFromByteArray", SbcConfigFromByteArray);
	}

	NODE_MODULE(a2dp, init);

}
