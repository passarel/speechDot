const DBus = require('../../lib/dbus');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(A2dpAudioAgent, EventEmitter);

function A2dpAudioAgent(mediaTransport) {
	const self = this;
	self.mediaTransport = mediaTransport;
	self.started = false;
	self.start = function(onComplete) {
		start(self, onComplete);
	}
	self.stop = function(onComplete) {
		stop(self, onComplete);
	}
}

function start(self, onComplete) {
	if (!self.started) {
		bus.getInterface('org.bluez', self.mediaTransport.path, 'org.bluez.MediaTransport1', function(err, iface) {
			if (notErr(err)) {
				console.log();
				console.log();
				console.log(iface);
				/*
				iface.Connect['timeout'] = 2000;
				iface.Connect['finish'] = function() {
					if (onComplete) onComplete.apply(null, [null]);
				};
				iface.Connect['error'] = function(err) {
					console.log('[error] connect device(' + device.path + ') - ' + err);
					if (onComplete) onComplete(err);
				};
				iface.Connect();
				*/
			}
		});
		self.started = true;
	}
}

function stop(self, onComplete) {
	if (self.started) {

	}
}

module.exports = A2dpAudioAgent;

