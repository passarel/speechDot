#include "utils.h"

namespace utils {

	void print_errno(const char *method_name) {
		fprintf(stdout, "Oh dear, something went wrong with %s()! %s\n", method_name, strerror(errno));
	}

	void set_priority(int fd, int priority) {
		if (setsockopt(fd, SOL_SOCKET, SO_PRIORITY, (const void *) &priority, sizeof(priority)) < 0)
			print_errno("setsockopt");
	}

	void make_nonblocking(int fd) {
		int flags = fcntl(fd, F_GETFL); // get flags
		if (fcntl(fd, F_SETFL, flags & O_NONBLOCK) < 0)
			print_errno("fcntl");
	}

	void enable_socket(int fd) {
		if (recv(fd, NULL, 0, 0)<  0)
			printf("enable_socket failed: %d (%s)\n", errno, strerror(errno));
	}

	short poll_func(int fd, short events, int timeout) {
		struct pollfd pfd[1];
		pfd[0].fd = fd;
		pfd[0].events = events;
		int status = poll(pfd, 1, timeout);
		if (status < 0)
			print_errno("poll");
		if (status <= 0)
			return (short) status;
		else
			return pfd[0].revents;
	}

	void poll_async(uv_work_t *req) {
		poll_args_t *poll_args = static_cast<poll_args_t *>(req->data);
		short status = poll_func(poll_args->fd, poll_args->events, poll_args->timeout);
		poll_args->revents = status;
	}

	void poll_async_after(uv_work_t *req, int status) {
		poll_args_t *poll_args = static_cast<poll_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Value> revents = Nan::New<Int32>((int) poll_args->revents);
		Local<Function> cb = Nan::New(poll_args->callback);
		const unsigned argc = 1;
		Local<Value> argv[argc] = { revents };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		poll_args->callback.Reset();
		delete poll_args;
	}

	NAN_METHOD(Poll) {
		poll_args_t *poll_args = new poll_args_t;
		poll_args->fd = info[0]->Int32Value();
		poll_args->events = (short) info[1]->Int32Value();
		poll_args->revents = (short) -1;
		poll_args->timeout = info[2]->Int32Value();
		poll_args->callback.Reset(info[3].As<Function>());
		poll_args->request.data = poll_args;
		uv_queue_work(uv_default_loop(), &poll_args->request, poll_async, poll_async_after);
	}

	void read_and_decode_async(uv_work_t *req) {
		sbc_args_t *sbc_args = static_cast<sbc_args_t *>(req->data);
		int bytes = read(sbc_args->fd, sbc_args->in_buf, sbc_args->in_buf_len);
		if (bytes > 0) {
			sbc_args->decode_func(sbc_args);
		} else {
			printf("Failed to read MTU_SIZE bytes from socket");
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

	void read_and_decode(const Nan::FunctionCallbackInfo<v8::Value>& info, void (*decode_func)(sbc_args_t *)) {
		sbc_args_t *sbc_args = new sbc_args_t;
		sbc_args->fd = info[0]->Int32Value();
		sbc_args->sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[1]));
		sbc_args->in_buf = (char *) node::Buffer::Data(info[2].As<v8::Object>());
		sbc_args->in_buf_len = info[3]->Int32Value();
		sbc_args->out_buf = (char *) node::Buffer::Data(info[4].As<v8::Object>());
		sbc_args->out_buf_len = info[5]->Int32Value();
		sbc_args->callback.Reset(info[6].As<Function>());
		sbc_args->decode_func = decode_func;
		sbc_args->request.data = sbc_args;
		uv_queue_work(uv_default_loop(), &sbc_args->request, read_and_decode_async, read_and_decode_async_after);
	}

	NAN_METHOD(CloseFd) {
		int fd = info[0]->Int32Value();
		if (shutdown(fd, SHUT_RDWR)<  0)
			printf("Failed to shutdown socket\n");
		if (close(fd)<  0)
			printf("Failed to close socket\n");
	}

	NAN_METHOD(SbcFree) {
		sbc_t *sbc = reinterpret_cast<sbc_t *>(UnwrapPointer(info[0]));
		sbc_finish(sbc);
		delete sbc;
	}
}

