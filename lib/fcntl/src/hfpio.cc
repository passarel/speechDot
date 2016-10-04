#include <v8.h>
#include <node.h>
#include <uv.h>
#include <stdlib.h>
#include <nan.h>
#include <alsa/asoundlib.h>
#include "node_pointer.h"
#include "hfpio.h"

namespace hfpio {

	void print_errno(const char *method_name) {
		fprintf(stdout, "Oh dear, something went wrong with %s()! %s\n", method_name, strerror(errno));
	}

	void read_one_byte(int fd) {
	    printf("enabling socket by reading 1 byte...\n");
	    char c;
	    if (read(fd, &c, 1) < 0)
	    	print_errno("read");
	    printf("socket is now enabled\n");
	}

	void enable_socket(int fd) {
		if (recv(fd, NULL, 0, 0)<  0)
			printf("Defered setup failed: %d (%s)\n", errno, strerror(errno));
	}

	snd_pcm_t *pcm_init(snd_pcm_stream_t stream, int rate, bool isBlocking) {
		snd_pcm_t *pcm;
		printf("Initializing pcm for %s\n", (stream == SND_PCM_STREAM_CAPTURE) ? "capture" : "playback");
		if (snd_pcm_open(&pcm, "default", stream, isBlocking ? 0 : SND_PCM_NONBLOCK)<  0) { //SND_PCM_NONBLOCK
			printf("Failed to open pcm\n");
			return NULL;
		}
		if (snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, rate, 1, 120000) <  0) {
			printf("Failed to set pcm params\n");
			snd_pcm_close(pcm);
			pcm = NULL;
		}
		snd_pcm_prepare(pcm);
		return pcm;
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

	NAN_METHOD(OpenMic) {
		int rate = info[0]->Int32Value();
		bool isBlocking = info[1]->BooleanValue();
		snd_pcm_t *pcm = pcm_init(SND_PCM_STREAM_CAPTURE, rate, isBlocking);
		info.GetReturnValue().Set(WrapPointer(pcm).ToLocalChecked());
	}

	NAN_METHOD(CloseMic) {
		snd_pcm_t *pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		snd_pcm_drain(pcm);
		snd_pcm_close(pcm);
	}

	NAN_METHOD(OpenSpeaker) {
		int rate = info[0]->Int32Value();
		bool isBlocking = info[1]->BooleanValue();
		snd_pcm_t *pcm = pcm_init(SND_PCM_STREAM_PLAYBACK, rate, isBlocking);
		info.GetReturnValue().Set(WrapPointer(pcm).ToLocalChecked());
	}

	NAN_METHOD(CloseSpeaker) {
		snd_pcm_t *pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		snd_pcm_drain(pcm);
		snd_pcm_close(pcm);
	}

	NAN_METHOD(SndPcmAvailUpdate) {
		snd_pcm_t *pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		int frames = snd_pcm_avail_update(pcm);
        if (frames < 0)
        	frames = snd_pcm_recover(pcm, frames, 0);
        if (frames < 0)
                printf("SndPcmAvailUpdate failed: %s, code: %d \n", snd_strerror(frames), frames);
		info.GetReturnValue().Set(frames);
	}

	NAN_METHOD(SndPcmStart) {
		snd_pcm_t *pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		int status = snd_pcm_start(pcm);
		if (status < 0)
			print_errno("snd_pcm_start");
		info.GetReturnValue().Set(status);
	}

	NAN_METHOD(SndPcmPrepare) {
		snd_pcm_t *pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		int status = snd_pcm_prepare(pcm);
		if (status < 0)
			print_errno("snd_pcm_prepare");
		info.GetReturnValue().Set(status);
	}

	NAN_METHOD(SndPcmWait) {
		snd_pcm_t *pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		int status = snd_pcm_wait(pcm, 1000);
		if (status < 0)
			print_errno("snd_pcm_wait");
		info.GetReturnValue().Set(status);
	}

	void read_from_mic(uv_work_t *req) {
		pcm_args_t *pcm_args = static_cast<pcm_args_t *>(req->data);
		int frames = (int) snd_pcm_readi(pcm_args->pcm, pcm_args->buf, pcm_args->buf_len);
        if (frames < 0)
        	frames = snd_pcm_recover(pcm_args->pcm, frames, 0);
        if (frames < 0)
                printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
		/*
		if (frames == -EPIPE) {
			snd_pcm_prepare(pcm_args->pcm);
		}
		*/
		pcm_args->return_val = frames;
	}

	void read_from_mic_after(uv_work_t *req, int status) {
		pcm_args_t *pcm_args = static_cast<pcm_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Value> frames = Nan::New<Int32>(pcm_args->return_val);
		Local<Function> cb = Nan::New(pcm_args->callback);
		const unsigned argc = 1;
		Local<Value> argv[argc] = { frames };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		pcm_args->callback.Reset();
		delete pcm_args;
	}

	NAN_METHOD(ReadFromMic) {
		pcm_args_t *pcm_args = new pcm_args_t;
		pcm_args->pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		pcm_args->buf = (char *) node::Buffer::Data(info[1].As<v8::Object>());
		pcm_args->buf_len = info[2]->Int32Value();
		pcm_args->callback.Reset(info[3].As<Function>());
		pcm_args->request.data = pcm_args;
		uv_queue_work(uv_default_loop(), &pcm_args->request, read_from_mic, read_from_mic_after);
	}

