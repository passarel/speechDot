/*
const ProcessManager = require('./src/ProcessManager.js');
const Bluez = require('./src/dbus/Bluez.js');
const Ofono = require('./src/dbus/Ofono.js');
const SerialPortProfile = require('./src/dbus/SerialPortProfile.js');
const HandsFreeAudioAgent = require('./src/dbus/HandsFreeAudioAgent.js');

Bluez.on('ready', function(adapters) {

});
*/

const fs = require('fs');
const fcntl = require('./lib/fcntl');

var readStream = fs.createReadStream('/home/pi/test.raw');

const outStream = fs.createWriteStream('/home/pi/sbc_decoded.data');

const getFileData = function() {

	var data = readStream.read(57);
	
	if (data == null) {
		outStream.end();
		process.exit();
	} else {

		console.log('got here');
		
		const output = new Buffer(240);
		fcntl.sbcEncode(data, 57, output, 240);
		outStream.write(output);
		
		setTimeout(function() {
			getFileData();
		}, 100);
	}
}

setTimeout(function() {
	getFileData();
}, 100);



/*
const spawn = require('child_process').spawn;

const arecord = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw']);

const outStream = fs.createWriteStream('/home/pi/sbc_encoded.data');

const CHUNK_SIZE = 240;

const getMicData = function(onData) {
	var data = arecord.stdout.read(CHUNK_SIZE);
	if (data == null) {
		setTimeout(function() {
			getMicData(onData);
		}, 2);
	} else {
		onData(data);
		//const _data = Buffer.alloc(CHUNK_SIZE);
		//onData(_data);
	}
}

var count = 0;

const onMicData = function(data) {
	count++;
	if (count == 1000) {
		outStream.end();
		process.exit();
	}
	
	const input = new Buffer(240);
	const output = new Buffer(57);
	
	fcntl.sbcEncode(input, 240, output, 57);
	
	outStream.write(output);
	
	console.log();
	
	getMicData(onMicData);
}

arecord.stderr.on('data', function(data) {
	console.log('arecord.stderr_data');
	console.log(data.toString());
});

getMicData(onMicData);
*/

