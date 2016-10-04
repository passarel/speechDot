const net = require('net');

function MicrophoneClient(audioSink) {
	this.audioSink = audioSink;
}

MicrophoneClient.prototype.record = function(port, host, onConnect) {
	if (!this.client) {
		this.client = new net.Socket();
		var serverUri =  'tcp://' + host + ':' + port;
		this.client.connect(port, host, function() {
			if (onConnect) onConnect();
			
			//this.client.pipe(this.audioSink);
			
			console.log('MicrophoneClient connected & recording from: ' + serverUri);
		}.bind(this));
		this.client.on('close', function() {
			console.log('MicrophoneClient connection closed to: ' + serverUri);
		});
		this.client.on('error', () => {
			console.log('[error] MicrophoneClient: ' + err);
		});
	}
}

MicrophoneClient.prototype.stop = function() {
	if (this.client) {
		this.client.destroy();
		this.client = undefined;
	}
}

module.exports = MicrophoneClient;
