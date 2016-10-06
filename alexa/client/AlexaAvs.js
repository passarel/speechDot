var config = require('./config.js');
var https = require('https');
var http = require('http');
var request = require('request');
var fs = require('fs');
var player = require('play-sound')(opts = {});

const spawn = require('child_process').spawn;

const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(AlexaAvs, EventEmitter);

const certs_path = require('path').resolve(__dirname, './alexa/client/certs');
const audio_ask_file = require('path').resolve(__dirname, './alexa/client/ask.wav');
const audio_resp_file = require('path').resolve(__dirname, './alexa/client/resp.mp3');

module.exports = new AlexaAvs();

function AlexaAvs() {
    var self = this;

    self.regCode = null;
    self.sessionId = null;
    self.accessToken = null;
    self.doSendRequest = false;
    self.isRecording = false;
    self.isPlaying = false;
    self.atokenRefresher = null;

    self.requestRegCode = requestRegCode;
    self.requestAccessToken = requestAccessToken;
    self.sendRequestToAVS = sendRequestToAVS;

    self.on('access_token_ready', function (s) {
	onAccessTokenReady(s);
    });
    self.on('audio_rec_ready', function (s) {
	onAudioRecReady(s);
    });
}

function requestRegCode(self) {
    if (self.sessionId !== null) {
        console.log('user has already been authenticated');
	return;
    }

    var options = {
	host: 'localhost',
	port: 3000,
	path: '/provision/regCode?' + 'productId=' + config.productId + '&dsn=' + config.dsn,
	method: 'GET',
	key: fs.readFileSync(certs_path + '/client/client.key'),
	passphrase: '',
	cert: fs.readFileSync(certs_path + '/client/client.crt'),
	ca: fs.readFileSync(certs_path + '/ca/ca.crt'),
	rejectUnauthorized: false
    };

    var req = https.request(options, function (res) {
	var buffer = null;

	res.on('end', function () {
	    if (res.statusCode === 200 && buffer !== null) {
		var result = JSON.parse(buffer);
		self.regCode = result['regCode'];
		self.sessionId = result['sessionId'];
		console.log('regCode: ' + self.regCode);
		console.log('sessionId: ' + self.sessionId);
		console.log('*** register your device by visiting the following URL: ' + config.serviceUrl + '/provision/' + self.regCode);
	    } else {
		console.log('error - status code: ' + res.statusCode);
	    }
	});
	res.on('data', function (data) {
	    if (res.statusCode == 200 && buffer === null) {
		buffer = data;
	    }
	});
    });

    req.on('error', function (e) {
	console.log('error: ' + e);
    });

    req.end();
}

function requestAccessToken(self) {
    if (self.sessionId === null) {
	console.log('no sessionId, so impossible to request new access token');
	return;
    }
    console.log('sessionId: ' + self.sessionId);

    if (self.accessToken !== null) {
	console.log('access token is still available');
	self.emit('access_token_ready', self);
	return;
    }

    if (self.atokenRefresher !== null) {
	clearTimeout(self.atokenRefresher);
	self.atokenRefresher = null;
    }

    var options = {
	host: 'localhost',
	port: 3000,
	path: '/provision/accessToken?' + 'sessionId=' + self.sessionId,
	method: 'GET',
	key: fs.readFileSync(certs_path + '/client/client.key'),
	passphrase: '',
	cert: fs.readFileSync(certs_path + '/client/client.crt'),
	ca: fs.readFileSync(certs_path + '/ca/ca.crt'),
	rejectUnauthorized: false
    };

    var req = https.request(options, function (res) {
	var buffer = null;

	res.on('end', function () {
	    if (res.statusCode === 200 && buffer !== null) {
		var result = JSON.parse(buffer);
		self.accessToken = result['access_token'];
		console.log('access_token: ' + self.accessToken);
		self.expiresIn = result['expires_in'];
		console.log('expires_in: ' + self.expiresIn);

		var timeout = (self.expiresIn - 4 * 60) * 1000;
		
		self.atokenRefresher = setTimeout(function () {
		    console.log('*** access token has been expired ***');
		    self.accessToken = null;
		    self.requestAccessToken(self);
		}, timeout);
		self.emit('access_token_ready', self);
	    } else {
		console.log('error - status code: ' + res.statusCode);
	    }
	});
	res.on('data', function (data) {
	    if (res.statusCode == 200 && buffer === null) {
		buffer = data;
	    }
	});
    });

    req.on('error', function (e) {
	console.log('error: ' + e);
    });

    req.end();
}

function sendRequestToAVS(self) {
    console.log('set doSendRequest to TRUE');
    self.doSendRequest = true;
    self.requestAccessToken(self);
}

function playAudioResponse(self, data) {
    var s = data.toString();
    var i = s.indexOf('Content-Type: audio/mpeg');
    var k;
    console.log('playAudioResponse(): in');
    if (i <= -1) {
	console.log('unable to parse response: ');
	console.log(s);
	console.log('end of response');
	return;
    }

    console.log('found a mpeg file in response');
    i = s.indexOf('\r\n', i);
    if (i <= -1) {
	console.log('it seems to be an invalid response :-(');
	return;
    }

    i = i + 4; /* skip CRLF chars */
    console.log('multipart msg separator: ' + /--[a-zA-Z0-9]+.*--/m.exec(s));
    k = /--[a-zA-Z0-9]+.*--/m.exec(s).index - 2;
 
    var stream = fs.createWriteStream(audio_resp_file);
    stream.write(data);
    stream.end();
    stream.on('finish', function () {
	console.log('saved audio resp file to underlying fs');
	console.log('*** PLAYING RESPONSE ***');
	self.isPlaying = true;
	var play = player.play(audio_resp_file, function (err) {
	    if (err) {
		throw err;
	    }
	    console.log('*** END OF RESPONSE ***');
	    self.isPlaying = false;
	});
    });
}

function startRecording(self) {
    console.log('*** RECORDING ***');
    self.isRecording = true;
    const child = spawn('rec',
			[
			    audio_ask_file,
			    'rate', '16k',
			    'silence', '1', '0.1', '3%', '1', '3.0', '3%'
			]);

    child.on('close', function (c) {
	self.isRecording = false;
	console.log('*** END OF RECORDING ***');
	console.log('saved recorded audio in ' + audio_ask_file);
	self.emit('audio_rec_ready', self);
	console.log('err: ' + c);
    });
}

function onAudioRecReady(self) {
    var reqUrl = 'https://access-alexa-na.amazon.com/v1/avs/speechrecognizer/recognize';

    const child = spawn('curl',
			['-i',
			 '-H',
			 'Expect:',
			 '-H',
			 'Authorization: Bearer ' + self.accessToken,
			 '-F',
			 '"metadata=<alexa/client/metadata.json;type=application/json; charset=UTF-8"',
			 '-F',
			 '"audio=<alexa/client/ask.wav;type=audio/L16; rate=16000; channels=1"',
			 reqUrl
			]);

    var buffer = new Buffer('');

    child.stdout.on('data', function (data) {
	buffer = Buffer.concat([buffer, data]);
    });
    child.on('close', function (c) {
	console.log('*** request sent ***');
	console.log('code: ' + c);
	playAudioResponse(self, buffer);
    });
}

function onAccessTokenReady(self) {
    if (self.doSendRequest === false) {
	console.log('no request to send');
	return;
    }
    console.log('about to send request');
    console.log('set doSendRequest to FALSE');
    self.doSendRequest = false;

    startRecording(self);
}
