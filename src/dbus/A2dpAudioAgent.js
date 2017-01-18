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

	/*
	if (!self.started) {

		addSignalHandler('org.bluez', self.mediaTransport.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function(ifaceName, props) {
			Object.keys(props).forEach(function(name) {
				const val = props[name];
				self.mediaTransport[name] = val;
				console.log('[Bluez.MediaTransport1] PropertyChanged: ' + self.mediaTransport.path + ', ' + name + '=' + val);
				if (name === 'State') onState(self, val);
			});
		});
		self.started = true;
	}
	*/
	
	//acquireStream(self);
}

function stop(self, onComplete) {
	if (self.started) {

	}
}

/*
function onState(self, val) {
	if (val === 'active') {
		
	}
	if (val === 'idle') {
		
	}
	if (val === 'pending') {
		tryAcquireStream(self);
	}
}
*/

function acquireStream(self, onComplete) {
	bus.getInterface('org.bluez', self.mediaTransport.path, 'org.bluez.MediaTransport1', function(err, iface) {
		if (notErr(err)) {
			

			iface.Acquire['timeout'] = 2000;
			iface.Acquire['finish'] = function(fd, mtuRead, mtuWrite) {
				
				console.log('TryAcquire executed successfully');
				
				console.log();
				console.log('fd: ' + fd);
				console.log('mtuRead: ' + mtuRead);
				console.log('mtuWrite: ' + mtuWrite);
				console.log();
				
				//if (onComplete) onComplete.apply(null, [null]);
			};
			iface.Acquire['error'] = function(err) {
				console.log('[error] Acquire execution failed (' + self.mediaTransport.path + ') - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.Acquire();

			
			console.log(iface);
		}
	});
}

module.exports = A2dpAudioAgent;

