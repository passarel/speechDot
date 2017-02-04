const spawn = require('child_process').spawn;
const a2dp = require('bindings')('a2dp');

const POLLNVAL = 0x020;
const POLLERR = 0x008;
const POLLIN = 0x001;
const POLLOUT = 0x004;
const POLLHUP = 0x010;

const transportState = {}

a2dp.setTransportState = function(path, state) {
	transportState[path] = state;
}

a2dp.play = function(path, fd, mtu, config) {
	
	const read_buf = Buffer.alloc(mtu);
	console.log();
	console.log('a2dp.play() fd: ' + fd + ' config: ' + config);
	console.log();

	const aplay = spawn('aplay', ['-f', 'S16_LE', '-c', '2', '-r', '44100', '-t', 'raw', '--buffer-size', '4096']);
	aplay.stdin._writableState.highWaterMark = 8192;
	aplay.stderr.on('data', function(data) {});
	aplay.stdin.on('error', function(data) {
		console.log('a2dp.aplay.stdin_error -> ' + data.toString());
	});
	
	const decoder = a2dp.sbcNew(Buffer.from(config));
	a2dp.setupSocket(fd);

	const cleanUp = function(pval) {
		console.log("a2dp.pval <= 0 || (POLLERR | POLLHUP | POLLNVAL) triggered -> " + pval);
		aplay.kill('SIGKILL');
		a2dp.sbcFree(decoder);
	}
	
	const loop = function() {
		a2dp.poll(fd, POLLIN, 50, function(pval) {
			// check pval and exit conditions
			if (pval <= 0 || (pval & (POLLERR | POLLHUP | POLLNVAL))) {
				if (pval == 0 && transportState[path] != 'idle') {
					process.nextTick(loop);
				} else {
					cleanUp(pval);
				}
				return;
			}
			// socket is ready for read, so lets read...
			a2dp.read(fd, read_buf, mtu, function(bytes) {
				if (bytes > 0) {
					a2dp.decode(decoder, read_buf, bytes, function(data) {
						aplay.stdin.write(data);
						process.nextTick(loop);
					});
				} else {
					console.log('a2dp: Failed to read MTU_SIZE bytes from socket');
				}
			});
		});
	}
	
	loop();
}

module.exports = a2dp;