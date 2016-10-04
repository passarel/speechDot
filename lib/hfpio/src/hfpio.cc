#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sbc/sbc.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "hfpio.h"

static const char sntable[4] = { 0x08, 0x38, 0xC8, 0xF8 };

struct sbc_info_t sbc_info;

Nan::Persistent<v8::Function> readFromMic;
Nan::Persistent<v8::Function> writeToSpeaker;
Nan::Persistent<v8::Function> onExit;

void print_errno() {
	fprintf(stdout, "Oh dear, something went wrong with read()! %s\n", strerror(errno));
}

void outq_free() {
	int i;
	for (i=0; i<OUTQ_SIZE; i++) {
		if (sbc_info.outq[i] != NULL) {
			free(sbc_info.outq[i]);
			sbc_info.outq[i] = NULL;
		}
	}
}

void buffers_free() {
	outq_free();
	if (sbc_info.ebuffer != NULL) {
		free(sbc_info.ebuffer);
		sbc_info.ebuffer = NULL;
	}
	if (sbc_info.capture_buffer != NULL) {
		free(sbc_info.capture_buffer);
		sbc_info.capture_buffer = NULL;
	}
	if (sbc_info.parser_buf.data != NULL) {
		free(sbc_info.parser_buf.data);
		sbc_info.parser_buf.data = NULL;
	}
}

void buffers_alloc() {
	if (sbc_info.ebuffer == NULL) {
		sbc_info.ebuffer = (char*) malloc(CAPTURE_SIZE);
	}
	if (sbc_info.capture_buffer == NULL) {
		sbc_info.capture_buffer = (char*) malloc(CAPTURE_SIZE);
	}
	if (sbc_info.parser_buf.data == NULL) {
		sbc_info.parser_buf.data = (uint8_t*) malloc(CAPTURE_SIZE);
	}
}

int outq_next() {
	int i;
	for (i=0; i<OUTQ_SIZE; i++) {
		if (sbc_info.outq[i] == NULL) return i;
	}
	// should not get here, if here error is somewhere else in code...
	printf("outq_next() -> OUTQ IS TOO SMALL!!!, RETURNING -1\n");
	return -1;
}

void parser_buf_reset() {
	sbc_info.parser_buf.len = 0;
}

void sbc_info_reset() {
	parser_buf_reset();
	buffers_free();
	buffers_alloc();
	sbc_info.ebuffer_start = 0;
	sbc_info.ebuffer_end = 0;
	sbc_info.captured = 0;
}

void sbc_info_init() {
	sbc_init_msbc(&sbc_info.sbcenc, 0L);
	sbc_init_msbc(&sbc_info.sbcdec, 0L);
	int i;
	for (i=0; i<OUTQ_SIZE; i++) {
		sbc_info.outq[i] = NULL;
	}
}

snd_pcm_t *hfp_audio_pcm_init(snd_pcm_stream_t stream, int rate) {
	snd_pcm_t *pcm;
	printf("Initializing pcm for %s\n", (stream == SND_PCM_STREAM_CAPTURE) ? "capture" : "playback");
	if (snd_pcm_open(&pcm, "default", stream, SND_PCM_NONBLOCK)<  0) {
		printf("Failed to open pcm\n");
		return NULL;
	}
	if (snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1, rate, 1, 120000) <  0) {
		printf("Failed to set pcm params\n");
		snd_pcm_close(pcm);
		pcm = NULL;
	}
	return pcm;
}

int msbc_state_machine(parser_buf_t *p, uint8_t byte) {
	assert(p->len < 60);
	switch (p->len) {
	case 0:
		if (byte == 0x01)
			goto copy;
		return 0;
	case 1:
		if (byte == 0x08 || byte == 0x38 || byte == 0xC8 || byte == 0xF8)
			goto copy;
		break;
	case 2:
		if (byte == 0xAD)
			goto copy;
		break;
	case 3:
		if (byte == 0x00)
			goto copy;
		break;
	case 4:
		if (byte == 0x00)
			goto copy;
		break;
	default:
		goto copy;
	}
	p->len = 0;
	return 0;

copy:
	p->data[p->len] = byte;
	p->len++;

	return p->len;
}

