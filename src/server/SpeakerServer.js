const tcpUtils = require('../utils/tcp_utils.js');
const audioUtils = require('../utils/audio_utils.js');
const net = require('net');
var lame = require('lame');

function SpeakerServer() {
}

SpeakerServer.prototype.listen = function(port, deviceId) {
	
	if (this.server) return;

	var onConnection = function(client) {
		
		var clientIp = tcpUtils.parseIpAddress(client.remoteAddress);
		console.log('SpeakerServer.Client[' + clientIp + '] connected');
		
		var speaker = audioUtils.openSpeaker(deviceId);
		
		speaker.stderr.on('data', (data) => {
			console.log('SpeakerServer.Speaker.Stderr: ' + data);
		});
		
		speaker.stdout.on('data', (data) => {
			console.log('SpeakerServer.Speaker.Stdout: ' + data);
		});

		client.pipe(new lame.Decoder)
		  .on('format', console.log)
		  .pipe(speaker.stdin);
		
		
		client.on('close', () => {
			console.log('SpeakerServer.Client[' + clientIp + '] disconnected');
			speaker.kill();
		});

		client.on('error', (err) => {
			console.log('[error] SpeakerServer.Client[' + clientIp + ']: ' + err);
		});
	}
	
	this.server = net.createServer(onConnection);
	this.server.maxConnections = 1;
	
	this.server.on('error', (err) => {
		console.log('[error] SpeakerServer: ' + err);
	});
	
	this.server.on('close', (err) => {
		console.log('SpeakerServer closed');
	})
	
	this.server.listen(port, () => {
		console.log('SpeakerServer listening on port: ' + port);
	});
}

SpeakerServer.prototype.close = function() {
	if (!this.server) return;
	this.server.close();
	this.server = undefined;
}

module.exports = () => {
	return new SpeakerServer();
};
