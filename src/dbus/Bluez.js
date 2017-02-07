const execSync = require('child_process').execSync;
const ProcessManager = require('../ProcessManager.js');
const PairingAgent = require('./PairingAgent.js');
const SerialPortProfile = require('./SerialPortProfile.js');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const A2dpSinkEndpoint = require('./A2dpSinkEndpoint.js');

const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(Bluez, EventEmitter);

module.exports = new Bluez();

const adapterConfig = {
	Alias: 'Speech Dot',
	Discoverable: false, 
	Pairable: true
}

function Bluez() {
	
	const self = this;
	
	const addDevice = function(device, onAdded) {
		console.log('[Bluez.js] attempting to add device...');
		if (!self.adapters[device.Adapter]) {
			self.adapters[device.Adapter] = {devices: [], path: device.Adapter, Discoverable: false};
		}
		if (self.devices[device.path]) {
			console.log('not adding device because device already exists');
			return;
		}
		self.devices[device.path] = device;
		self.adapters[device.Adapter].devices.push(device);
		removeAllHandlersForSignal('org.bluez', device.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function() {
			addSignalHandler('org.bluez', device.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function(ifaceName, props) {
				Object.keys(props).forEach(function(name) {
					const val = props[name];
					console.log('[Bluez.Device] PropertyChanged: ' + device.path + ', ' + name + '=' + val);
					device[name] = val;
					if (name === 'Connected' && val) {
						//self.emit('device_connected' , device.path);
						if (self.pairModeAdapter && device.Adapter === self.pairModeAdapter.path) {
							setTimeout(function() {
								setAdapterProperty(self, [device.Adapter], 'Discoverable', false);
							}, 5000); // allow time for phone name to be said first
							// i do not love this approach,
							// but this way we dont need the discoverable state known/manged outside this object.
							// otherwise we need app.js to explicity control discoverable state
							// this is not really a big deal as long as app.js uses non async calls for 
							// saying phone name and saying pairing off...
						}
					}
				});
			});
			console.log('[Bluez.js] device added: ' + JSON.stringify(device));
			if (onAdded) onAdded();
		});
	}
	
	const addSignalHandlers = function(onComplete) {
		const keys = Object.keys(self.adapters);
		var i = 0;
		const nextAdapter = function() {
			if (i < keys.length) {
				i++;
				return self.adapters[keys[i-1]];
			}
			return null;
		}
		const addPropertiesChangedHandler = function() {
			const adapter = nextAdapter();
			if (adapter) {
				removeAllHandlersForSignal('org.bluez', adapter.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function() {
					addSignalHandler('org.bluez', adapter.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function(ifaceName, props) {
						Object.keys(props).forEach(function(name) {
							const val = props[name];
							adapter[name] = val;
							console.log('[Bluez.Adapter] PropertyChanged: ' + adapter.path + ', ' + name + '=' + val);
							if (name === 'Discoverable') {
								if (val) {
									self.pairModeAdapter = adapter;
									self.emit('pairing_mode_on');
								} else {
									self.pairModeAdapter = null;
									self.emit('pairing_mode_off');
								}
							}
							if (name === 'Class' && val === 0) {
								return; // the adapter has been shutdown
							}
							if (name === 'Class' && val !== 2360324) {
								setAdapterClass(self, [adapter.path], '0x240404');
							}
						});
					}, addPropertiesChangedHandler);
				});
			} else {
				if (onComplete) onComplete();
			}
		}
		addPropertiesChangedHandler();
	}
	
	const onManagedObjects = function(mo) {
		Object.keys(mo).forEach(function(key) {
			if (key.startsWith('/org/bluez')) {
				if (mo[key]['org.bluez.Adapter1']) {				
					const adapter = mo[key]['org.bluez.Adapter1'];
					adapter.path = key;
					if (!self.adapters[key]) {
						adapter.devices = [];
					} else {
						self.adapter.devices = self.adapters[key].devices.slice();
					}
					self.adapters[key] = adapter;
				}
				if (mo[key]['org.bluez.Device1']) {
					const device = mo[key]['org.bluez.Device1'];
					device.path = key;
					addDevice(device);
				}
			}
		});
		addSignalHandlers(function() {
			self.powerOnAdapters(function() {
				console.log('all bluetooth adapters are now powered on');
				configureAdapter(self, Object.keys(self.adapters), adapterConfig, function() {
					console.log('all bluetooth adapters are configured');
					startPairingAgent(function() {
						startSerialPortProfile(function() {
							registerA2dpSinkEndpoints(self, Object.keys(self.adapters), function() {
								console.log('all a2dp sink endpoints registered');
							});
						});
					});
					//console.log('---------------------------------------------');
					//console.log(self.adapters);
					//console.log('---------------------------------------------');
				});
			});
		});
	}
	
	function startSerialPortProfile(onComplete) {
		SerialPortProfile.stop(function() {
			SerialPortProfile.start(function() {
				console.log('serialport profile started');
				if (onComplete) onComplete();
			});
		});
	}
	
	function startPairingAgent(onComplete) {
		PairingAgent.stop(function() {
			PairingAgent.start(function() {
				console.log('pairing agent started');
				if (onComplete) onComplete();
			});
		});
	}

	self.powerOnAdapters = function(onComplete) {
		setAdapterProperty(self, Object.keys(self.adapters), 'Powered', true, onComplete);
	}

	self.powerOffAdapters = function(onComplete) {
		setAdapterProperty(self, Object.keys(self.adapters), 'Powered', false, onComplete);
	}

	self.resetAdapters = function(onComplete) {
		resetAdapter(self, Object.keys(self.adapters), function() {
			ProcessManager.reset(onComplete);
		});
	}
	
	self.pair = function(onComplete) {
		const adapter = findAvailableAdapter(self);
		if (!adapter) {
			if (onComplete) onComplete('no available adapter');
			return;
		}
		if (self.pairModeAdapter) {
			if (onComplete) onComplete('pairing already on');
			return;
		}
		setAdapterProperty(self, [adapter.path], 'Discoverable', true, onComplete);
	}
	
	const onBluezDbusOnline = function(managedObjects) {
		self.pairModeAdapter = null;
		self.adapters = {};
		self.devices = {};
		const addInterfacesAddedHandler = function(onComplete) {
			removeAllHandlersForSignal('org.bluez', '/', 'org.freedesktop.DBus.ObjectManager', 'InterfacesAdded', function() {
				addSignalHandler('org.bluez', '/', 'org.freedesktop.DBus.ObjectManager', 'InterfacesAdded', function(path, ifaces) {
					console.log('[Bluez.DBus] InterfacesAdded: ' + path);
					console.log(ifaces);
					console.log();
					if (ifaces['org.bluez.Device1']) {
						const device = ifaces['org.bluez.Device1'];
						device.path = path;
						addDevice(device);
					}
				}, onComplete);
			});
		}
		const addInterfacesRemovedHandler = function() {
			removeAllHandlersForSignal('org.bluez', '/', 'org.freedesktop.DBus.ObjectManager', 'InterfacesRemoved', function() {
				addSignalHandler('org.bluez', '/', 'org.freedesktop.DBus.ObjectManager', 'InterfacesRemoved', function(path, ifaces) {
					console.log('[Bluez.DBus] InterfacesRemoved: ' + path);
					console.log(ifaces);
					console.log();
				}, function() {
					onManagedObjects(managedObjects);
				});
			});
		}
		addInterfacesAddedHandler(addInterfacesRemovedHandler);
	}
	
	ProcessManager.on('bluez_online', onBluezDbusOnline);
	
	ProcessManager.on('pulse_online', function(sensory) {
		// pulse needs to be online to say when a device is connected
		// otherwise its a race condition
		console.log('executing tryConnectDevices...');
		tryConnectDevices(self, Object.keys(self.adapters));
	});
}

