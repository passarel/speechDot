require('./string_utils.js');
const spawn = require('child_process').spawn;
const execSync = require('child_process').execSync;
const exec = require('child_process').exec;
const path = require('path');
const audio_responses_path = path.resolve(__dirname, '../../audio_responses');
const bash_scripts_path = path.resolve(__dirname, '../../bash_scripts');

module.exports = {
		
	openMic: (deviceId) => {
		return spawn('arecord', ['-f', 'S16_LE', '-c', '1', '-r', '16000', '-D', deviceId]);
	},
		
	openSpeaker: (deviceId) => {
		return spawn('aplay', ['-f', 'cd', '-D', deviceId]);
	},

	getMicDeviceIds: () => {
		try {
			return parseDeviceIds(execSync('arecord --list-devices').toString());
		} catch (err) {
			console.log('[error] audio_utils.getMicDeviceIds: ' + err);
			return [];
		}
	},
		
	getSpeakerDeviceIds: () => {
		try {
			return parseDeviceIds(execSync('aplay --list-devices').toString());
		} catch (err) {
			console.log('[error] audio_utils.getSpeakerDeviceIds: ' + error);
			return [];
		}
	},
	
	playAudioResponseAsync: playAudioResponseAsync,

	sayThisTextAsync: sayThisTextAsync,
	
	sayThisNameAsync: sayThisNameAsync,

	playAudioResponse: playAudioResponse,

	sayThisText: sayThisText,
	
	sayThisName: sayThisName
	
}

function playAudioResponseAsync(fileName, onComplete) {
	console.log('  playAudioResponseAsync: ' + fileName);
	var cmd = 'aplay ' + audio_responses_path + '/' + fileName;
	return execAsync(cmd, onComplete);
}

function sayThisTextAsync(text, onComplete) {
	console.log('  sayThisText: ' + text);
	var cmd = bash_scripts_path + '/text_to_speech.sh ' + '"' + text + '"';
	return execAsync(cmd, onComplete);
}

function sayThisNameAsync(name, onComplete) {
	name = name.replace('iPhone', 'i phone') + '.';
	name = name.replace('iPad', 'i pad') + '.';
	return sayThisTextAsync(name, onComplete);
}

function playAudioResponse(fileName) {
	console.log('  playAudioResponse: ' + fileName);
	execSync('aplay ' + audio_responses_path + '/' + fileName);
}

function sayThisText(text, onComplete) {
	console.log('  sayThisText: ' + text)
	execSync(bash_scripts_path + '/text_to_speech.sh ' + '"' + text + '"');
}

function sayThisName(name) {
	name = name.replace('iPhone', 'i phone') + '.';
	name = name.replace('iPad', 'i pad') + '.';
	sayThisText(name);
}

function execAsync(cmd, onComplete) {
	return exec(cmd, (error, stdout, stderr) => {
		if (error) {
			console.error(`exec error: ${error}`);
		}
		if (onComplete) onComplete();
	});
}

function parseDeviceIds(data) {
	return data.split('\n').filter((part) => {
		if (part.search(/^card [0-9]:/) === 0) {
			if (part.contains('USB') || 
					(!part.contains('HDMI') &&
							part.contains('bcm2835 ALSA'))) {
				return true;
			}
		}
		return false;
	}).map((part) => {
		var cardNum = part.substring(5, 6);
		var idx = part.search(/device [0-9]:/);
		var deviceNum = part.substring(idx+7, idx+8);
		return 'plughw:' + cardNum + ',' + deviceNum;
	});
}
