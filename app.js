const spawn = require('child_process').spawn;
const execSync = require('child_process').execSync;
const exec = require('child_process').exec;
const appUtils = require('./src/utils/app_utils.js');
const Ofono = require('./src/dbus/Ofono.js');
const Bluez = require('./src/dbus/Bluez.js');
const path = require('path');
const audio_responses_path = path.resolve(__dirname, 'audio_responses');
const bash_scripts_path = path.resolve(__dirname, 'bash_scripts');
const ProcessManager = require('./src/ProcessManager.js');

var AlexaAvs = require('./alexa/client/AlexaAvs.js');

var activeModem = null;
var incomingCall = null;
var activeCall = null;
var isVoiceRecognitionOn = false;
var voiceRecognitionTimeout = null;
var yesNoConfirmation = null;
var isReady = false;

ProcessManager.on('sensory_online', function(sensory) {
	setLocalSpeakerVolume(25);
	setLocalMicVolume(85);
	addOfonoHandlers();
	sensory.stderr.on('data', onSensoryData);
	playAudioResponse('speech_dot_online.wav');
	isReady = true;
});

ProcessManager.on('sensory_offline', function() {
	Bluez.removeAllListeners('pairing_mode_timeout');
	Ofono.removeAllListeners('voice_recognition');
	Ofono.removeAllListeners('modem_offline');
	Ofono.removeAllListeners('modem_online');
	Ofono.removeAllListeners('call_added');
	Ofono.removeAllListeners('call_removed');
	var isReady = false;
});

function addOfonoHandlers() {
	Ofono.on('voice_recognition', function(modem, val) {
		isVoiceRecognitionOn = val;
		if (isVoiceRecognitionOn) {
			voiceRecognitionTimeout = new Date().getTime() + 14000;
			activeModem = modem;
		} else {
			activeModem = null;
		}
	});
	
	Ofono.on('modem_offline', function(modem) {
		if (activeModem && activeModem.path == modem.path) {
			isVoiceRecognitionOn = false;
			activeModem = null;
		}
		playAudioResponse('phone_disconnected.wav');
		sayThisName(modem.Name);
	});

	Ofono.on('modem_online', function(modem) {
		playAudioResponse('phone_connected.wav');
		sayThisName(modem.Name);
		setTimeout(function() {
			setHfpVolume(modem, 100);
		}, 500);
	});

	Ofono.on('call_added', function(call, modem) {
		if (call.State === 'incoming') {
			incomingCall = call;
			Ofono.removeAllListeners('call_state_changed');
			Ofono.once('call_state_changed', function(_call, state) {
				if (state == 'active' && _call.path == call.path) {
		 			activeCall = incomingCall;
		 			incomingCall = null;
				}
			})
			playAudioResponseAsync('incoming_call_on.wav', function() {
				if (incomingCall == null) return;
				sayThisNameAsync(modem.Name, function() {
					if (incomingCall == null) return;
					playAudioResponseAsync('from.wav', function() {
						if (incomingCall == null) return;
						if (call.Name) {
							sayThisNameAsync(call.Name);
						} else {
							var number = call.LineIdentification.replace('+', '');
							if (number.length === 11) {
								sayThisTextAsync(number.substring(1));
							} else {
								sayThisTextAsync(number);
							}
						}
					});
				});
			});
		}
	});

	Ofono.on('call_removed', function() {
		incomingCall = null;
	});
}

function onSensoryYesNoData(data) {
    var _data = data.toString().trim();
    console.log('sensoryYesNoData -> ' + _data);
    if (yesNoConfirmation) {
		if (_data === 'Yes') {
			yesNoConfirmation.onYes();
		}
		if (_data === 'No') {
			yesNoConfirmation.onNo();
		}
    }
}

