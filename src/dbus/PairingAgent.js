const DBus = require('../../lib/dbus');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(PairingAgent, EventEmitter);

function PairingAgent() {
	var self = this;
	self.agent = null;
	self.object_path = '/pairing/agent';
	self.service = dbusUtils.dbus.registerService('system', 'pairing.agent');
	self.start = function(onComplete) {
		registerAgent(self, function(err) {
			if (notErr(err)) {
				setTimeout(function() {
					// delay to give enuff time for agent to get registered
					requestDefaultAgent(self, onComplete);
				}, 100);
			} else {
				onComplete(err);
			}
		});
	}
	self.stop = function(onComplete) {
		unregisterAgent(self, function(err) {
			if (notErr(err)) {
				// delay to give enuff time for agent to get unregistered
				setTimeout(onComplete(), 100);
			} else {
				onComplete(err);
			}
		});
	}
	self.setDeviceTrusted = setDeviceTrusted;
}

function setDeviceTrusted(path, val, onComplete) {
	console.log('setting device trusted(' + val + '): ' + path);
	bus.getInterface('org.bluez', path, 'org.bluez.Device1', function(err, iface) {
		if (notErr(err)) {
			iface.setProperty('Trusted', val, function(err) {
				if (err) console.log('[error] setProperty org.bluez.Device1.Trusted - ' + err);
				if (onComplete) onComplete();
			});
		}
	});
}

function requestDefaultAgent(self, onComplete) {
	bus.getInterface('org.bluez', '/org/bluez', 'org.bluez.AgentManager1', function(err, iface) {
		if (notErr(err)) {
			iface.RequestDefaultAgent['timeout'] = 1000;
			iface.RequestDefaultAgent['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); //there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.RequestDefaultAgent['error'] = function(err) {
				console.log('[error] PairingAgent.requestDefaultAgent - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.RequestDefaultAgent(self.object_path);
		}
	});
}

function unregisterAgent(self, onComplete) {
	bus.getInterface('org.bluez', '/org/bluez', 'org.bluez.AgentManager1', function(err, iface) {
		if (notErr(err)) {
			iface.UnregisterAgent['timeout'] = 1000;
			iface.UnregisterAgent['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); //there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.UnregisterAgent['error'] = function(err) {
				console.log('[error] PairingAgent.unregisterAgent - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.UnregisterAgent(self.object_path);
		}
	});
}

function registerAgent(self, onComplete) {
	
	if (!self.agent) {
		var obj = self.service.createObject(self.object_path);
		var iface = obj.createInterface('org.bluez.Agent1');
		self.agent = {obj: obj, iface: iface};
		iface.addMethod('Release', {}, function(callback) {
			console.log('>> PairingAgent.Release called');
			callback();
		});
		/*
		iface.addMethod('RequestPinCode', { in: [ DBus.Define(Object) ], out: DBus.Define(String) }, function(device, callback) {
			console.log('>> RequestPinCode called');
			callback('abcdefg');
		});
		
		iface.addMethod('DisplayPinCode', { in: [ DBus.Define(Object), DBus.Define(String) ] }, function(device, pincode, callback) {
			console.log('>> RequestPinCode called');
			callback();
		});
		
		iface.addMethod('RequestPasskey', { in: [ DBus.Define(Object) ], out: DBus.Define(Number) }, function(device, callback) {
			console.log('>> RequestPasskey called');
			callback(123456);
		});
		
		iface.addMethod('DisplayPasskey', { in: [ DBus.Define(Object), DBus.Define(Number), DBus.Define(Number) ] }, function(device, passkey, entered, callback) {
			console.log('>> DisplayPasskey called');
			callback();
		});
		
		iface.addMethod('RequestConfirmation', { in: [ DBus.Define(Object), DBus.Define(Number) ] }, function(device, passkey, callback) {
			console.log('>> RequestConfirmation called');
			callback();
		});
		
		iface.addMethod('RequestAuthorization', { in: [ DBus.Define(Object) ] }, function(device, callback) {
			console.log('>> RequestAuthorization called');
			callback();
		});
		*/
		iface.addMethod('AuthorizeService', { in: [ DBus.Define(Object), DBus.Define(String) ] }, function(device, uuid, callback) {
			console.log('>> PairingAgent.AuthorizeService called');
			setDeviceTrusted(device, true, function() {
				console.log('PairingAgent emitting device_trusted');
				self.emit('device_trusted', device);
			});
			callback();
		});
		iface.addMethod('Cancel', {}, function(callback) {
			console.log('>> PairingAgent.Cancel called');
			callback();
		});
		iface.update();
	}

	bus.getInterface('org.bluez', '/org/bluez', 'org.bluez.AgentManager1', function(err, iface) {
		if (notErr(err)) {
			iface.RegisterAgent['timeout'] = 1000;
			iface.RegisterAgent['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); //there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.RegisterAgent['error'] = function(err) {
				console.log('[error] PairingAgent.registerAgent - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.RegisterAgent(self.object_path, 'NoInputNoOutput');
		}
	});
}

module.exports = new PairingAgent();

