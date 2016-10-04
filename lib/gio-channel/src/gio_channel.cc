#include <glib.h>
#include <stdio.h>
#include <nan.h>
#include <string.h>

GMainLoop *loop;
v8::Local<v8::Function> data_cb;

void read(GIOChannel *ioch, GIOCondition cond, gpointer data) {
	gchar buf[4096];
	gsize bytes_read;
	GError *error = NULL;
	do {
	
		fprintf(stdout, "TRYING TO READ DATA\n");
		
		g_io_channel_read_chars(ioch, buf, 4096, &bytes_read, &error);
		
		//memcpy( subbuff, &buff[10], 4 );
		
		
		if (bytes_read > 0) {
			gchar _buf[bytes_read];
			memcpy( _buf, &buff[0], bytes_read);
		}
		
		g_print("buf.length: %d\n", bytes_read);
		
		g_print("bytes_read: %d\n", bytes_read);
		
		if (error != NULL) {
			fprintf(stdout, "error: %s\n", error->message);
		}
		
		
		fprintf(stdout, "received data\n");
		
		//todo call the watch_function with data
	} while (bytes_read > 0);
}

void on_data(GIOChannel *ioch) {

	fprintf(stdout, "g_io_channel_get_buffer_condition\n");

	GIOCondition cond = g_io_channel_get_buffer_condition(ioch);
	
	fprintf(stdout, "cond = %d\n", cond);

	fprintf(stdout, "adding watch to file\n");

	read(ioch, G_IO_IN, NULL);
	
		cond = g_io_channel_get_buffer_condition(ioch);
	
	fprintf(stdout, "cond = %d\n", cond);

	g_io_add_watch(ioch, G_IO_IN, (GIOFunc) read, NULL);
}

void OnData(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	int fd = info[0]->Int32Value();
	
	fprintf(stdout, "got here, fd=%d\n", fd);
	
	data_cb = info[1].As<v8::Function>();
	GIOChannel *ioch = g_io_channel_unix_new(fd);
	on_data(ioch);
}

void Init(v8::Local<v8::Object> exports) {
  exports->Set(Nan::New("onData").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(OnData)->GetFunction());
               
    //loop = g_main_loop_new(NULL,FALSE);
	//g_main_loop_run(loop);
}

NODE_MODULE(gio_channel, Init);
