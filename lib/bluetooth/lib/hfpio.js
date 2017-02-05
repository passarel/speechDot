const spawn = require('child_process').spawn;
const assert = require('assert');
const hfpio = require('bindings')('hfpio');

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
	outqLength = 0;
	hfpio.resetParserBuf();
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
						console.log('hfpio - !! ERROR, (BUG) COULD NOT Q WRITE TO BUFFER!!! FIX ME!!');
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
		console.log('hfpio:Failed to write MTU_SIZE bytes to socket');
	}
}

const playback_buf = Buffer.alloc(CAPTURE_SIZE);
const read_buf = Buffer.alloc(MTU_SIZE);

const audioPlayback = function(aplay, decoder, fd, onComplete) {
	hfpio.read(fd, read_buf, MTU_SIZE, function(bytes) {
		if (bytes > 0) {
			hfpio.audioDecode(decoder, read_buf, read_buf.length, playback_buf, playback_buf.length, function(written) {
				if (written > 0) {
					aplay.stdin.write(playback_buf);
				}
				onComplete();
			});
			
		} else {
			console.log('hfpio: Failed to read MTU_SIZE bytes from socket');
			onComplete();
		}
	});
}

hfpio.msbcIo = function(fd) {
	if (IS_BUSY) {
		console.log('hfpio.msbcIo - IS_BUSY=true, returning...');
		return;
	}
	IS_BUSY = true;
	
	const encoder = hfpio.msbcNew();
	const decoder = hfpio.msbcNew();

	resetBuffers();
	hfpio.setupSocket(fd);
	
	const arecord = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw', '--buffer-size', '512']);
	arecord.stdout._readableState.highWaterMark = 1024;

	const aplay = spawn('aplay', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw', '--buffer-size', '4096']);
	aplay.stdin._writableState.highWaterMark = 8192;
	
	arecord.stderr.on('data', function(data) {});
	arecord.stdout.on('error', function(data) {
		console.log('hfpio.arecord.stdout_error -> ' + data.toString());
	});
	aplay.stderr.on('data', function(data) {});
	aplay.stdin.on('error', function(data) {
		console.log('hfpio.aplay.stdin_error -> ' + data.toString());
	});

	const cleanUp = function(pval) {
		console.log("hfpio pval <= 0 || (POLLERR | POLLHUP | POLLNVAL) triggered -> " + pval);
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
		console.log('hfpio.cvsdIo - IS_BUSY=true, returning...');
		return;
	}
	IS_BUSY = true;
	
	hfpio.setupSocket(fd);

	const arecord = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '8000', '-t', 'raw', '--buffer-size', '2048']);
	const aplay = spawn('aplay', ['-f', 'S16_LE', '-c', '1', '-r', '8000', '-t', 'raw', '--buffer-size', '2048']);

	arecord.stdout._readableState.highWaterMark = 4096;
	arecord.stderr.on('data', function(data) {});
	arecord.stdout.on('error', function(data) {
		console.log('hfpio.arecord.stdout_error -> ' + data.toString());
	});
	
	aplay.stdin._writableState.highWaterMark = 4096;
	aplay.stderr.on('data', function(data) {});
	aplay.stdin.on('error', function(data) {
		console.log('hfpio.aplay.stdin_error -> ' + data.toString());
	});
	
	const cleanUp = function(pval) {
		console.log("hfpio.pval <= 0 || (POLLERR | POLLHUP | POLLNVAL) triggered -> " + pval);
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
