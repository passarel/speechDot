const hfpio = require('bindings')('hfpio');
const spawn = require('child_process').spawn;
const assert = require('assert');

const POLLNVAL = 0x020;
const POLLERR = 0x008;
const POLLIN = 0x001;
const POLLOUT = 0x004;
const POLLHUP = 0x010;

const MTU_SIZE = 48;
const MSBC_RATE = 16000;
const CVSD_RATE = 8000;
const CAPTURE_SIZE = 240;
const OUTQ_SIZE = 44;

const parserBuf = Buffer.alloc(60);
var parserBufLength = 0;

const sntable = [0x08, 0x38, 0xC8, 0xF8];
var sn = 0;

const outq = [];
var outqLength = 0;

// ONLY DO THIS ONCE
const initOutq = function() {
	for (var z=0; z<OUTQ_SIZE; z++) {
		outq[z] = Buffer.alloc(MTU_SIZE);
	}
}
initOutq();

const encoderBuf = Buffer.alloc(CAPTURE_SIZE);
var encoderBufLength = 0;

const writeBuf = Buffer.alloc(MTU_SIZE);
var writeBufLength = 0;

var IS_BUSY = false;

const resetBuffers = function() {
	sn = 0;
	encoderBufLength = 0;
	writeBufLength = 0;
	parserBufLength = 0;
	outqLength = 0;
}

const copy = function(byte) {
	parserBuf[parserBufLength] = byte;
	parserBufLength++;
	return parserBufLength;
}

