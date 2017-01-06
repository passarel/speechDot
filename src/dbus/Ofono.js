const ProcessManager = require('../ProcessManager.js');
const HandsFreeAudioAgent = require('./HandsFreeAudioAgent.js');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const addPropertyChangedSignalHandler = dbusUtils.addPropertyChangedSignalHandler;
const removeAllHandlersForPropertyChangedSignal = dbusUtils.removeAllHandlersForPropertyChangedSignal;

const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(Ofono, EventEmitter);

function Ofono() {
	
	const self = this;
	
	const addHandsFreeSignalHandler = function(modem, onComplete) {
		removeAllHandlersForPropertyChangedSignal('org.ofono', modem.path, 'org.ofono.Handsfree', function() {
			addPropertyChangedSignalHandler('org.ofono', modem.path, 'org.ofono.Handsfree', function(name, val) {
				console.log('[Handsfree] PropertyChanged: ' + name + ' = ' + val);
				if (name == 'VoiceRecognition') {
					self.emit('voice_recognition', modem, val)
				}
			}, onComplete);
		});
	}
	
	const addModemSignalHandler = function(modem, onComplete) {
		removeAllHandlersForPropertyChangedSignal('org.ofono', modem.path, 'org.ofono.Modem', function() {
			addPropertyChangedSignalHandler('org.ofono', modem.path, 'org.ofono.Modem', function(name, val) {
				console.log('[Modem] PropertyChanged: ' + name + '=' + val);
				if (name == 'Online' && val) {
					addHandsFreeSignalHandler(modem, function() {
						addVoiceCallSignalHandler(modem, function() {
							modem[name] = val;
							self.emit('modem_online', modem);
						});
					});
				} else {
					modem[name] = val;
					if (name == 'Online' && !val) {
						self.emit('modem_offline', modem);
					}
					if (name == 'Interfaces') {
						//modem.Siri = hasSiriInterface(modem);
						modem.Siri = hasInterface(modem, 'org.ofono.Siri');
						self.emit('modem_interfaces_changed', modem);
					}
				}
			}, onComplete);
		});
	}
	
	const addVoiceCallSignalHandler = function(modem, onComplete) {
		const addCallAddedHandler = function(onComplete) {
			removeAllHandlersForSignal('org.ofono', modem.path, 'org.ofono.VoiceCallManager', 'CallAdded', function() {
				addSignalHandler('org.ofono', modem.path, 'org.ofono.VoiceCallManager', 'CallAdded', function(path, call) {
					console.log('VoiceCallManager.CallAdded: ' + path);
					call.path = path;
					self.calls[path] = call;
					addPropertyChangedSignalHandler('org.ofono', path, 'org.ofono.VoiceCall', function(name, val) {
						console.log('[org.ofono.VoiceCall] PropertyChanged: ' + name + '=' + val);
						if (name == 'State') {
							self.emit('call_state_changed', call, val);
						}
					});
					console.log('emitting call added for call -> ' + call.path);
					self.emit('call_added', call, modem);
				}, onComplete);
			});
		}
		const addCallRemovedHandler = function(onComplete) {
			removeAllHandlersForSignal('org.ofono', modem.path, 'org.ofono.VoiceCallManager', 'CallRemoved', function() {
				addSignalHandler('org.ofono', modem.path, 'org.ofono.VoiceCallManager', 'CallRemoved', function(path) {
					console.log('VoiceCallManager.CallRemoved: ' + path);
					self.calls[path] = undefined;
					self.emit('call_removed', path, modem);
					setTimeout(function() {
						removeAllHandlersForPropertyChangedSignal('org.ofono', path, 'org.ofono.VoiceCall');
					}, 1000); //wait a second to remove incase call was hungup immediatly
				}, onComplete)
			});
		}
		if (hasInterface(modem, 'org.ofono.VoiceCallManager')) {
			addCallAddedHandler(addCallRemovedHandler(onComplete));
		} else {
			self.on('modem_interfaces_changed', function(_modem) {
				if (_modem.path === modem.path) {
					addCallAddedHandler(addCallRemovedHandler());
				}
			});
			onComplete();
		}
	}
	
	const addSignalHandlers = function(modem) {
		addModemSignalHandler(modem, function() {
			if (modem.Online) {
				addHandsFreeSignalHandler(modem, function() {
					addVoiceCallSignalHandler(modem);
				});
			}
		});
	}
	
	self.getModems = function(onModems) {
		onModems(self.modems);
	};
	
	self.setVoiceRecognitionOn = function(path, onSucc, onErr) {
		setVoiceRecognition(path, true, onSucc, onErr);
	};
	
	self.setVoiceRecognitionOff = function(path, onSucc, onErr) {
		setVoiceRecognition(path, false, onSucc, onErr);
	};
	
	self.getVoiceRecognition = getVoiceRecognition;
	
	self.setSpeakerVolume = setSpeakerVolume;
	
	self.setMicrophoneVolume = setMicrophoneVolume;
	
	self.setSiriEyesFreeModeOn = setSiriEyesFreeModeOn;
	
	self.answerCall = answerCall;
	
	self.hangupCall = hangupCall;
	
	self.establishAudioConnection = establishAudioConnection;
	
	ProcessManager.on('ofono_online', function(modemsData) {
		self.calls = {};
		self.modems = parseModems(modemsData[0]);
		//console.log('---------------------------------------------');
		//console.log(modemsData)
		//console.log('---------------------------------------------');
		self.modems.forEach(function(modem) {
			addSignalHandlers(modem);
		});
		removeAllHandlersForSignal('org.ofono', '/', 'org.ofono.Manager', 'ModemAdded', function() {
			addSignalHandler('org.ofono', '/', 'org.ofono.Manager', 'ModemAdded', function(path, data) {
				const modem = parseModem(path, data);
				self.modems.push(modem);
				console.log('ModemAdded -> ' + JSON.stringify(modem));
				addSignalHandlers(modem);
			});
		});
		HandsFreeAudioAgent.stop(function() {
			HandsFreeAudioAgent.start(function() {
				console.log('handsfree audio agent started');
			});
		});
	});
}

