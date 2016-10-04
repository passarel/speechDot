const audioUtils = require('../utils/audio_utils.js');
const speakerServer = require('../server/SpeakerServer.js')();
const micServer = require('../server/MicrophoneServer.js')();

function ControlClient() {
}

ControlClient.prototype.connect = function(port, host, onConnect) {
	if (this.socket) return;
	
	var speakerDevice = audioUtils.getSpeakerDeviceIds()[0];
	console.log('ControlClient speakerDevice -> ' + speakerDevice);
	speakerServer.listen(port+1, speakerDevice);
	
	//var micDevice = audioUtils.getMicDeviceIds()[0];
	//console.log('ControlClient micDevice -> ' + micDevice);
	//micServer.listen(port+2, micDevice);
	
	var serverUri = 'http://' + host + ':' + port;
	this.socket = require('socket.io-client')(serverUri);
	this.socket.on('disconnect', () => {
		console.log('ControlClient disconnected from ' + serverUri);
	});
	this.socket.on('connect', () => {
		console.log('ControlClient connected to ' + serverUri);
		if (onConnect) onConnect(this.socket);
	}.bind(this));
}

ControlClient.prototype.disconnect = function() {
	if (!this.socket) return;
	this.socket.disconnect()
	this.socket = undefined;
	speakerServer.close();
	micServer.close();
}

module.exports = ControlClient;

