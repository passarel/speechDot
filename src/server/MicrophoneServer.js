const tcpUtils = require('../utils/tcp_utils.js');
const audioUtils = require('../utils/audio_utils.js');
const net = require('net');

function MicrophoneServer() {
}

MicrophoneServer.prototype.listen = function(port, deviceId) {
	
	if (this.server) return;
	
	var onConnection = function(client) {
		
		var clientIp = tcpUtils.parseIpAddress(client.remoteAddress);
		console.log('MicrophoneServer.Client[' + clientIp + '] connected');

		var mic = audioUtils.openMic(deviceId);
		
		mic.stderr.on('data', (data) => {
			console.log('MicrophoneServer.Mic.Stderr: ' + data);
		});

		client.on('close', ()=> {
			console.log('MicrophoneServer.Client[' + clientIp + '] disconnected');
			mic.kill();
		});

		client.on('error', (err) => {
			console.log('[error] MicrophoneServer.Client[' + clientIp + ']: ' + err);
		});
		
		//mic.stdout.pipe(client);
		
	}
	
	this.server = net.createServer(onConnection);
	this.server.maxConnections = 1;
	
	this.server.on('error', (err) => {
		console.log('[error] MicrophoneServer: ' + err);
	});
	
	this.server.on('close', (err) => {
		console.log('MicrophoneServer closed');
	})
	
	this.server.listen(port, () => {
		console.log('MicrophoneServer listening on port: ' + port);
	});
}

MicrophoneServer.prototype.close = function() {
	if (!this.server) return;
	this.server.close();
	this.server = undefined;
}

module.exports = () => {
	return new MicrophoneServer();
};

