var config = require('./config.js');
var https = require('https');
var http = require('http');
var fs = require('fs');
var player = require('play-sound')(opts = {});
var lame = require('lame');
var wav = require('wav');

const spawn = require('child_process').spawn;
const execSync = require('child_process').execSync;

const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(AlexaAvs, EventEmitter);

const certs_path = require('path').resolve(__dirname, './certs');
const metadata_file = require('path').resolve(__dirname, './metadata.json');
const audio_ask_file = require('path').resolve(__dirname, './ask.wav');
const mp3_resp_file = require('path').resolve(__dirname, './resp.mp3');
const wav_resp_file = require('path').resolve(__dirname, './resp.wav');
const start_rec_file = require('path').resolve(__dirname, './start_recording.wav');
const stop_rec_file = require('path').resolve(__dirname, './stop_recording.wav');

const ipAddr = execSync('ip route get 1 | awk \'{print $NF;exit}\'').toString().trim();
const serviceUrl = 'https://' + ipAddr + ':' + config.servicePort;

module.exports = new AlexaAvs();

function AlexaAvs() {
    var self = this;

    self.regCode = null;
    self.sessionId = null;
    self.accessToken = null;
    self.doSendRequest = false;
    self.processingRequest = false;
    self.atokenRefresher = null;

    self.requestRegCode = requestRegCode;
    self.requestedRegCode = false;
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
    if (self.sessionId !== null && self.accessToken !== null) {
        console.log('user has already been authenticated');
	return;
    }

    /* request authentication only once */
    if (self.requestedRegCode === true) {
	console.log('*** register your device by visiting the following URL: ' + serviceUrl + '/provision/' + self.regCode);
	return;
    }

    var options = {
	host: 'localhost',
	port: config.servicePort,
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
		console.log('*** register your device by visiting the following URL: ' + serviceUrl + '/provision/' + self.regCode);
		self.requestedRegCode = true;
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
	port: config.servicePort,
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
	self.processingRequest = false;
	return;
    }

    console.log('found a mpeg file in response');
    i = s.indexOf('\r\n', i);
    if (i <= -1) {
	console.log('it seems to be an invalid response :-(');
	self.processingRequest = false;
	return;
    }

    i = i + 4; /* skip CRLF chars */
    console.log('multipart msg separator: ' + /--[a-zA-Z0-9]+.*--/m.exec(s));
    k = /--[a-zA-Z0-9]+.*--/m.exec(s).index - 2;
 
    var input = fs.createWriteStream(mp3_resp_file);
    input.write(data);
    input.end();
    input.on('finish', function () {
	console.log('saved audio resp file to underlying fs');
	console.log('encoding resp audio to WAV format');

	var inStream = fs.createReadStream(mp3_resp_file);

	const child = spawn('aplay', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw', '--buffer-size', '4096']);
	child.stdin._writableState.highWaterMark = 8192;

	child.on('close', function (err) {
	    console.log('*** end of response ***');
	    self.processingRequest = false;
	});

	var decoder = new lame.Decoder();

	inStream.pipe(decoder);

	decoder.on('format', function (format) {
	    console.log('*** alexa\'s speaking ***');
	    var writer = new wav.Writer(format);
	    decoder.pipe(writer).pipe(child.stdin);
	});
    });
}

function startRecording(self) {
    console.log('*** recording ***');

    execSync('aplay ' + start_rec_file + ' 2>&1 >/dev/null');

    var output = fs.createWriteStream(audio_ask_file);
    const child = spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-t', 'raw', '--buffer-size', '512']);
    child.stdout._readableState.highWaterMark = 1024;

    child.on('exit', function (c) {
	console.log('err: ' + c);
	if (c !== 0 && c !== 1) {
	    throw c;
	}
    });

    child.stdout.pipe(output).on('finish', function () {
	console.log('*** end of recording ***');
	execSync('aplay ' + stop_rec_file + ' 2>&1 >/dev/null');
	self.emit('audio_rec_ready', self);
    });

    /*
     * FIXME: find out a better way to detect silence in arecord and then remove me please
     */
    const recordingInMillisecs = 6 * 1000;
    setTimeout(function () {
	child.kill(); /* gracefully terminate arecord by sending out SIGTERM */
    }, recordingInMillisecs);
}

function onAudioRecReady(self) {
    var reqUrl = 'https://access-alexa-na.amazon.com/v1/avs/speechrecognizer/recognize';

    const child = spawn('curl',
			['-s',
			 '-i',
			 '-H',
			 'Expect:',
			 '-H',
			 'Authorization: Bearer ' + self.accessToken,
			 '-F',
			 '"metadata=@' + metadata_file + ';type=application/json;charset=UTF-8"',
			 '-F',
			 '"audio=@' + audio_ask_file + ';type=audio/L16;rate=16000;channels=1"',
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

    self.processingRequest = true;
    startRecording(self);
}