size_t msbc_parse(sbc_t* sbcdec, parser_buf_t* p, char* data, int len, char* out, int outlen, int* bytes) {
	size_t totalwritten = 0;
	size_t written = 0;
	int i;
	*bytes = 0;
	for (i = 0; i < len; i++) {
		if (msbc_state_machine(p, data[i]) == 60) {
			int decoded = 0;
			decoded = sbc_decode(sbcdec, p->data + 2, p->len - 2 - 1, out, outlen, &written);

			//printf("decoded = %d\n", decoded);

			if (decoded > 0) {
				totalwritten += written;
				*bytes += decoded;
			} else {
				printf("Error while decoding: %d\n", decoded);
			}
			parser_buf_reset();
		}
	}
	return totalwritten;
}

static int sn = 0;

int hfp_audio_msbc_encode(char *data, int len) {
	char* h2 = sbc_info.ebuffer + sbc_info.ebuffer_end;

	int written = 0;
	char* qbuf;
	h2[0] = 0x01;
	h2[1] = sntable[sn];
	h2[59] = 0xff;
	sn = (sn + 1) % 4;

	sbc_encode(&sbc_info.sbcenc, data, len,
			sbc_info.ebuffer + sbc_info.ebuffer_end + 2,
			CAPTURE_SIZE - sbc_info.ebuffer_end - 2,
			(ssize_t *) &written);

	written += 2 + 1;

	printf("sbc_info.ebuffer_end + 2 = %d\n", sbc_info.ebuffer_end + 2);

	printf("CAPTURE_SIZE - sbc_info.ebuffer_end - 2 = %d\n", CAPTURE_SIZE - sbc_info.ebuffer_end - 2);

	printf("sbc_encode_written = %d\n", written);

	printf("\n");

	sbc_info.ebuffer_end += written;
	// Split into MTU sized chunks
	while (sbc_info.ebuffer_start + MTU_SIZE <= sbc_info.ebuffer_end) {
		qbuf = (char*) malloc(MTU_SIZE);
		if (!qbuf)
			return -ENOMEM;
		// DBG("Enqueuing %d bytes", MTU_SIZE);
		memcpy(qbuf, sbc_info.ebuffer + sbc_info.ebuffer_start, MTU_SIZE);

		int next = outq_next();
		if (next >= 0)
			sbc_info.outq[next] = qbuf;

		sbc_info.ebuffer_start += MTU_SIZE;
		if (sbc_info.ebuffer_start>= sbc_info.ebuffer_end)
			sbc_info.ebuffer_start = sbc_info.ebuffer_end = 0;
	}
	return 0;
}

int hfp_audio_msbc_decode(char* data, int len, char* out, int outlen) {
	int written, decoded;
	written = msbc_parse(&sbc_info.sbcdec,&sbc_info.parser_buf, data, len, out, outlen, &decoded);
	return written;
}

/*
char* read_from_mic(int len) {
	v8::Local<v8::Function> cb = Nan::New(readFromMic);
	const unsigned argc = 1;
	v8::Local<v8::Value> argv[argc] = { Nan::New(len) };
	v8::Local<v8::Value> mic_data = Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
	//v8::Local<v8::Object> obj = rval->ToObject();
	while (mic_data->IsNull()) {
		usleep(2000);
		mic_data = Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
	}
	return (char*) node::Buffer::Data(mic_data.As<v8::Object>());
}

char* mic_data = read_from_mic(CAPTURE_SIZE);
printf("GOT HERE JUST GOT SOME MIC DATA \n");
hfp_audio_msbc_encode(mic_data, CAPTURE_SIZE);
*/

