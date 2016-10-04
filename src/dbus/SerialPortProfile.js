const DBus = require('../../lib/dbus');
const fcntl = require('../../lib/fcntl');

const fs = require('fs');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(SerialPortProfile, EventEmitter);

function SerialPortProfile() {
	var self = this;
	self.object_path = '/profile/serialport';
	self.uuid = '00001101-0000-1000-8000-00805f9b34fb';
	self.options = {
		Name: 'Serial Port',
		Role: 'server'
	};
	self.service = dbusUtils.dbus.registerService('system', 'serialport.profile');

	self.start = function(onComplete) {
		registerProfile(self, function(err) {
			if (notErr(err)) {
				// delay to give enuff time for profile to get registered
				setTimeout(onComplete, 100);
			} else {
				onComplete(err);
			}
		});
	}
	
	self.stop = function(onComplete) {
		unregisterProfile(self, function(err) {
			if (notErr(err)) {
				// delay to give enuff time for profile to get unregistered
				setTimeout(onComplete(), 100);
			} else {
				onComplete(err);
			}
		});
	}
}

function unregisterProfile(self, onComplete) {
	bus.getInterface('org.bluez', '/org/bluez', 'org.bluez.ProfileManager1', function(err, iface) {
		if (notErr(err)) {
			iface.UnregisterProfile['timeout'] = 1000;
			iface.UnregisterProfile['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.UnregisterProfile['error'] = function(err) {
				console.log('[error] SerialPortProfile.unregisterProfile - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.UnregisterProfile(self.object_path);
		}
	});
}

function registerProfile(self, onComplete) {
	if (!self.profile) {
		var obj = self.service.createObject(self.object_path);
		var iface = obj.createInterface('org.bluez.Profile1');
		self.profile = {obj: obj, iface: iface};
		iface.addMethod('Release', {}, function(callback) {
			console.log('>> SerialPortProfile.Release called');
			callback();
		});
		iface.addMethod('RequestDisconnection', {in: [ DBus.Define(Object) ]}, function(device, callback) {
			console.log('>>> SerialPortProfile.RequestDisconnection called');
			console.log('    device: ' + device);
			self.emit('request_disconnection', device);
			callback();
			callback();
		});
		iface.addMethod('NewConnection', {in: [ DBus.Define(Object), DBus.Define('File'), DBus.Define(Object) ]}, function(device, fd, props, callback) {
			console.log('>>> Profile.NewConnection called');
			console.log('    device: ' + device);
			console.log('    fd: ' + fd);
			console.log('    props: ' + JSON.stringify(props));
			process.nextTick(function() {
				fcntl.makeBlocking(fd);
				var readStream = fs.createReadStream(null, {fd: fd});
				readStream.on('data', function(data) {
					console.log('received data: ' + data.toString());
				});
				readStream.on('error', function(err) {
					console.log('[error] SerialPortProfile.readStream - ' + err);
				});
			});
			callback();
		});
		iface.update();
	}

	bus.getInterface('org.bluez', '/org/bluez', 'org.bluez.ProfileManager1', function(err, iface) {
		if (notErr(err)) {
			iface.RegisterProfile['timeout'] = 1000;
			iface.RegisterProfile['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.RegisterProfile['error'] = function(err) {
				console.log('[error] SerialPortProfile.registerProfile - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.RegisterProfile(self.object_path, self.uuid, self.options);
		}
	});
}

module.exports = new SerialPortProfile();