const msbcCopyMachine = function(byte) {
	assert(parserBufLength < 60);
	switch (parserBufLength) {
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
	parserBufLength = 0;
	return 0;
}

const audioDecode = function(decoder, data, out) {
	assert(data.length == MTU_SIZE); //ensures that at most 1 decode occurs...
	var totalWritten = 0;
	var totalDecoded = 0;
	for (i = 0; i < data.length; i++) {
		if (msbcCopyMachine(data[i]) == 60) {
			const r = hfpio.msbcDecode(decoder, parserBuf.slice(2, 59), 57, out, out.length);
			if (r.decoded > 0) {
				totalDecoded += r.decoded;
				totalWritten += r.written;
			} else {
				console.log('Error while decoding, decoded -> ' + r.decoded);
			}
			parserBufLength = 0; //reset
		}
	}
	return {written: totalWritten, decoded: totalDecoded};
}

const encode_out = Buffer.alloc(60);
const cropped_encode_out = encode_out.slice(2, encode_out.length-1);

const audioEncode = function(encoder, data) {
	for (i = 0; i < data.length; i++) {
		encoderBuf[encoderBufLength] = data[i];
		encoderBufLength++;
		if (encoderBufLength == CAPTURE_SIZE) {
			encode_out[0] = 0x01;
			encode_out[1] = sntable[sn];
			encode_out[59] = 0xff;
			sn = (sn + 1) % 4;
			const r = hfpio.msbcEncode(encoder, encoderBuf, CAPTURE_SIZE, cropped_encode_out, 57);
			assert(r.written = 57);
			for (j = 0; j < encode_out.length; j++) {
				writeBuf[writeBufLength] = encode_out[j];
				writeBufLength++;
				if (writeBufLength == MTU_SIZE) {
					if (outqLength < OUTQ_SIZE) {
						writeBuf.copy(outq[outqLength], 0, 0, MTU_SIZE);
						outqLength++;
					} else {
						console.log('!! ERROR, (BUG) COULD NOT Q WRITE TO BUFFER!!! FIX ME!!');
					}
					writeBufLength = 0;
				}
			}
			encoderBufLength = 0;
		}
	}
}

const popOutq = function(fd, onComplete) {
	var i = 0;
	const nextQ = function() {
		if (i < outqLength) {
			i++;  
			return outq[i-1];
		}
		return null;
	}
	const wrtieNext = function() {
		const q = nextQ();
		if (q) {
			hfpio.write(fd, q, MTU_SIZE, function(written) {
				assertWrittenToSocket(written);
				wrtieNext();
			});
		} else {
			outqLength = 0;
			onComplete();
		}
	}
	wrtieNext();
}

const assertWrittenToSocket = function(written) {
	if (written > 0) {
		assert(written == MTU_SIZE);
	} else {
		console.log('Failed to write MTU_SIZE bytes to socket');
	}
}

const playback_buf = Buffer.alloc(CAPTURE_SIZE);
const read_buf = Buffer.alloc(MTU_SIZE);

const audioPlayback = function(aplay, decoder, fd, onComplete) {
	hfpio.read(fd, read_buf, MTU_SIZE, function(bytes) {
		if (bytes > 0) {
			const r = audioDecode(decoder, read_buf, playback_buf);
			if (r.written > 0) {
				aplay.stdin.write(playback_buf);
			}
			onComplete();
		} else {
			console.log('Failed to read MTU_SIZE bytes from socket');
			onComplete();
		}
	});
}

hfpio.msbcIo = function(fd) {
	if (IS_BUSY) {
		console.log('msbcIo - IS_BUSY=true, returning...');
		return;
	}
	IS_BUSY = true;
	
	const encoder = hfpio.msbcNew();
	const decoder = hfpio.msbcNew();

	resetBuffers();
	hfpio.enableSocket(fd);

	const arecord = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw', '--buffer-size', '512']);
	arecord.stdout._readableState.highWaterMark = 1024;

	const aplay = spawn('aplay', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw', '--buffer-size', '4096']);
	aplay.stdin._writableState.highWaterMark = 8192;
	
	arecord.stderr.on('data', function(data) {});
	arecord.stdout.on('error', function(data) {
		console.log('arecord.stdout_error -> ' + data.toString());
	});
	aplay.stderr.on('data', function(data) {});
	aplay.stdin.on('error', function(data) {
		console.log('aplay.stdin_error -> ' + data.toString());
	});

	const cleanUp = function(pval) {
		console.log("pval <= 0 || (POLLERR | POLLHUP | POLLNVAL) triggered -> " + pval);
		arecord.kill('SIGKILL');
		aplay.kill('SIGKILL');
		hfpio.msbcFree(encoder);
		hfpio.msbcFree(decoder);
		hfpio.closeFd(fd);
		IS_BUSY = false;
	}

	const getMicData = function(onData) {
		const data = arecord.stdout.read();
		if (data == null) onData(null);
		else {
			if (data.length > MTU_SIZE * OUTQ_SIZE) {
				onData(data.slice(0, MTU_SIZE * OUTQ_SIZE));
			} else {
				onData(data);
			}
		}
	}
	
	const onMicData = function(data) {
		if (data != null) {
			audioEncode(encoder, data);
		}
		hfpio.poll(fd, POLLIN, -1, function(pval) {
			if (pval <= 0 || (pval & (POLLERR | POLLHUP | POLLNVAL))) {
				cleanUp(pval);
				return;
			}
			audioPlayback(aplay, decoder, fd, function() {
				popOutq(fd, function() {
					getMicData(onMicData);
				});
			});
		});
	}
	
	getMicData(onMicData);
}

hfpio.cvsdIo = function(fd) {
	if (IS_BUSY) {
		console.log('cvsdIo - IS_BUSY=true, returning...');
		return;
	}
	IS_BUSY = true;
	
	hfpio.makeBlocking(fd);
	hfpio.setPriority(fd, 6);
	hfpio.readOneByte(fd); //enables socket

	const arecord = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '8000', '-t', 'raw', '--buffer-size', '2048']);
	const aplay = spawn('aplay', ['-f', 'S16_LE', '-c', '1', '-r', '8000', '-t', 'raw', '--buffer-size', '2048']);

	arecord.stdout._readableState.highWaterMark = 4096;
	arecord.stderr.on('data', function(data) {});
	arecord.stdout.on('error', function(data) {
		console.log('arecord.stdout_error -> ' + data.toString());
	});
	
	aplay.stdin._writableState.highWaterMark = 4096;
	aplay.stderr.on('data', function(data) {});
	aplay.stdin.on('error', function(data) {
		console.log('aplay.stdin_error -> ' + data.toString());
	});
	
	const cleanUp = function(pval) {
		console.log("pval <= 0 || (POLLERR | POLLHUP | POLLNVAL) triggered -> " + pval);
		arecord.kill('SIGKILL');
		aplay.kill('SIGKILL');
		hfpio.closeFd(fd);
		IS_BUSY = false;
	}
	
	const getMicData = function(onData) {
		const data = arecord.stdout.read(MTU_SIZE);
		if (data == null) {
			setTimeout(function() {
				getMicData(onData);
			}, 2);
		} else {
			onData(data);
		}
	}
	
	var write = true;
	
	const doRead = function(pval, onComplete) {
		if (!write && (pval & POLLIN)) {
			hfpio.recvmsg(fd, writeBuf, MTU_SIZE, function() {
				aplay.stdin.write(writeBuf);
				write = true;
				onComplete();
			});
		} else {
			onComplete();
		}
	}
	
	const onMicData = function(data) {
		hfpio.poll(fd, POLLOUT | POLLIN, -1, function(pval) {
			if (pval <= 0 || (pval & ~(POLLOUT|POLLIN))) {
				cleanUp(pval);
				return;
			}
			if (write && (pval & POLLOUT)) {
				hfpio.write(fd, data, MTU_SIZE, function(written) {
					assertWrittenToSocket(written);
					write = false;
					doRead(pval, function() {
						getMicData(onMicData);
					});
				});	
			} else {
				doRead(pval, function() {
					getMicData(onMicData);
				});
			}
		});
	}
	
	getMicData(function(data) {
		hfpio.write(fd, data, MTU_SIZE, function(written) {
			assertWrittenToSocket(written);
			getMicData(onMicData);
		});
	});
}

module.exports = hfpio;