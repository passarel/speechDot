#define MTU_SIZE 48
#define RATE 16000
#define CAPTURE_SIZE 240
#define OUTQ_SIZE 16
#define PARSER_BUF_SIZE 60

struct parser_buf_t {
  int len;
  uint8_t* data;
};

struct sbc_info_t {
	sbc_t sbcenc;
	sbc_t sbcdec;

	//char ebuffer[CAPTURE_SIZE];
	char* ebuffer;

	size_t ebuffer_start;
	size_t ebuffer_end;
	struct parser_buf_t parser_buf;

	//char capture_buffer[CAPTURE_SIZE];
	char* capture_buffer;

	int captured;
	char* outq[OUTQ_SIZE];
};
