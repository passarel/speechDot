#include "hfpio.h"

namespace hfpio {

	const int MTU_SIZE = 48;

	char parser_buf[60];
	int parser_buf_len = 0;

	NAN_METHOD(ResetParserBuf) {
		parser_buf_len = 0;
	}

	int copy(char byte) {
		parser_buf[parser_buf_len] = byte;
		parser_buf_len++;
		return parser_buf_len;
	}

	int msbc_copy_machine(char byte) {
		assert(parser_buf_len < 60);
		switch (parser_buf_len) {
			case 0:
				if (byte == 0x01)
					return copy(byte);
				return 0;
			case 1:
				if (byte == 0x08 || byte == 0x38 || byte == 0xC8 || byte == 0xF8)
					return copy(byte);
				break;
			case 2:
				if (byte == 0xAD)
					return copy(byte);
				break;
			case 3:
				if (byte == 0x00)
					return copy(byte);
				break;
			case 4:
				if (byte == 0x00)
					return copy(byte);
				break;
			default:
				return copy(byte);
		}
		parser_buf_len = 0;
		return 0;
	}

	void decode(sbc_args_t *sbc_args) {
		assert(sbc_args->in_buf_len == MTU_SIZE);
		int total_written = 0;
		for (int i = 0; i < sbc_args->in_buf_len; i++) {
			if (msbc_copy_machine(sbc_args->in_buf[i]) == 60) {
				int written = 0;
				int decoded = 0;
				char *to_decode = (char *) parser_buf + 2;
				decoded = sbc_decode(sbc_args->sbc, to_decode, 57, sbc_args->out_buf, sbc_args->out_buf_len, (size_t *) &written);
				if (decoded > 0) {
					total_written += written;
				} else {
					printf("hfpio: Error while decoding, decoded -> %d \n", decoded);
				}
				parser_buf_len = 0;
			}
		}
		sbc_args->out_buf_len = total_written;
	}

	NAN_METHOD(ReadAndDecode) {
		read_and_decode(info, decode);
	}

	const int CAPTURE_SIZE = 240;
	char encoder_buf[CAPTURE_SIZE];
	int encoder_buff_len = 0;
	char sntable[4] = { 0x08, 0x38, 0xC8, 0xF8 };
	int sn = 0;

	void audio_encode(sbc_args_t *sbc_args) {
		int total_written = 0;
		for (int i = 0; i < sbc_args->in_buf_len; i++) {
			encoder_buf[encoder_buff_len] = sbc_args->in_buf[i];
			encoder_buff_len++;
			if (encoder_buff_len == CAPTURE_SIZE) {
				char *encode_out = sbc_args->out_buf;
				encode_out[0] = 0x01;
				encode_out[1] = sntable[sn];
				encode_out[59] = 0xff;
				sn = (sn + 1) % 4;
				int written = 0;
				sbc_encode(sbc_args->sbc, encoder_buf, CAPTURE_SIZE, encode_out+2, 57, (ssize_t *) &written);
				assert(written == 57);
				total_written += 60;
				sbc_args->out_buf += 60;
				encoder_buff_len = 0;
			}
		}
		sbc_args->out_buf -= total_written;
		sbc_args->out_buf_len = total_written;
	}

	void audio_encode_async(uv_work_t *req) {
		sbc_args_t *sbc_args = static_cast<sbc_args_t *>(req->data);
		audio_encode(sbc_args);
	}

	void audio_encode_async_after(uv_work_t *req, int status) {
		sbc_args_t *sbc_args = static_cast<sbc_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Value> data = Nan::NewBuffer(sbc_args->out_buf, sbc_args->out_buf_len).ToLocalChecked();
		Local<Function> cb = Nan::New(sbc_args->callback);
		const unsigned argc = 1;
		Local<Value> argv[argc] = { data };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		sbc_args->callback.Reset();
		delete sbc_args;
	}