int hfp_audio_capture(snd_pcm_t *capture) {
	snd_pcm_sframes_t frames;

	printf("(CAPTURE_SIZE - sbc_info.captured) / 2 = %d\n", (CAPTURE_SIZE - sbc_info.captured) / 2);

	frames = snd_pcm_readi(capture, sbc_info.capture_buffer + sbc_info.captured, (CAPTURE_SIZE - sbc_info.captured) / 2);

	printf("frames = %d\n", frames);

	switch (frames) {
	case -EPIPE:
		printf("Capture overrun\n");
		snd_pcm_prepare(capture);
		return 0;
	case -EAGAIN:
		//printf("No data to capture\n");
		return 0;
	case -EBADFD:
	case -ESTRPIPE:
		printf("Other error %s (%d)\n", strerror(frames), (int) frames);
		return -EINVAL;
	}

	printf("sbc_info.captured = %d\n", sbc_info.captured);

	sbc_info.captured += frames * 2;

	printf("frames * 2 = %d\n", frames * 2);

	printf("\n");

	if (sbc_info.captured <  CAPTURE_SIZE)
		return frames * 2;
	// DBG("Encoding %d bytes", (int) thread->captured);
	hfp_audio_msbc_encode(sbc_info.capture_buffer, sbc_info.captured);
	sbc_info.captured = 0;
	return frames * 2;
}

void write_to_speaker(Nan::MaybeLocal<v8::Object> buf) {
	v8::Local<v8::Function> cb = Nan::New(writeToSpeaker);
	const unsigned argc = 1;
	v8::Local<v8::Value> argv[argc] = { buf.ToLocalChecked() };
	Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, argc, argv);
}

int hfp_audio_playback(int fd) {
	char playback_buf[MTU_SIZE], playback_out[CAPTURE_SIZE];
	int bytes, outlen;
	bytes = read(fd, playback_buf, MTU_SIZE);
	if (bytes < 0) {
		printf("Failed to read: bytes %d, errno %d\n", bytes, errno);
		switch (errno) {
		case ENOTCONN:
			return -ENOTCONN;
		case EAGAIN:
			return 0;
		default:
			return -EINVAL;
		}
	}
	if (bytes > 0) {
		outlen = hfp_audio_msbc_decode(playback_buf, bytes, playback_out, CAPTURE_SIZE);
		if (outlen > 0) {
			write_to_speaker(Nan::CopyBuffer(playback_out, outlen));
		}
	}
	return 0;
}

void pop_outq(int fd) {
	int i;
	for (i=0; i<OUTQ_SIZE; i++) {
		if (sbc_info.outq[i] == NULL) return;
		if (write(fd, sbc_info.outq[i], MTU_SIZE) <  0)
			printf("Failed to write: %d\n", errno);
		free(sbc_info.outq[i]);
		sbc_info.outq[i] = NULL;
	}
}

void enable_socket(int fd) {
    printf("enabling socket by reading 1 byte...\n");
    char c;
    if (read(fd, &c, 1) < 0)
    	print_errno();
    printf("socket is now enabled\n");
}