module.exports = new Ofono();

function establishAudioConnection(bdaddr, onComplete) {
	var card = HandsFreeAudioAgent.getCards()[bdaddr];
	if (card) {
		bus.getInterface('org.ofono', card.path, 'org.ofono.HandsfreeAudioCard', function(err, iface) {
			if (notErr(err)) {
				iface.Connect['timeout'] = 1000;
				iface.Connect['finish'] = function() {
					console.log('[success] HandsFreeAudioAgent.Connect - ' + bdaddr);
					var args = Array.prototype.slice.call(arguments);
					args.splice(0, 0, null); // there is no error
					if (onComplete) onComplete.apply(null, args);
				};
				iface.Connect['error'] = function(err) {
					console.log('[error] HandsFreeAudioAgent.Connect - ' + err);
					if (onComplete) onComplete(err);
				};
				iface.Connect();
			}
		});
	} else {
		if (onComplete) onComplete('card not found');
	}
}

function answerCall(call, onSucc, onErr) {
	if (call.State !== 'incoming') {
		console.log('answerCall failed - call.State is not incoming');
		return;
	}
	bus.getInterface('org.ofono', call.path, 'org.ofono.VoiceCall', function(err, iface) {
		if (notErr(err)) {
			iface.Answer['timeout'] = 2000;
			iface.Answer['finish'] = function() {
				if (onSucc) onSucc();
			};
			iface.Answer['error'] = function(err) {
				console.log(err);
				if (onErr) onErr(err);
			};
			iface.Answer();
		}
	});
}

