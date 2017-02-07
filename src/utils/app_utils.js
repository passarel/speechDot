require('./string_utils.js');
const exec = require('child_process').exec;
const execSync = require('child_process').execSync;
const bluez_connect_path = '/home/pi/bluez-5.40/test/test-connect';
const voice_recognition_path = '/home/pi/ofono-1.18/test/test';
const speaker_volume_path = '/home/pi/ofono-1.18/test/set-speaker-volume'
const mic_volume_path = '/home/pi/ofono-1.18/test/set-mic-volume'
const list_modems_path = '/home/pi/ofono-1.18/test/list-modems'

const path = require('path');
const python_scripts_path = path.resolve(__dirname, '../../python_scripts');
	
module.exports = {
		
		hasPulseAudioCardForDevice: hasPulseAudioCardForDevice,
		
		killAllProcessesWithNameSync: killAllProcessesWithNameSync,
		
		shutdownNow: function() {
			execSync('shutdown now');
		}
	}
	
function hasPulseAudioCardForDevice(bdaddr, onTrue) {
	console.log('Checking pulseaudio cards for device: ' + bdaddr);
	exec('sudo pactl list cards short', (error, stdout, stderr) => {
		  if (error) {
		    console.error(`checkPulseAudioSources.exec error: ${error}`);
		    return;
		  }
		  if (stdout.length > 0)
			  console.log(`checkPulseAudioSources.stdout: ${stdout}`);
		  if (stderr.length > 0)
			  console.log(`checkPulseAudioSources.stderr: ${stderr}`);
		  
		  if (stdout.indexOf(bdaddr.replaceAll(':', '_')) != -1) {
			  if (onTrue) onTrue();
		  }
		});
}

function killAllProcessesWithNameSync(names) {
	names.forEach(function(name) {
		try {
			var result = execSync('ps -A | grep ' + name).toString();
			result.split('\n').forEach(function(line) {
				line = line.trim();
				var pid = line.substring(0, line.indexOf(' '));
				if (pid.length > 0) {
					console.log('killing -9 ' + name + ', pid: ' + pid);
					execSync('kill -9 ' + pid);
				}
			})
		} catch (err) {
			//here if no processes with name
		}
	});
}