	void write_to_speaker(uv_work_t *req) {
		pcm_args_t *pcm_args = static_cast<pcm_args_t *>(req->data);
		int frames = (int) snd_pcm_writei(pcm_args->pcm, pcm_args->buf, pcm_args->buf_len);
        if (frames < 0)
        	frames = snd_pcm_recover(pcm_args->pcm, frames, 0);
        if (frames < 0)
                printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
		/*
		if (frames == -EPIPE) {
			snd_pcm_prepare(pcm_args->pcm);
		}
		*/
		pcm_args->return_val = frames;
	}

	void write_to_speaker_after(uv_work_t *req, int status) {
		pcm_args_t *pcm_args = static_cast<pcm_args_t *>(req->data);
		Nan::HandleScope scope;
		Local<Value> frames = Nan::New<Int32>(pcm_args->return_val);
		Local<Function> cb = Nan::New(pcm_args->callback);
		const unsigned argc = 1;
		Local<Value> argv[argc] = { frames };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
		pcm_args->callback.Reset();
		delete pcm_args;
	}

	NAN_METHOD(WriteToSpeaker) {
		pcm_args_t *pcm_args = new pcm_args_t;
		pcm_args->pcm = reinterpret_cast<snd_pcm_t *>(UnwrapPointer(info[0]));
		pcm_args->buf = (char *) node::Buffer::Data(info[1].As<v8::Object>());
		pcm_args->buf_len = info[2]->Int32Value();
		pcm_args->callback.Reset(info[3].As<Function>());
		pcm_args->request.data = pcm_args;
		uv_queue_work(uv_default_loop(), &pcm_args->request, write_to_speaker, write_to_speaker_after);
	}


	NAN_METHOD(PollSync) {
		int fd = info[0]->Int32Value();
		short events = (short) info[1]->Int32Value();
		int timeout = info[2]->Int32Value();
		short status = poll_func(fd, events, timeout);
		info.GetReturnValue().Set(status);
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

	NAN_METHOD(SetPriority) {
	    int fd = info[0]->Int32Value();
	    int priority = info[1]->Int32Value();
	    if (setsockopt(fd, SOL_SOCKET, SO_PRIORITY, (const void *) &priority, sizeof(priority)) < 0)
	    	print_errno("setsockopt");
	}

	NAN_METHOD(MakeBlocking) {
	    int fd = info[0]->Int32Value();
		int flags = fcntl(fd, F_GETFL); // get flags
		if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0)
			print_errno("fcntl");
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

	NAN_METHOD(CloseFd) {
		int fd = info[0]->Int32Value();
		if (shutdown(fd, SHUT_RDWR)<  0)
			printf("Failed to shutdown socket\n");
		if (close(fd)<  0)
			printf("Failed to close socket\n");
	}

	NAN_METHOD(EnableSocket) {
		int fd = info[0]->Int32Value();
		enable_socket(fd);
	}

	NAN_METHOD(ReadOneByte) {
		int fd = info[0]->Int32Value();
		read_one_byte(fd);
	}

	static void init(Local<Object> exports) {

		Nan::SetMethod(exports, "openMic", OpenMic);
		Nan::SetMethod(exports, "closeMic", CloseMic);

		Nan::SetMethod(exports, "openSpeaker", OpenSpeaker);
		Nan::SetMethod(exports, "closeSpeaker", CloseSpeaker);

		Nan::SetMethod(exports, "msbcNew", MsbcNew);

		Nan::SetMethod(exports, "msbcFree", MsbcFree);
		Nan::SetMethod(exports, "enableSocket", EnableSocket);

		Nan::SetMethod(exports, "readOneByte", ReadOneByte);

		Nan::SetMethod(exports, "msbcDecode", MsbcDecode);
		Nan::SetMethod(exports, "msbcEncode", MsbcEncode);

		Nan::SetMethod(exports, "write", WriteAsync);
		Nan::SetMethod(exports, "writeSync", WriteSync);

		Nan::SetMethod(exports, "read", ReadAsync);
		Nan::SetMethod(exports, "readSync", ReadSync);

		Nan::SetMethod(exports, "recvmsgSync", RecvmsgSync);
		Nan::SetMethod(exports, "recvmsg", RecvmsgAsync);

		Nan::SetMethod(exports, "poll", PollAsync);
		Nan::SetMethod(exports, "pollSync", PollSync);
		Nan::SetMethod(exports, "makeBlocking", MakeBlocking);
		Nan::SetMethod(exports, "setPriority", SetPriority);

		Nan::SetMethod(exports, "readFromMic", ReadFromMic);
		Nan::SetMethod(exports, "writeToSpeaker", WriteToSpeaker);

		Nan::SetMethod(exports, "closeFd", CloseFd);

		Nan::SetMethod(exports, "sndPcmAvailUpdate", SndPcmAvailUpdate);
		Nan::SetMethod(exports, "sndPcmStart", SndPcmStart);
		Nan::SetMethod(exports, "sndPcmWait", SndPcmWait);
		//Nan::SetMethod(exports, "sndPcmPrepare", SndPcmPrepare);
	}

	NODE_MODULE(hfpio, init);
}
