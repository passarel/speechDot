/*
const ProcessManager = require('./src/ProcessManager.js');
const Bluez = require('./src/dbus/Bluez.js');
const Ofono = require('./src/dbus/Ofono.js');
const SerialPortProfile = require('./src/dbus/SerialPortProfile.js');
const HandsFreeAudioAgent = require('./src/dbus/HandsFreeAudioAgent.js');

Bluez.on('ready', function(adapters) {

});
*/
const spawn = require('child_process').spawn;

const SoundMeter = require('./alexa/client/SoundMeter.js');

const soundMeter = new SoundMeter();

const child = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw', '--buffer-size', '2048']);
child.stdout._readableState.highWaterMark = 4096;

child.stdout.on('data', function(data) {
	soundMeter.write(data);
});

child.stderr.on('data', function(data) {
	console.log(data.toString());
});