function setAdapterClass(self, adapterKeys, clazz) {
	for (var i=0; i<adapterKeys.length; i++) {
		const adapter = self.adapters[adapterKeys[i]];
		const hciN = adapter.path.replace('/org/bluez/', '');
		const cmd = 'hciconfig ' + hciN + ' class ' + clazz;
		console.log('execSync: ' + cmd);
		execSync(cmd);
	}
}

function findAvailableAdapter(self) {
	const keys = Object.keys(self.adapters);
	for (var i=0; i<keys.length; i++) {
		if (self.adapters[keys[i]].devices.length === 0) {
			return self.adapters[keys[i]];
		}
		const devices = self.adapters[keys[i]].devices;
		var allDevicesOffline = true;
		for (var j=0; j<devices.length; j++) {
			if (devices[j].Connected) allDevicesOffline = false;
		}
		if (allDevicesOffline) return self.adapters[keys[i]];
	}
	return null;
}

function resetAdapter(self, adapterKeys, onComplete) {
	var i = 0;
	const nextAdapter = function() {
		if (i < adapterKeys.length) {
			i++;  
			return self.adapters[adapterKeys[i-1]];
		}
		return null;
	}
	const reset = function() {
		const adapter = nextAdapter();
		if (adapter) {
			removeAllHandlersForSignal('org.bluez', adapter.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', function() {
				removeDevices(self, adapter, reset);
			});
		} else {
			configureAdapter(self, adapterKeys, adapterConfig, function() {
				if (onComplete) onComplete();
			});
		}
	}
	reset();
}

function configureAdapter(self, adapterKeys, options, onComplete) {
	const optionKeys = Object.keys(options);
	var i = 0;
	const nextOption = function() {
		if (i < optionKeys.length) {
			i++;  
			return {name: optionKeys[i-1], val: options[optionKeys[i-1]]};
		}
		return null;
	}
	const config = function() {
		const option = nextOption();
		if (option) {
			setAdapterProperty(self, adapterKeys, option.name, option.val, config);
		} else {
			if (onComplete) onComplete();
		}
	}
	config();
}

