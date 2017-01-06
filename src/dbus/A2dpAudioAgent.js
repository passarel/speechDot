const DBus = require('../../lib/dbus');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(A2dpAudioAgent, EventEmitter);

function A2dpAudioAgent(adapterId, deviceId) {
	var self = this;
	self.adapterId = adapterId;
	self.deviceId = deviceId;
	
	console.log('self.adapterId = ' + self.adapterId);
	console.log('self.deviceId = ' + self.deviceId);
	
	self.start = function(onComplete) {
		start(self, onComplete);
	}
	self.stop = function(onComplete) {
		stop(self, onComplete);
	}
}

function start(self, onComplete) {

	///org/bluez/hci0/dev_40_40_A7_1A_7A_E8
	
	/*
	bus.getInterface('org.bluez', '/org/bluez/' + self.adapter, 'org.bluez.Media1', function(err, iface) {
		if (notErr(err)) {
			console.log(iface);
		}
	});
	*/
	
	/*
	addSignalHandler('org.bluez', device.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function(ifaceName, props) {
		Object.keys(props).forEach(function(name) {
			var val = props[name];
			console.log('[Bluez.Device] PropertyChanged: ' + device.path + ', ' + name + '=' + val);
			device[name] = val;
			if (name === 'Connected' && val) {
				if (self.pairModeAdapter && device.Adapter === self.pairModeAdapter.path) {
					PairingAgent.removeAllListeners('device_trusted');
					console.log('[Bluez.js] device_trusted event handler removed');
					setTimeout(function() {
						setAdapterProperty(self, [device.Adapter], 'Discoverable', false);
					}, 5000);
				}
			}
		});
	});
	*/
}

function stop(self, onComplete) {
	
}

module.exports = A2dpAudioAgent;