function hangupCall(call, onSucc, onErr) {
	bus.getInterface('org.ofono', call.path, 'org.ofono.VoiceCall', function(err, iface) {
		if (notErr(err)) {
			iface.Hangup['timeout'] = 2000;
			iface.Hangup['finish'] = function() {
				if (onSucc) onSucc();
			};
			iface.Hangup['error'] = function(err) {
				console.log(err);
				if (onErr) onErr(err);
			};
			iface.Hangup();
		}
	});
}

function setVoiceRecognition(path, val, onSucc, onErr) {
	bus.getInterface('org.ofono', path, 'org.ofono.Handsfree', function(err, iface) {
		if (notErr(err)) {
			iface.SetProperty['timeout'] = 2000;
			iface.SetProperty['finish'] = function() {
				if (onSucc) onSucc();
			};
			iface.SetProperty['error'] = function(err) {
				console.log(err);
				if (onErr) onErr(err);
			};
			iface.SetProperty('VoiceRecognition', val);
		}
	});
}

function setSpeakerVolume(path, val, onComplete) {
	bus.getInterface('org.ofono', path, 'org.ofono.CallVolume', function(err, iface) {
		if (notErr(err)) {
			iface.SetProperty['timeout'] = 2000;
			iface.SetProperty['finish'] = function() {
				if (onComplete) onComplete.apply(null, [null]);
			};
			iface.SetProperty['error'] = function(err) {
				console.log('[error] setSpeakerVolume(' + path + ') - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.SetProperty('SpeakerVolume', val);
		}
	});
}

function setMicrophoneVolume(path, val, onComplete) {
	bus.getInterface('org.ofono', path, 'org.ofono.CallVolume', function(err, iface) {
		if (notErr(err)) {
			iface.SetProperty['timeout'] = 2000;
			iface.SetProperty['finish'] = function() {
				if (onComplete) onComplete.apply(null, [null]);
			};
			iface.SetProperty['error'] = function(err) {
				console.log('[error] setMicrophoneVolume(' + path + ') - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.SetProperty('MicrophoneVolume', val);
		}
	});
}

function setSiriEyesFreeModeOn(path, onSucc, onErr) {
	bus.getInterface('org.ofono', path, 'org.ofono.Siri', function(err, iface) {
		if (notErr(err)) {
			iface.SetProperty['timeout'] = 2000;
			iface.SetProperty['finish'] = function() {
				if (onSucc) onSucc();
			};
			iface.SetProperty['error'] = function(err) {
				console.log(err);
				if (onErr) onErr(err);
			};
			iface.SetProperty('EyesFreeMode', 'enabled');
		}
	});
}

function getVoiceRecognition(modem, onVal, onErr) {
	bus.getInterface('org.ofono', modem.path, 'org.ofono.Handsfree', function(err, iface) {
		if (notErr(err)) {
			iface.GetProperties['timeout'] = 2000;
			iface.GetProperties['finish'] = function(props) {
				Object.keys(props).forEach(function(key) {
					if (key == 'VoiceRecognition') {
						onVal(props[key]);
						return;
					}
				});
			};
			iface.GetProperties['error'] = function(err) {
				console.log(err);
				if (onErr) onErr(err);
			};
			iface.GetProperties();
		}
	});
}

function parseModems(data) {
	if (!data) return [];
	return Object.keys(data).map(function(path) {
		return parseModem(path, data[path]);
	})
}

function parseModem(path, props) {
	const modem = props;
	modem.path = path;
	modem.bdaddr = parseBdaddr(path);
	//modem.Siri = hasSiriInterface(modem);
	modem.Siri = hasInterface(modem, 'org.ofono.Siri');
	return modem;
}

function hasInterface(modem, iface) {
	return modem.Interfaces.includes(iface);
}

/*
function hasSiriInterface(modem) {
	if (modem.Interfaces) {
		for (var i=0; i<modem.Interfaces.length; i++) {
			if (modem.Interfaces[i] == 'org.ofono.Siri') {
				return true;
			}
		}
	}
	return false;
}
*/

function parseBdaddr(path) {
	return path
		.substring(path.indexOf('dev_') + 4)
		.replaceAll('_', ':');
}
