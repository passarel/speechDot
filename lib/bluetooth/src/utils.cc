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

	NAN_METHOD(PollAsync) {
		poll_args_t *poll_args = new poll_args_t;
		poll_args->fd = info[0]->Int32Value();
		poll_args->events = (short) info[1]->Int32Value();
		poll_args->revents = (short) -1;
		poll_args->timeout = info[2]->Int32Value();
		poll_args->callback.Reset(info[3].As<Function>());
		poll_args->request.data = poll_args;
		uv_queue_work(uv_default_loop(), &poll_args->request, poll_async, poll_async_after);
	}

	void read_async(uv_work_t *req) {
		socket_io_args_t *socket_io_args = static_cast<socket_io_args_t *>(req->data);
		socket_io_args->return_val = read(socket_io_args->fd, socket_io_args->buf, socket_io_args->buf_len);
	}

	void read_async_after(uv_work_t *req, int status) {
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

	NAN_METHOD(ReadAsync) {
		socket_io_args_t *socket_io_args = new socket_io_args_t;
		socket_io_args->fd = info[0]->Int32Value();
		socket_io_args->buf = (char *) node::Buffer::Data(info[1].As<v8::Object>());
		socket_io_args->buf_len = info[2]->Int32Value();
		socket_io_args->callback.Reset(info[3].As<Function>());
		socket_io_args->request.data = socket_io_args;
		uv_queue_work(uv_default_loop(), &socket_io_args->request, read_async, read_async_after);
	}

	NAN_METHOD(ReadSync) {
		int fd = info[0]->Int32Value();
		unsigned char *buf = (unsigned char *) node::Buffer::Data(info[1].As<v8::Object>());
		int buf_len = info[2]->Int32Value();
	    info.GetReturnValue().Set(read(fd, buf, buf_len));
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

