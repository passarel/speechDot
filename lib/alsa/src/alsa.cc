#include "alsa.h"

namespace alsa {

	void print_errno(const char *method_name) {
		fprintf(stdout, "Oh dear, something went wrong with %s()! %s\n", method_name, strerror(errno));
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

	static void init(Local<Object> exports) {
		Nan::SetMethod(exports, "openMic", OpenMic);
		Nan::SetMethod(exports, "closeMic", CloseMic);
		Nan::SetMethod(exports, "openSpeaker", OpenSpeaker);
		Nan::SetMethod(exports, "closeSpeaker", CloseSpeaker);
		Nan::SetMethod(exports, "sndPcmAvailUpdate", SndPcmAvailUpdate);
		Nan::SetMethod(exports, "sndPcmStart", SndPcmStart);
		Nan::SetMethod(exports, "sndPcmWait", SndPcmWait);
		Nan::SetMethod(exports, "readFromMic", ReadFromMic);
		Nan::SetMethod(exports, "writeToSpeaker", WriteToSpeaker);
	}

	NODE_MODULE(alsa, init);

}

