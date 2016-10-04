const hfpio = require('bindings')('hfpio');

/*
const POLLNVAL = 0x020;
const POLLERR = 0x008;
const POLLIN = 0x001;
const POLLOUT = 0x004;
const POLLHUP = 0x010;

const MTU_SIZE = 48;
const RATE = 16000;
const CAPTURE_SIZE = 240;

const parserBuf = [];
const outq = [];
const sntable = [0x08, 0x38, 0xC8, 0xF8];
var sn = 0;
var remainder_buf = null;

const resetBuffers = function() {
	parserBuf.length = 0;
	outq.length = 0;
	ebufferEnd = 0;
	ebufferStart = 0;
}

const copy = function() {
	parserBuf.push(byte);
	return parserBuf.length;
}

const msbcStateMachine = function(byte) {
	assert(parserBuf.length < 60);
	switch (parserBuf.length) {
	case 0:
		if (byte == 0x01)
			return copy();
		return 0;
	case 1:
		if (byte == 0x08 || byte == 0x38 || byte == 0xC8 || byte == 0xF8)
			return copy();
		break;
	case 2:
		if (byte == 0xAD)
			return copy();
		break;
	case 3:
		if (byte == 0x00)
			return copy();
		break;
	case 4:
		if (byte == 0x00)
			return copy();
		break;
	default:
		return copy();
	}
	parserBuf.length = 0;
	return 0;
}

const msbcParse = function(data, out) {
	var totalWritten = 0;
	var totalDecoded = 0;
	var written = 0;
	for (i = 0; i < data.length; i++) {
		if (msbcStateMachine(data[i]) == 60) {
			const buf = parserBuf.slice(2, parserBuf.length-1);
			assert(buf.length == 57);
			const r = hfpio.msbcDecode(buf, buf.length, out, out.length);
			//console.log('decoded result -> ' + JSON.stringify(r));
			if (r.decoded > 0) {
				totalDecoded += r.decoded;
				totalwritten += r.written;
			} else {
				console.log('Error while decoding, decoded -> ' + r.decoded);
			}
			parserBuf.length = 0; //reset
		}
	}
	return {written: totalWritten, decoded: totalDecoded};
}

const audioEncodeAndQ = function(data) {
	var encoded_buf =  audioEncode(data);
	if (remainder_buf && remainder_buf.length > 0) {
		var total_length = encoded_buf.length + remainder_buf.length;
		encoded_buf = Buffer.concat([encoded_buf, remainder_buf], totalLength);
	}
	var index = 0;
	while (true) {
		var qbuf = encoded_buf.slice(index, MTU_SIZE);
		outq.push(qbuf);
		if (index + MTU_SIZE > encoded_buf.length) {
			if (index + 1 < encoded_buf.length) {
				remainder_buf = encoded_buf.slice(index+1);
			} else {
				// there is no remainder
				remainder_buf = null;
			}
			break;
		} else {
			index += MTU_SIZE;
		}
	}
}

const audioEncode = function(data) {
	const out = Buffer.alloc(60);
	out[0] = 0x01;
	out[1] = sntable[sn];
	out[59] = 0xff;
	sn = (sn + 1) % 4;
	const cropped_out = out_buf.slice(2, out_buf.length-1);
	var r = hfpio.msbcEncode(data, data.length, cropped_out, cropped_out.length);
	assert(r.written = 57);
	return out;
}

const audioDecode = function(data, out) {
	return msbcParse(data, out);
}

const popOutq = function(fd) {
	while (outq.length > 0) {
		hfpio.write(fd, outq[0], MTU_SIZE);
		outq.splice(0, 1);
	}
}

const audioPlayback = function(fd, aplay) {
	const read_buf = Buffer.alloc(MTU_SIZE);
	var bytes = hfpio.read(fd, read_buf, MTU_SIZE);
	if (bytes > 0) {
		const playback_buf = Buffer.alloc(CAPTURE_SIZE);
		var r = audioDecode(read_buf, playback_buf);
		//console.log('audioDecode result -> ' + JSON.stringify(r));
		if (r.written > 0) {
			aplay.stdin.write(playback_buf);
		}
	} else {
		console.log('Failed to read MTU_SIZE bytes from socket');
	}
}

hfpio.msbcIo = function(fd) {
	
	const arecord = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw']);
	const aplay = spawn('aplay', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw']);
	
	hfpio.enableSocket();
	
	const getMicData = function(onData) {
		var data = arecord.stdout.read(CHUNK_SIZE);
		if (data == null) {
			setTimeout(function() {
				getMicData(onData);
			}, 2);
		} else {
			onData(data);
		}
	}
	
	const onMicData = function(data) {
		audioEncodeAndQ(data);
		var pval = fcntl.poll(fd, POLLIN, 200);
		console.log('pval = ' + pval);
		if (pval & (POLLERR | POLLHUP | POLLNVAL)) {
			console.log("POLLERR | POLLHUP | POLLNVAL triggered -> " + pval);
			arecord.kill('SIGKILL');
			aplay.kill('SIGKILL');
			return;
		}
		if (pval == 0 || !(pval & POLLIN)) {
			process.nextTick(function() {
				onMicData(data);
			});
			return;
		}
		audioPlayback(fd, data);
		popOutq(fd);
		process.nextTick(function() {
			onMicData(data);
		});
	}
	
	getMicData(onMicData);
}
*/

module.exports = hfpio;
