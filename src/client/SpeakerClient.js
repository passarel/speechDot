const net = require('net');
const lame = require('lame');
const wav = require('wav');

function SpeakerClient(audioSource) {
	this.audioSource = audioSource;
}

SpeakerClient.prototype.play = function(port, host, onConnect) {
	if (!this.client) {
		this.client = new net.Socket();
		
		var serverUri =  host + ':' + port;
		this.client.connect(port, host, function() {
			if (onConnect) onConnect();
			
			var reader = new wav.Reader();
			reader.on('format', function(format) {
				console.error('WAV format: %j', format);
				var encoder = new lame.Encoder(format);
				reader.pipe(encoder).pipe(this.client)
			}.bind(this));
			var wavWriter = new wav.Writer();
			this.audioSource.pipe(wavWriter).pipe(reader)
			
			console.log('SpeakerClient connected & playing to: ' + serverUri);
		}.bind(this));
		
		this.client.on('close', () => {
			console.log('SpeakerClient connection closed to: ' + serverUri);
		}.bind(this));
		this.client.on('error', () => {
			console.log('[error] SpeakerClient: ' + err);
		});
	}
}

SpeakerClient.prototype.stop = function() {
	if (this.client) {
		this.client.destroy();
		this.client = undefined;
	}
}

module.exports = SpeakerClient;