void msbc_io(uv_work_t *req) {

	int fd = *(int *) req->data;
	printf("msbc_io started...\n");

	snd_pcm_t *capture;
	sbc_info_reset();

	capture = hfp_audio_pcm_init(SND_PCM_STREAM_CAPTURE, RATE);

	//if (recv(fd, NULL, 0, 0) <  0)
		//printf("Defered setup failed: %d (%s)", errno, strerror(errno));

	enable_socket(fd);

	struct pollfd pfd[1];
	while (true) {

		if (hfp_audio_capture(capture) <  0) {
			printf("Failed to capture\n");
			break;
		}

		pfd[0].fd = fd;
		pfd[0].events = POLLIN;

		if (poll(pfd, 1, 200) == 0)
			continue;

		if (pfd[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			//printf("POLLERR | POLLHUP | POLLNVAL triggered (%d)\n", pfd[0].revents);
			break;
		}

		if (!pfd[0].revents & POLLIN)
			continue;

		if (hfp_audio_playback(fd) <  0) {
			printf("POLLIN triggered, but read error\n");
			break;
		}

		pop_outq(fd);
	}
	//snd_pcm_close(playback);
	snd_pcm_close(capture);
	buffers_free();
	printf("msbc_io exited gracefully...\n");
}

void msbc_io_after(uv_work_t *req) {
	printf("got here - msbc_io_after -...\n");

	v8::Local<v8::Function> cb = Nan::New(onExit);
	Nan::MakeCallback(Nan::GetCurrentContext()->Global(), cb, 0, NULL);
}

void OnWriteToSpeaker(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	writeToSpeaker.Reset(info[0].As<v8::Function>());
}

void OnReadFromMic(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	readFromMic.Reset(info[0].As<v8::Function>());
}

void OnExit(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	//onExit.Reset(info[0].As<v8::Function>());
}

void MsbcIo(const Nan::FunctionCallbackInfo<v8::Value>& info) {

	//int fd = info[0]->Int32Value();
	//msbc_io(fd);

	int fd = info[0]->Int32Value();
	uv_work_t req;
	req.data = (void *) &fd;

	msbc_io(&req);
	//msbc_io_after(&req);

	//uv_queue_work(uv_default_loop(), &req, msbc_io, (uv_after_work_cb) msbc_io_after);
}

void Write(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	int fd = info[0]->Int32Value();
	unsigned char* buf = (unsigned char*) node::Buffer::Data(info[1].As<v8::Object>());
	int buf_len = info[2]->Int32Value();
	int written = write(fd, buf, buf_len);
	info.GetReturnValue().Set(written);
}

void Read(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	int fd = info[0]->Int32Value();
	unsigned char* buf = (unsigned char*) node::Buffer::Data(info[1].As<v8::Object>());
	int buf_len = info[2]->Int32Value();
	struct iovec iov;
	struct msghdr m;
	m.msg_iov = &iov;
	m.msg_iovlen = 1;
    iov.iov_base = buf;
    iov.iov_len = buf_len;
    info.GetReturnValue().Set(recvmsg(fd, &m, 0));
}

void EnableSocket(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	int fd = info[0]->Int32Value();
	enable_socket(fd);
    /*
	fprintf(stdout, "enabling socket by reading 1 byte...\n");
    char c;
    if (read(fd, &c, 1) < 0)
    	print_errno();
    fprintf(stdout, "socket is now enabled\n");
    */
}

void Poll(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	int fd = info[0]->Int32Value();
	short events = (short) info[1]->Int32Value();
	int timeout = info[2]->Int32Value();
	struct pollfd pfd[1];
    pfd[0].fd = fd;
    pfd[0].events = events;
    int status = poll(pfd, 1, timeout);
	if (status < 0)
		print_errno();
	if (status <= 0)
		info.GetReturnValue().Set(status);
	else
		info.GetReturnValue().Set(pfd[0].revents);
}

void SetPriority(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int fd = info[0]->Int32Value();
    int priority = info[1]->Int32Value();
    if (setsockopt(fd, SOL_SOCKET, SO_PRIORITY, (const void *) &priority, sizeof(priority)) < 0)
    	print_errno();
}

void MakeBlocking(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    int fd = info[0]->Int32Value();
    //fprintf(stdout, "trying to set flags to BLOCKING for FD: %d\n", fd);
	int flags = fcntl(fd, F_GETFL); // get flags
	if (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0)
		print_errno();
}

void Init(v8::Local<v8::Object> exports) {
	exports->Set(Nan::New("makeBlocking").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(MakeBlocking)->GetFunction());
	exports->Set(Nan::New("setPriority").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(SetPriority)->GetFunction());
	exports->Set(Nan::New("poll").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Poll)->GetFunction());
	exports->Set(Nan::New("enableSocket").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(EnableSocket)->GetFunction());
	exports->Set(Nan::New("write").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Write)->GetFunction());
	exports->Set(Nan::New("read").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(Read)->GetFunction());
	exports->Set(Nan::New("msbcIo").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(MsbcIo)->GetFunction());
	exports->Set(Nan::New("onWriteToSpeaker").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnWriteToSpeaker)->GetFunction());
	exports->Set(Nan::New("onReadFromMic").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnReadFromMic)->GetFunction());
	exports->Set(Nan::New("onExit").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(OnExit)->GetFunction());
	sbc_info_init();

	printf("-EPIPE -> %d\n", -EPIPE);
}

NODE_MODULE(hfpio, Init);