	NAN_METHOD(AudioEncode) {
		sbc_args_t *sbc_args = new sbc_args_t;
		sbc_args->sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[0]));
		sbc_args->in_buf = (char *) node::Buffer::Data(info[1].As<v8::Object>());
		sbc_args->in_buf_len = info[2]->Int32Value();
		sbc_args->out_buf_len = ((encoder_buff_len + sbc_args->in_buf_len)/240)*60;
		sbc_args->out_buf = (char *) malloc(sbc_args->out_buf_len);
		sbc_args->callback.Reset(info[3].As<Function>());
		sbc_args->request.data = sbc_args;
		uv_queue_work(uv_default_loop(), &sbc_args->request, audio_encode_async, audio_encode_async_after);
	}

	void write_async(uv_work_t *req) {
		socket_io_args_t *socket_io_args = static_cast<socket_io_args_t *>(req->data);
		socket_io_args->return_val = write(socket_io_args->fd, socket_io_args->buf, socket_io_args->buf_len);
	}

	void write_async_after(uv_work_t *req, int status) {
		socket_io_args_t *socket_io_args = static_cast<socket_io_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Value> written = Nan::New<Int32>(socket_io_args->return_val);
		Local<Function> cb = Nan::New(socket_io_args->callback);
		const unsigned argc = 1;
		Local<Value> argv[argc] = { written };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		socket_io_args->callback.Reset();
		delete socket_io_args;
	}

	NAN_METHOD(WriteAsync) {
		socket_io_args_t *socket_io_args = new socket_io_args_t;
		socket_io_args->fd = info[0]->Int32Value();
		socket_io_args->buf = (char *) node::Buffer::Data(info[1].As<v8::Object>());
		socket_io_args->buf_len = info[2]->Int32Value();
		socket_io_args->callback.Reset(info[3].As<Function>());
		socket_io_args->request.data = socket_io_args;
		uv_queue_work(uv_default_loop(), &socket_io_args->request, write_async, write_async_after);
	}

	char write_buf[MTU_SIZE];
	int write_buf_len = 0;

	NAN_METHOD(ResetWriteBuf) {
		write_buf_len = 0;
	}

	void mtu_write_async(uv_work_t *req) {
		socket_io_args_t *socket_io_args = static_cast<socket_io_args_t *>(req->data);
		int total_written = 0;
		for (int j = 0; j < socket_io_args->buf_len; j++) {
			write_buf[write_buf_len] = socket_io_args->buf[j];
			write_buf_len++;
			if (write_buf_len == MTU_SIZE) {
				int written = write(socket_io_args->fd, write_buf, write_buf_len);
				total_written += written;
				write_buf_len = 0; // reset buf
			}
		}
		socket_io_args->return_val = total_written;
	}

	NAN_METHOD(MtuWriteAsync) {
		socket_io_args_t *socket_io_args = new socket_io_args_t;
		socket_io_args->fd = info[0]->Int32Value();
		socket_io_args->buf = (char *) node::Buffer::Data(info[1].As<v8::Object>());
		socket_io_args->buf_len = info[2]->Int32Value();
		socket_io_args->callback.Reset(info[3].As<Function>());
		socket_io_args->request.data = socket_io_args;
		uv_queue_work(uv_default_loop(), &socket_io_args->request, mtu_write_async, write_async_after);
	}

	void recvmsg_async(uv_work_t *req) {
		socket_io_args_t *socket_io_args = static_cast<socket_io_args_t *>(req->data);
		struct iovec iov;
		struct msghdr m;
		m.msg_iov = &iov;
		m.msg_iovlen = 1;
	    iov.iov_base = socket_io_args->buf;
	    iov.iov_len = socket_io_args->buf_len;
	    socket_io_args->return_val = recvmsg(socket_io_args->fd, &m, 0);
	}

	void recvmsg_async_after(uv_work_t *req, int status) {
		socket_io_args_t *socket_io_args = static_cast<socket_io_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Value> bytesRead = Nan::New<Int32>(socket_io_args->return_val);
		Local<Function> cb = Nan::New(socket_io_args->callback);
		const unsigned argc = 1;
		Local<Value> argv[argc] = { bytesRead };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		socket_io_args->callback.Reset();
		delete socket_io_args;
	}

	NAN_METHOD(RecvmsgAsync) {
		socket_io_args_t *socket_io_args = new socket_io_args_t;
		socket_io_args->fd = info[0]->Int32Value();
		socket_io_args->buf = (char *) node::Buffer::Data(info[1].As<v8::Object>());
		socket_io_args->buf_len = info[2]->Int32Value();
		socket_io_args->callback.Reset(info[3].As<Function>());
		socket_io_args->request.data = socket_io_args;
		uv_queue_work(uv_default_loop(), &socket_io_args->request, recvmsg_async, recvmsg_async_after);
	}

	NAN_METHOD(RecvmsgSync) {
		int fd = info[0]->Int32Value();
		struct iovec iov;
		struct msghdr m;
		m.msg_iov = &iov;
		m.msg_iovlen = 1;
		unsigned char *buf = (unsigned char *) node::Buffer::Data(info[1].As<v8::Object>());
		int buf_len = info[2]->Int32Value();
	    iov.iov_base = buf;
	    iov.iov_len = buf_len;
	    info.GetReturnValue().Set(recvmsg(fd, &m, 0));
	}

	NAN_METHOD(MsbcNew) {
		sbc_t *sbc = new sbc_t;
		sbc_init_msbc(sbc, 0L);
		info.GetReturnValue().Set(WrapPointer(sbc).ToLocalChecked());
	}

	NAN_METHOD(MsbcEncode) {
		sbc_t *sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[0]));
		unsigned char *input_buf = (unsigned char *) node::Buffer::Data(info[1].As<v8::Object>());
		int input_buf_len = info[2]->Int32Value();
		unsigned char *output_buf = (unsigned char *) node::Buffer::Data(info[3].As<v8::Object>());
		int output_buf_len = info[4]->Int32Value();
		int written;
		int encoded;
		encoded = sbc_encode(sbc, input_buf, input_buf_len, output_buf, output_buf_len, (ssize_t *) &written);
		v8::Local<v8::Object> obj = Nan::New<v8::Object>();
		obj->Set(Nan::New("written").ToLocalChecked(), Nan::New<v8::Integer>(written));
		obj->Set(Nan::New("encoded").ToLocalChecked(), Nan::New<v8::Integer>(encoded));
		info.GetReturnValue().Set(obj);
	}

	NAN_METHOD(SetupSocket) {
		int fd = info[0]->Int32Value();
		make_nonblocking(fd);
		set_priority(fd, 6);
		enable_socket(fd);
	}

	static void init(Local<Object> exports) {

		Nan::SetMethod(exports, "msbcNew", MsbcNew);
		Nan::SetMethod(exports, "msbcFree", SbcFree);

		Nan::SetMethod(exports, "setupSocket", SetupSocket);

		Nan::SetMethod(exports, "msbcEncode", MsbcEncode);

		Nan::SetMethod(exports, "mtuWrite", MtuWriteAsync);
		Nan::SetMethod(exports, "write", WriteAsync);
		Nan::SetMethod(exports, "resetWriteBuf", ResetWriteBuf);

		Nan::SetMethod(exports, "recvmsgSync", RecvmsgSync);
		Nan::SetMethod(exports, "recvmsg", RecvmsgAsync);

		Nan::SetMethod(exports, "poll", PollAsync);
		Nan::SetMethod(exports, "closeFd", CloseFd);

		Nan::SetMethod(exports, "readAndDecode", ReadAndDecode);

		Nan::SetMethod(exports, "resetParserBuf", ResetParserBuf);

		Nan::SetMethod(exports, "audioEncode", AudioEncode);
	}

	NODE_MODULE(hfpio, init);
}
