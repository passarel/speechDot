const DBus = require('../../lib/dbus');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(A2dpAudioHandler, EventEmitter);

function A2dpAudioHandler() {
	const self = this;
	self.add = function(mediaTransport, onComplete) {
		add(self, mediaTransport, onComplete);
	}
}

function add(self, mediaTransportPath, onComplete) {
	remove(self, mediaTransportPath, function() {
		addSignalHandler('org.bluez', mediaTransportPath, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function(ifaceName, props) {
			Object.keys(props).forEach(function(name) {
				const val = props[name];
				console.log('[Bluez.MediaTransport1] PropertyChanged: ' + mediaTransportPath + ', ' + name + '=' + val);
				if (name === 'State' && val === 'pending') {
					acquire(mediaTransportPath);
				}
			});
			if (onComplete) onComplete();
		});
	});
}

function acquire(mediaTransportPath) {
	bus.getInterface('org.bluez', mediaTransportPath, 'org.bluez.MediaTransport1', function(err, iface) {
		if (notErr(err)) {
			//console.log(iface);
			iface.Acquire['timeout'] = 2000;
			iface.Acquire['finish'] = function(args) {
				console.log();
				console.log('--- Acquire results ---')
				console.log('fd = ' + args[0]);
				console.log('mtuRead = ' + args[1]);
				console.log('mtuWrite = ' + args[2]);
				
				//if (onComplete) onComplete.apply(null, [null]);
			};
			iface.Acquire['error'] = function(err) {
				console.log('[error] connect device(' + device.path + ') - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.Acquire();
		}
	});
}

function remove(self, mediaTransportPath, onComplete) {
	removeAllHandlersForSignal('org.bluez', mediaTransportPath, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', onComplete);
}

module.exports = new A2dpAudioHandler();
