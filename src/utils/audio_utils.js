require('./string_utils.js');
const spawn = require('child_process').spawn;
const execSync = require('child_process').execSync;

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
	}
		
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