function onSensoryData(data) {
	if (!isReady) return;
    var _data = data.toString().trim();
    console.log('sensory -> ' + _data);
   
    if (_data === 'Ok Google') {
 	    console.log('onOkGoogle');
 		onOkGoogle();
 	}
 	if (_data === 'Hey Seary') {
 		console.log('onHeySiri');
 		onHeySiri();
    }
 	if (_data === 'Hey Kor tana') {
 		console.log('onHeyCortana');
 		onHeyCortana();
    }

        if (_data == 'Alexa') {
                console.log('onAlexa');
                onAlexa();
        }
 	
 	// im doing this because jessina pair will can be recognized when u say jessina answer
 	if (incomingCall && (_data === 'Jessina Dismiss')) {
 		Ofono.hangupCall(incomingCall, function() {
 			incomingCall = null;
 		});
 		return;
 	}
 	
 	// im doing this because jessina pair will can be recognized when u say jessina answer
 	if (incomingCall && (_data === 'Jessina Answer')) {
 		Ofono.answerCall(incomingCall, function() {
 			activeCall = incomingCall;
 			incomingCall = null;
 		});
 		return;
 	}
 	
 	// im doing this because jessina pair will can be recognized when u say jessina hangup
 	if (activeCall && (_data === 'Jessina Hang_Up')) {
 		Ofono.hangupCall(activeCall, function() {
 			activeCall = null;
 		});
 		return;
 	}

 	if (!isVoiceRecognitionOn && _data === 'Jessina Reset') {
 		getConfirmation(function() {
	 		Bluez.resetAdapters(function() {
	 			playAudioResponseAsync('reset_completed.wav');
	 		});
 		}, function() {
 			playAudioResponseAsync('reset_canceled.wav');
 		}, 'reset');
 	}
 	
 	if (!isVoiceRecognitionOn && _data === 'Jessina Shutdown') {
 		getConfirmation(function() {
 			playAudioResponse('shutting_down.wav');
 			execSync('shutdown now');
 		}, function() {
 			playAudioResponseAsync('shutdown_canceled.wav');
 		}, 'shutdown');
 	}
 	
 	if (!isVoiceRecognitionOn && _data === 'Jessina Pair') {
 		Bluez.pair(function(err) {
 			if (err) {
 				if (err === 'no available adapter') {
 					playAudioResponseAsync('too_many_phones_connected.wav');
 				}
 				if (err === 'pairing already on') {
 					playAudioResponseAsync('pairing_already_on.wav');
 				}
 			} else {
 				playAudioResponseAsync('pairing_on.wav');
 				Bluez.once('pairing_mode_timeout', function() {
 					playAudioResponseAsync('pairing_off.wav');
 				});
 			}
 		})
 	}
}

function getConfirmation(onYes, onNo, eventName) {
	if (yesNoConfirmation) return;
	playAudioResponseAsync('confirm_' + eventName + '.wav', function() {
		setTimeout(function() {
			if (yesNoConfirmation) {
				yesNoConfirmation.onNo();
			}
		}, 5000);
	});
	var sensoryYesNo = ProcessManager.spawnSensoryYesNo();
	sensoryYesNo.on('exit', function() {
		console.log('[info] sensoryYesNo process has exited.');
	});
	var cleanUp = function() {
		appUtils.killAllProcessesWithNameSync(['yesNo']);
		yesNoConfirmation = null;
	}
	yesNoConfirmation = {
			onYes: function() {
				cleanUp();
				onYes();
			}, 
			onNo: function() {
				cleanUp();
				onNo();
			}, 
			sensoryYesNo: sensoryYesNo};
	//wait about 2 sec before we start acting on user response...
	sensoryYesNo.stderr.on('data', function(data){}); //ignore early data
	setTimeout(function() {
		sensoryYesNo.stderr.removeAllListeners('data');
		sensoryYesNo.stderr.on('data', onSensoryYesNoData);
	}, 2000);
}

function setLocalSpeakerVolume(val) {
	execSync('amixer set Master ' + val + '% -q');
	console.log('Local speaker volume set to ' + val + '%');
}

function setLocalMicVolume(val) {
	execSync('amixer set Capture ' + val + '% -q');
	console.log('Local mic volume set to ' + val + '%');
}

function onHeySiri() {
	Ofono.getModems(function(modems) {
		var modem = findModem(modems, {Siri: true});
		if (modem == null) {
			if (!isVoiceRecognitionOn || new Date().getTime() > voiceRecognitionTimeout) {
				console.log('There is no Siri phone online');
				playAudioResponseAsync('no_siri_phone_online.wav');
			}
		} else {
			tryAcquireVoiceRecognition(modem);
		}
	});
}

function onOkGoogle() {
	Ofono.getModems(function(modems) {
		var modem = findModem(modems, {Google: true});
		if (modem == null) {
			if (!isVoiceRecognitionOn || new Date().getTime() > voiceRecognitionTimeout) {
				console.log('There is no Google phone online');
				playAudioResponseAsync('no_google_phone_online.wav');
			}
		} else {
			tryAcquireVoiceRecognition(modem);
		}
	});
}

function onHeyCortana() {
	Ofono.getModems(function(modems) {
		var modem = findModem(modems, {Cortana: true});
		if (modem == null) {
			if (!isVoiceRecognitionOn || new Date().getTime() > voiceRecognitionTimeout) {
				console.log('There is no Cortana phone online');
				playAudioResponseAsync('no_cortana_phone_online.wav');
			}
		} else {
			tryAcquireVoiceRecognition(modem);
		}
	});
}

