const spawn = require('child_process').spawn;
const assert = require('assert');
const hfpio = require('bindings')('hfpio');

const POLLNVAL = 0x020;
const POLLERR = 0x008;
const POLLIN = 0x001;
const POLLOUT = 0x004;
const POLLHUP = 0x010;

const MTU_SIZE = 48;
const CAPTURE_SIZE = 240;
const MSBC_RATE = 16000;
const CVSD_RATE = 8000;

var IS_BUSY = false;

// these are buffers for IO
const playback_buf = Buffer.alloc(CAPTURE_SIZE);
const read_buf = Buffer.alloc(MTU_SIZE);

const audioPlayback = function(aplay, decoder, fd, onComplete) {
	hfpio.readAndDecode(fd, decoder, read_buf, MTU_SIZE, playback_buf, playback_buf.length, function(written) {
		if (written > 0) {
			aplay.stdin.write(playback_buf);
		}
		onComplete();
	});
}

const encodeAudioAndWrite = function(fd, encoder, data, onComplete) {
	if (data == null || data.length <= 0) {
		onComplete();
	} else {
		hfpio.encodeAndWrite(fd, encoder, data, data.length, onComplete);
	}
}

hfpio.msbcIo = function(fd) {
	if (IS_BUSY) {
		console.log('hfpio.msbcIo - IS_BUSY=true, returning...');
		return;
	}
	IS_BUSY = true;
	
	const encoder = hfpio.msbcNew();
	const decoder = hfpio.msbcNew();

	hfpio.resetBuffers();
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
	
	const loop = function() {
		hfpio.poll(fd, POLLIN, -1, function(pval) {
			if (pval <= 0 || (pval & (POLLERR | POLLHUP | POLLNVAL))) {
				cleanUp(pval);
				return;
			}
			encodeAudioAndWrite(fd, encoder, arecord.stdout.read(), function() {
				audioPlayback(aplay, decoder, fd, loop);
			});
		});
	}
	
	loop();
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
			hfpio.recvmsg(fd, read_buf, MTU_SIZE, function() {
				aplay.stdin.write(read_buf);
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
			getMicData(onMicData);
		});
	});
}

module.exports = hfpio;
