const fs = require('fs');
const tcpUtils = require('../utils/tcp_utils.js');
const appUtils = require('../utils/app_utils.js');
const ModemManager = require('../ModemManager.js');
const SpeakerClient = require('../client/SpeakerClient.js');
const MicrophoneClient = require('../client/MicrophoneClient.js');

var speaker = new SpeakerClient( fs.createReadStream('/tmp/pulse.output') );
//var mic = new MicrophoneClient( fs.createWriteStream('/tmp/pulse.input') );

function ControlServer() {
}

ControlServer.prototype.listen = function(port) {
	
	if (this.io) return;

	this.io = require('socket.io')(port);

	this.io.on('connection', (client) => {
		
		var clientIp = tcpUtils.parseIpAddress(client.handshake.address);
		console.log('ControlServer.Client[' + clientIp + '] connected');
		
		speaker.play(port+1, clientIp);
		//mic.record(port+2, clientIp);
		
		client.on('voiceRecognition', (bdaddr) => {

			ModemManager.tryAcquireVoiceRecognition(bdaddr, () => {
				ModemManager.once('voiceRecognitionReleased', (bdaddr) => {
					console.log('ControlServer voiceRecognitionReleased');
				});
			},
			(err) => {
				client.emit('voiceRecognitionError', err);
				console.log('ControlServer voiceRecognitionError: ' + err);
			});
			
			/*
			console.log('ControlServer received event [voiceRecognition] for ' + bdaddr);
			
			var onSpeakerAndMicConnected = () => {
				
				console.log('onSpeakerAndMicConnected');
				
				ModemManager.tryAcquireVoiceRecognition(bdaddr, () => {
					ModemManager.once('voiceRecognitionReleased', (bdaddr) => {
						console.log('voiceRecognitionReleased');
						speaker.stop();
						mic.stop();
					});
				},
				(err) => {
					client.emit('voiceRecognitionError', err);
					speaker.stop();
					mic.stop();
				});
			}

			var isSpeakerConnected = false;
			var isMicConnected = false;
			
			speaker.play(port+1, clientIp, () => {
				console.log('isSpeakerConnected = true');
				isSpeakerConnected = true;
				if (isMicConnected) onSpeakerAndMicConnected();
				
			});
			
			mic.record(port+2, clientIp, () => {
				console.log('isMicConnected = true');
				isMicConnected = true;
				if (isSpeakerConnected) onSpeakerAndMicConnected();
			});
			*/
		
		});
		
		client.on('listModems', () => {
			
			console.log('ControlServer received event [listModems]');
			
			appUtils.listModems(function(modems) {
				client.emit('listModemsResponse', modems);
			})
		});
			
	});
	
	ModemManager.on('modemOnline', function(modem) {
		this.io.emit('modemOnline', modem);
	}.bind(this));
	
	ModemManager.on('modemOffline', function(bdaddr) {
		this.io.emit('modemOffline', bdaddr);
	}.bind(this));

	console.log('ControlServer started on port: ' + port);
}

ControlServer.prototype.close = function(port) {
	if (!this.io) return;
	this.io.close();
	this.io = undefined;
}

module.exports = function() {
	return new ControlServer();
};
