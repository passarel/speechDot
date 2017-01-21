const DBus = require('../../lib/dbus');
//const a2dp = require('../../lib/bluetooth').a2dp;

//const spawn = require('child_process').spawn;

const assert = require('assert');
//const appUtils = require('../utils/app_utils.js');

const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;

const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(A2dpEndPoint, EventEmitter);

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
const sbc_config = [255, 255, 2, 64];

const A2DP_SINK_ENDPOINT = '/MediaEndpoint/A2DPSink';
const A2DP_SOURCE_ENDPOINT = '/MediaEndpoint/A2DPSource';

const A2DP_SINK_UUID = '0000110b-0000-1000-8000-00805f9b34fb';
const A2DP_SOURCE_UUID = '0000110a-0000-1000-8000-00805f9b34fb';

function A2dpEndPoint(type) {
	assert(type === 'sink' || type === 'source');
	
	var self = this;
	self.type = type;

	self.register = function(adapter, onComplete) {
		registerEndpoint(self, adapter, onComplete);
	}
}

function init(self) {
	self.service = dbusUtils.dbus.registerService('system', 'mediaendpoint.' + type);	
	self.endpoint_props = {};
	self.endpoint_props.Codec = 0x00;
	self.endpoint_props.Capabilities = sbc_capabilities;
	if (self.type === 'sink') {
		self.endpoint_props.UUID = A2DP_SINK_UUID;
		self.object_path = A2DP_SINK_ENDPOINT;
		self.obj = self.service.createObject(self.object_path);
	} else {
		self.endpoint_props.UUID = A2DP_SOURCE_UUID;
		self.object_path = A2DP_SOURCE_ENDPOINT;
		self.obj = self.service.createObject(self.object_path);
	}
	const iface = self.obj.createInterface('org.bluez.MediaEndpoint1');	
	iface.addMethod('SetConfiguration', {in: [ DBus.Define(Object), DBus.Define(Object) ]}, function(path, props, callback) {
		console.log();
		console.log('>>> A2dpEndPoint.' + self.type + '.SetConfiguration called');
		console.log('    path: ' + path);
		console.log('    props: ' + JSON.stringify(props));
		console.log();
		callback();
		/*
		addSignalHandler('org.bluez', self.mediaTransport.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function(ifaceName, props) {
			Object.keys(props).forEach(function(name) {
				const val = props[name];
				self.mediaTransport[name] = val;
				console.log('[Bluez.MediaTransport1] PropertyChanged: ' + self.mediaTransport.path + ', ' + name + '=' + val);
				if (name === 'State') onState(self, val);
			});
		});
		*/
	});
	iface.addMethod('SelectConfiguration', {in: [ DBus.Define(Array) ], out: DBus.Define(Array)}, function(capabilities, callback) {
		console.log();
		console.log('>>> A2dpEndPoint.' + self.type + '.SelectConfiguration called');
		console.log('    capabilities: ' + capabilities);
		console.log('    responding with sbc_config: ' + sbc_config);
		callback(sbc_config);
	});
	iface.addMethod('ClearConfiguration', {in: [ DBus.Define(Object) ]}, function(path, callback) {
		console.log();
		console.log('>>> A2dpEndPoint.' + self.type + '.ClearConfiguration called');
		console.log('    path: ' + path);
		console.log();
		callback();
	});
	iface.addMethod('Release', {}, function(callback) {
		console.log();
		console.log('>>> A2dpEndPoint.' + self.type + '.Release called');
		console.log();
		callback();
	});
	iface.update();
}

function registerEndpoint(self, adapter, onComplete) {
	if (!self.service) init(self);
	bus.getInterface('org.bluez', '/org/bluez/' + adapter, 'org.bluez.Media1', function(err, iface) {
		if (notErr(err)) {
			iface.RegisterEndpoint['timeout'] = 1000;
			iface.RegisterEndpoint['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				console.log('A2dpEndPoint.' + self.type + '.registerEndpoint(' + adapter + ') - SUCCESS');
				if (onComplete) onComplete.apply(null, args);
			};
			iface.RegisterEndpoint['error'] = function(err) {
				console.log('[error] A2dpEndPoint.' + self.type + '.registerEndpoint - ' + err);
				console.log('A2dpEndPoint.' + self.type + '.registerEndpoint(' + adapter + ') - ERROR');
				if (onComplete) onComplete(err);
			};			
			iface.RegisterEndpoint(self.object_path, self.endpoint_props);
		}
	});
}

module.exports = {sink: new A2dpEndPoint('sink'), source: new A2dpEndPoint('source')};