function onAlexa() {
	AlexaAvs.requestRegCode(AlexaAvs);
	AlexaAvs.sendRequestToAVS(AlexaAvs);
}

function playAudioResponseAsync(fileName, onComplete) {
	console.log('  playAudioResponseAsync: ' + fileName);
	var cmd = 'aplay ' + audio_responses_path + '/' + fileName;
	return execAsync(cmd, onComplete);
}

function sayThisNameAsync(name, onComplete) {
	name = name.replace('iPhone', 'i phone') + '.';
	name = name.replace('iPad', 'i pad') + '.';
	return sayThisTextAsync(name, onComplete);
}

function sayThisTextAsync(text, onComplete) {
	console.log('  sayThisText: ' + text);
	var cmd = bash_scripts_path + '/text_to_speech.sh ' + '"' + text + '"';
	return execAsync(cmd, onComplete);
}

function execAsync(cmd, onComplete) {
	return exec(cmd, (error, stdout, stderr) => {
		if (error) {
			console.error(`exec error: ${error}`);
		}
		if (onComplete) onComplete();
	});
}

function playAudioResponse(fileName) {
	console.log('  playAudioResponse: ' + fileName);
	execSync('aplay ' + audio_responses_path + '/' + fileName);
}

function sayThisName(name) {
	name = name.replace('iPhone', 'i phone') + '.';
	name = name.replace('iPad', 'i pad') + '.';
	sayThisText(name);
}

function sayThisText(text, onComplete) {
	console.log('  sayThisText: ' + text)
	execSync(bash_scripts_path + '/text_to_speech.sh ' + '"' + text + '"');
}

function findModem(modems, opts) {
	for (var i=0; i<modems.length; i++) {
		if (modems[i].Online) {
			if (opts.Cortana && !modems[i].Siri) {
				if (modems[i].Name.contains('Windows phone') || modems[i].Name.contains('HP Elite x3')) {
					return modems[i];
				}
			}
			if (opts.Siri && modems[i].Siri) {
				return modems[i];
			}
			if (opts.Google && !modems[i].Siri) {
				if (!modems[i].Name.contains('Windows phone')) {
					return modems[i];
				}
			}
		}
	}
	return null;
}

function setHfpVolume(modem, volume, onComplete) {
	Ofono.setSpeakerVolume(modem.path, volume, function(err) {
		if (!err) {
			console.log('Ofono.setSpeakerVolume(' + modem.path + ',' + volume + '%) - SUCCESS');
		}
		Ofono.setMicrophoneVolume(modem.path, volume, function(err) {
			if (!err) {
				console.log('Ofono.setMicrophoneVolume(' + modem.path + ',' + volume + '%) - SUCCESS');
			}
			if (onComplete) onComplete();
		});
	});
}

function tryAcquireVoiceRecognition(modem) {

	var setVoiceRecognitionOn = function() {
		Ofono.setVoiceRecognitionOn(modem.path, function() {
			console.log('setVoiceRecognitionOn - SUCCESS');
		}, function(err) {
			console.log('setVoiceRecognitionOn - ERR: ' + err);
		});
	}
	
	var setVoiceRecognitionOffAndThenOn = function() {
		console.log('setVoiceRecognitionOffAndThenOn...');
		Ofono.setVoiceRecognitionOff(modem.path, function() {
			console.log('setVoiceRecognitionOff - SUCCESS');
			setTimeout(function() {
				setVoiceRecognitionOn();
			}, 500);
		}, function(err) {
			console.log('setVoiceRecognitionOff - ERR: ' + err);
		});
	}
	
	if (isVoiceRecognitionOn) {
		if (!activeModem || !activeModem.Online) {
			isVoiceRecognitionOn = false;
			activeModem = null;
			setVoiceRecognitionOn();
			return;
		}
		if (activeModem && activeModem.path == modem.path) {
			// sometimes its slow to update VoiceRecognition
			// retry in one second.
			setTimeout(function() {
				Ofono.getVoiceRecognition(modem, function(val) {
					console.log('got VoiceRecognition val from modem: ' + val);
					isVoiceRecognitionOn = val;
					if (!isVoiceRecognitionOn) {
						activeModem = null;
						setVoiceRecognitionOn();
					} else {
						if (new Date().getTime() >= voiceRecognitionTimeout) {
							setVoiceRecognitionOffAndThenOn();
						}
					}
				}, function(err) {
					console.log('failed to get VoiceRecognition: ' + err);
				});
			}, 1200);
		} else {
			if (activeModem && new Date().getTime() >= voiceRecognitionTimeout) {
				Ofono.setVoiceRecognitionOff(activeModem.path, setVoiceRecognitionOn);
			}
		}
	} else {
		setVoiceRecognitionOn();
	}
}