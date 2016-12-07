const DBus = require('../../lib/dbus');
const hfpio = require('../../lib/fcntl');
const spawn = require('child_process').spawn;
const appUtils = require('../utils/app_utils.js');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(A2dpSinkEndPoint, EventEmitter);

const SBC_CHANNEL_MODE_MONO = (1 << 3)
const SBC_CHANNEL_MODE_DUAL_CHANNEL = (1 << 2)
const SBC_CHANNEL_MODE_STEREO = (1 << 1)
const SBC_CHANNEL_MODE_JOINT_STEREO = 1

const SBC_SAMPLING_FREQ_16000 = (1 << 3)
const SBC_SAMPLING_FREQ_32000 = (1 << 2)
const SBC_SAMPLING_FREQ_44100 = (1 << 1)
const SBC_SAMPLING_FREQ_48000 = 1

const SBC_ALLOCATION_SNR = (1 << 1)
const SBC_ALLOCATION_LOUDNESS = 1

const SBC_SUBBANDS_4 = (1 << 1)
const SBC_SUBBANDS_8 = 1

const SBC_BLOCK_LENGTH_4 = (1 << 3)
const SBC_BLOCK_LENGTH_8 = (1 << 2)
const SBC_BLOCK_LENGTH_12 = (1 << 1)
const SBC_BLOCK_LENGTH_16 = 1

const MIN_BITPOOL = 2
const MAX_BITPOOL = 64

const channel_mode = SBC_CHANNEL_MODE_MONO | SBC_CHANNEL_MODE_DUAL_CHANNEL| 
					 SBC_CHANNEL_MODE_STEREO | SBC_CHANNEL_MODE_JOINT_STEREO;
const frequency = SBC_SAMPLING_FREQ_16000 | SBC_SAMPLING_FREQ_32000 | SBC_SAMPLING_FREQ_44100 |
				  SBC_SAMPLING_FREQ_48000;
const allocation_method = SBC_ALLOCATION_SNR | SBC_ALLOCATION_LOUDNESS;
const subbands = SBC_SUBBANDS_4 | SBC_SUBBANDS_8;
const block_length = SBC_BLOCK_LENGTH_4 | SBC_BLOCK_LENGTH_8 | SBC_BLOCK_LENGTH_12 | SBC_BLOCK_LENGTH_16;
const min_bitpool = MIN_BITPOOL;
const max_bitpool = MAX_BITPOOL;
const sbc_capabilities = [channel_mode, frequency, allocation_method, subbands, block_length, min_bitpool, max_bitpool];

// JointStereo 44.1Khz Subbands: Blocks: 16 Bitpool Range: 2-32
const sbc_config = [0x21, 0x15, 2, 32];

function A2dpSinkEndPoint() {
	var self = this;
	self.object_path = '/MediaEndpoint/A2DPSink';
	self.service = dbusUtils.dbus.registerService('system', 'mediaendpoint.a2dpsink');	
	self.endpoint_props = {};
	self.endpoint_props.UUID = '0000110b-0000-1000-8000-00805f9b34fb';
	self.endpoint_props.Codec = 0x00;
	self.endpoint_props.Capabilities = sbc_capabilities;
	
	self.start = function(onComplete) {
		registerEndpoint(self, onComplete);
	}
	
	self.stop = function(onComplete) {
		unregisterEndpoint(self, onComplete);
	}
}

function unregisterEndpoint(self, onComplete) {

}

function registerEndpoint(self, onComplete) {
	
	if (!self.profile) {
		
		var obj = self.service.createObject(self.object_path);
		var iface = obj.createInterface('org.bluez.MediaEndpoint1');
		self.profile = {obj: obj, iface: iface};
				
		iface.addMethod('SetConfiguration', {in: [ DBus.Define(Object), DBus.Define(Object) ]}, function(path, props, callback) {
			console.log('>>> A2dpSinkEndPoint.SetConfiguration called');
			console.log('    path: ' + path);
			console.log('    props: ' + JSON.stringify(props));
			console.log();
			callback();
		});
		
		iface.addMethod('SelectConfiguration', {out: [ DBus.Define(Array) ], in: [ DBus.Define(Array) ]}, function(capabilities, callback) {
			console.log('>>> A2dpSinkEndPoint.SelectConfiguration called');
			console.log('    capabilities: ' + capabilities);
			console.log();
			callback(sbc_config);
		});
		
		iface.addMethod('ClearConfiguration', {in: [ DBus.Define(Object) ]}, function(path, callback) {
			console.log('>>> A2dpSinkEndPoint.ClearConfiguration called');
			console.log('    path: ' + path);
			console.log();
			callback();
		});
		
		iface.addMethod('Release', {}, function(callback) {
			console.log('>> A2dpSinkEndPoint.Release called');
			console.log();
			callback();
		});
		
		iface.update();
	}
	
	bus.getInterface('org.bluez', '/org/bluez/hci0', 'org.bluez.Media1', function(err, iface) {
		if (notErr(err)) {
			iface.RegisterEndpoint['timeout'] = 1000;
			iface.RegisterEndpoint['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.RegisterEndpoint['error'] = function(err) {
				console.log('[error] A2dpSinkEndPoint.registerEndpoint - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.RegisterEndpoint(self.object_path, self.endpoint_props);
		}
	});
}

module.exports = new A2dpSinkEndPoint();
