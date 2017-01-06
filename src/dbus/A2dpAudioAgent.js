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
		
	}
}

function stop(self, onComplete) {
	if (self.started) {
		
	}
}

module.exports = A2dpAudioAgent;

