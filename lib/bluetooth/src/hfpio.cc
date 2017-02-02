#include "hfpio.h"

namespace hfpio {

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

	NAN_METHOD(WriteSync) {
		int fd = info[0]->Int32Value();
		unsigned char *buf = (unsigned char *) node::Buffer::Data(info[1].As<v8::Object>());
		int buf_len = info[2]->Int32Value();
		int written = write(fd, buf, buf_len);
		info.GetReturnValue().Set(written);
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

	NAN_METHOD(MsbcFree) {
		sbc_t *sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[0]));
		sbc_finish(sbc);
		delete sbc;
	}

	NAN_METHOD(MsbcDecode) {
		sbc_t *sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[0]));
		unsigned char *input_buf = (unsigned char *) node::Buffer::Data(info[1].As<Object>());
		int input_buf_len = info[2]->Int32Value();
		unsigned char *output_buf = (unsigned char *) node::Buffer::Data(info[3].As<Object>());
		int output_buf_len = info[4]->Int32Value();
		int written;
		int decoded;
		decoded = sbc_decode(sbc, input_buf, input_buf_len, output_buf, output_buf_len, (size_t *) &written);
		v8::Local<Object> obj = Nan::New<Object>();
		obj->Set(Nan::New("written").ToLocalChecked(), Nan::New<Integer>(written));
		obj->Set(Nan::New("decoded").ToLocalChecked(), Nan::New<Integer>(decoded));
		info.GetReturnValue().Set(obj);
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
		Nan::SetMethod(exports, "msbcFree", MsbcFree);

		Nan::SetMethod(exports, "setupSocket", SetupSocket);

		Nan::SetMethod(exports, "msbcDecode", MsbcDecode);
		Nan::SetMethod(exports, "msbcEncode", MsbcEncode);

		Nan::SetMethod(exports, "write", WriteAsync);
		Nan::SetMethod(exports, "writeSync", WriteSync);

		Nan::SetMethod(exports, "read", ReadAsync);
		Nan::SetMethod(exports, "readSync", ReadSync);

		Nan::SetMethod(exports, "recvmsgSync", RecvmsgSync);
		Nan::SetMethod(exports, "recvmsg", RecvmsgAsync);

		Nan::SetMethod(exports, "poll", PollAsync);
		Nan::SetMethod(exports, "closeFd", CloseFd);
	}

	NODE_MODULE(hfpio, init);
}
