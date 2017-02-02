const DBus = require('../../lib/dbus');

//const spawn = require('child_process').spawn;

//const a2dp = require('../../lib/bluetooth').a2dp;

const A2dpAudioHandler = require('./A2dpAudioHandler.js');

const appUtils = require('../utils/app_utils.js');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(A2dpSinkEndpoint, EventEmitter);

// JointStereo 44.1Khz Subbands: Blocks: 16 Bitpool Range: 2-32
const sbc_config = [255, 255, 2, 64];

function A2dpSinkEndpoint() {
	var self = this;
	self.object_path = '/MediaEndpoint/A2DPSink';
	self.service = dbusUtils.dbus.registerService('system', 'mediaendpoint.a2dpsink');	
	self.endpoint_props = {};
	self.endpoint_props.UUID = '0000110b-0000-1000-8000-00805f9b34fb';
	self.endpoint_props.Codec = 0x00;
	self.endpoint_props.Capabilities = sbc_config;
	
	self.register = function(adapterPath, onComplete) {
		register(self, adapterPath, onComplete);
	}
	
	self.unregister = function(adapterPath, onComplete) {
		unregister(self, adapterPath, onComplete);
	}
}

function unregister(self, adapterPath, onComplete) {

}

function register(self, adapterPath, onComplete) {
	if (!self.profile) {
		var obj = self.service.createObject(self.object_path);
		var iface = obj.createInterface('org.bluez.MediaEndpoint1');
		self.profile = {obj: obj, iface: iface};
				
		iface.addMethod('SetConfiguration', {in: [ DBus.Define(Object), DBus.Define(Object) ]}, function(path, props, callback) {
			console.log();
			console.log('>>> A2dpSinkEndpoint.SetConfiguration called');
			console.log('    path: ' + path);
			console.log('    props: ' + JSON.stringify(props));
			console.log();
			callback();
			A2dpAudioHandler.add(path, props.Configuration); //will remove any existing handler and then add...
		});
		
		iface.addMethod('SelectConfiguration', {in: [ DBus.Define(Array) ], out: DBus.Define(Array)}, function(capabilities, callback) {
			console.log();
			console.log('>>> A2dpSinkEndpoint.SelectConfiguration called');
			console.log('    capabilities: ' + capabilities);
			console.log();
			callback(sbc_config);
		});
		
		iface.addMethod('ClearConfiguration', {in: [ DBus.Define(Object) ]}, function(path, callback) {
			console.log();
			console.log('>>> A2dpSinkEndpoint.ClearConfiguration called');
			console.log('    path: ' + path);
			console.log();
			callback();
		});
		
		iface.addMethod('Release', {}, function(callback) {
			console.log();
			console.log('>> A2dpSinkEndpoint.Release called');
			console.log();
			callback();
		});
		
		iface.update();
	}
	bus.getInterface('org.bluez', adapterPath, 'org.bluez.Media1', function(err, iface) {
		if (notErr(err)) {
			iface.RegisterEndpoint['timeout'] = 1000;
			iface.RegisterEndpoint['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				console.log('A2dpSinkEndpoint.register(' + adapterPath + ') - SUCCESS');
				if (onComplete) onComplete.apply(null, args);
			};
			iface.RegisterEndpoint['error'] = function(err) {
				console.log('[error] A2dpSinkEndpoint.register - ' + err);
				console.log('A2dpSinkEndpoint.register(' + adapterPath + ') - ERROR');
				if (onComplete) onComplete(err);
			};
			iface.RegisterEndpoint(self.object_path, self.endpoint_props);
		}
	});
}

module.exports = new A2dpSinkEndpoint();