function registerA2dpSinkEndpoints(self, adapterKeys, onComplete) {
	var i = 0;
	const nextAdapter = function() {
		if (i < adapterKeys.length) {
			i++;  
			return self.adapters[adapterKeys[i-1]];
		}
		return null;
	}
	const registerEndpoint = function() {
		const adapter = nextAdapter();
		if (adapter) {
			A2dpSinkEndpoint.register(adapter.path, registerEndpoint);
		} else {
			if (onComplete) onComplete();
		}
	}
	registerEndpoint();
}

function setAdapterProperty(self, adapterKeys, name, val, onComplete) {
	var i = 0;
	const nextAdapter = function() {
		if (i < adapterKeys.length) {
			i++;  
			return self.adapters[adapterKeys[i-1]];
		}
		return null;
	}
	const setProperty = function() {
		const adapter = nextAdapter();
		if (adapter) {
			bus.getInterface('org.bluez', adapter.path, 'org.bluez.Adapter1', function(err, iface) {
				if (notErr(err)) {
					iface.setProperty(name, val, setProperty);
				} else {
					onComplete(err);
				}
			});
		} else {
			if (onComplete) onComplete();
		}
	}
	setProperty();
}

function removeDevices(self, adapter, onComplete) {
	const nextDevice = function() {
		if (adapter.devices.length > 0) {
			return adapter.devices[0];
		}
		return null;
	}
	const remove = function() {
		const device = nextDevice();
		if (device) {
			console.log('removing device: ' + device.path);
			removeDevice(self, adapter, device, function() {
				removeAllHandlersForSignal('org.bluez', device.path, 'org.freedesktop.DBus.Properties', 'PropertiesChanged', remove);
			});
		} else {
			if (onComplete) onComplete();
		}
	}
	remove();
}

function removeDevice(self, adapter, device, onComplete) {
	const remove = function() {
		for (var i=0; i<adapter.devices.length; i++) {
			if (adapter.devices[i].path === device.path) {
				self.devices[device.path] = null;
				adapter.devices.splice(i, 1); //remove device
				console.log('removed device: ' + device.path);
				break;
			}
		}
	}
	PairingAgent.setDeviceTrusted(device.path, false, function() {
		bus.getInterface('org.bluez', adapter.path, 'org.bluez.Adapter1', function(err, iface) {
			if (notErr(err)) {
				iface.RemoveDevice['timeout'] = 2000;
				iface.RemoveDevice['finish'] = function() {
					remove();
					if (onComplete) onComplete();
				};
				iface.RemoveDevice['error'] = function(err) {
					console.log('[error] remove device(' + device.path + ') - ' + err);
					remove();
					if (onComplete) onComplete();
				};
				iface.RemoveDevice(device.path);
			}
		});
	});
}

function tryConnectDevices(self, adapterKeys, onComplete) {
	var i = 0;
	const nextAdapter = function() {
		if (i < adapterKeys.length) {
			i++;
			return self.adapters[adapterKeys[i-1]];
		}
		return null;
	}
	const tryConnect = function() {
		var adapter = nextAdapter();
		if (adapter) {
			tryConnectDevice(adapter, tryConnect);
		} else {
			if (onComplete) onComplete();
		}
	}
	tryConnect();
}

function tryConnectDevice(adapter, onComplete) {
	var i = 0;
	const nextDevice = function() {
		if (i < adapter.devices.length) {
			i++;  
			return adapter.devices[i-1];
		}
		return null;
	}
	const tryConnect = function() {
		var device = nextDevice();
		if (device) {
			connectDevice(device, function(err) {
				if (err) {
					//not connected, try next device.
					tryConnect();
				} else {
					//connected
					if (onComplete) onComplete();
				}
			});
		} else {
			if (onComplete) onComplete();
		}
	}
	tryConnect();
}

function disconnectDevice(device, onComplete) {
	bus.getInterface('org.bluez', device.path, 'org.bluez.Device1', function(err, iface) {
		if (notErr(err)) {
			iface.Disconnect['timeout'] = 2000;
			iface.Disconnect['finish'] = function() {
				if (onComplete) onComplete.apply(null, [null]);
			};
			iface.Disconnect['error'] = function(err) {
				console.log('[error] disconnect device(' + device.path + ') - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.Disconnect();
		}
	});
}

function connectDevice(device, onComplete) {
	console.log();
	console.log('Trying to connect to device: ' + device.path);
	console.log();
	bus.getInterface('org.bluez', device.path, 'org.bluez.Device1', function(err, iface) {
		if (notErr(err)) {
			iface.Connect['timeout'] = 2000;
			iface.Connect['finish'] = function() {
				if (onComplete) onComplete.apply(null, [null]);
			};
			iface.Connect['error'] = function(err) {
				console.log('[error] connect device(' + device.path + ') - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.Connect();
		}
	});
}

function getDevice(path, onComplete) {
	bus.getInterface('org.bluez', path, 'org.bluez.Device1', function(err, iface) {
		if (notErr(err)) {
			iface.getProperties(onComplete);
		}
	});
}
