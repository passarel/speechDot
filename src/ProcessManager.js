const fs = require('fs');
const spawn = require('child_process').spawn;
const execSync = require('child_process').execSync;
const appUtils = require('./utils/app_utils.js');
const dbusUtils = require('./utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;
const sensory_path = require('path').resolve(__dirname, '../sensory');

const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(ProcessManager, EventEmitter);

const bluezLog = '/home/pi/logs/bluetooth.log';
const ofonoLog = '/home/pi/logs/ofono.log';
const pulseLog = '/home/pi/logs/pulseaudio.log';

module.exports = new ProcessManager();

function ProcessManager() {

	var self = this;
	self.isResetting = false;
	
	var killAll = function(onComplete) {
		self.isKilling = true;
		appUtils.killAllProcessesWithNameSync(['phraseSpot', 'pulseaudio', 'ofonod', 'bluetoothd', 'arecord', 'aplay']);
		clearLogs([bluezLog, ofonoLog, pulseLog]);
		self.bluezChild = null;
		self.ofonoChild = null;
		self.pulseChild = null;
		self.sensoryChild = null;
		self.isBluezDbusOnline = false;
		self.isOfonoDbusOnline = false;
		self.emit('sensory_offline');
		self.emit('pulse_offline');
		self.emit('ofono_offline');
		self.emit('bluez_offline');
		self.isKilling = false;
		setTimeout(function() {
			//give time for things to truly die
			onComplete();
		}, 1000);
	}
	
	var startAll = function() {
		startBluez(self);
		startOfono(self);
	}
	
	self.on('sensory_online', function() {
		self.isResetting = false;
	})
	
	self.on('bluez_online', function() {
		startPulse(self);
	});
	
	self.on('ofono_online', function() {
		startPulse(self);
	});
	
	self.spawnSensoryYesNo = spawnSensoryYesNo;
	
	self.reset = function(onComplete) {
		if (self.isResetting) {
			//already in the process of resetting
			if (onComplete) {
				self.once('sensory_online', onComplete);
			}
			return;
		}
		self.isResetting = true;
		removeAllListeners([self.bluezChild, self.ofonoChild, self.pulseChild, self.sensoryChild]);
		killAll(function() {
			if (onComplete) {
				self.once('sensory_online', onComplete);
			}
			startAll();
		});
	};

	/*
	self.killOfono = function(onComplete) {
		killOfono(self, onComplete);
	}
	*/
	
	// start all the processes
	process.nextTick(function() {
		self.reset();
	});
}

/*
function killOfono(self, onComplete) {
	if (self.ofonoChild) {
		self.ofonoChild.removeAllListeners();
		appUtils.killAllProcessesWithNameSync(['ofonod']);	
	}
	if (onComplete) onComplete();
}
*/

function clearLogs(logFiles) {
	logFiles.forEach(function(logFile) {
		execSync('rm -rf ' + logFile);
	});
}

function removeAllListeners(children) {
	children.forEach(function(child) {
		if (child) {
			child.removeAllListeners();
		}
	});
}

function pipeLogs(child, logFile) {
	child.stderr.pipe(logFile);
	child.stdout.pipe(logFile);
}

function startBluez(self) {
	self.bluezChild = spawn('/usr/local/libexec/bluetooth/bluetoothd', ['-d', '-n', '-C']);
	self.bluezChild.on('exit', function() {
		isBluezDbusOnline = false;
		console.log('[error] bluezChild process has exited.');
		self.reset();
	});
	pipeLogs(self.bluezChild, fs.createWriteStream(bluezLog));
	bluezDbusCheck(self);
}


function startOfono(self) {
	self.ofonoChild = spawn('/usr/sbin/ofonod', ['-d', '-n']);
	self.ofonoChild.on('exit', function() {
		isOfonoDbusOnline = false;
		console.log('[error] ofonoChild process has exited.');
		self.reset();
	});
	pipeLogs(self.ofonoChild, fs.createWriteStream(ofonoLog));
	ofonoDbusCheck(self);
}

function startPulse(self) {
	if (!self.isBluezDbusOnline || !self.isOfonoDbusOnline) return;
	self.pulseChild = spawn('/usr/bin/pulseaudio', ['-v']);
	self.pulseChild.on('exit', function() {
		console.log('[error] pulseChild process has exited.');
		self.reset();
	});
	console.log();
	console.log('-------------------- PULSEAUDIO STARTUP BEGIN --------------------');
	self.pulseChild.stderr.on('data', function(data) {
		var _data = data.toString();
		console.log(_data);
		if (_data.contains('Daemon startup complete')) {
			console.log('-------------------- PULSEAUDIO STARTUP END --------------------');
			console.log();
			self.emit('pulse_online');
			self.pulseChild.stderr.removeAllListeners('data');
			process.nextTick(function() {
				pipeLogs(self.pulseChild, fs.createWriteStream(pulseLog));
				startSensory(self);
			});
		}
	});
}

function startSensory(self) {
	self.sensoryChild = spawn('nice', ['--5', sensory_path + '/Samples/PhraseSpot/phraseSpot']);
	self.sensoryChild.on('exit', function() {
		console.log('[error] sensoryChild process has exited.');
		self.reset();
	});
	console.log('Sensory is online');
	self.emit('sensory_online', self.sensoryChild);
}

function spawnSensoryYesNo() {
	console.log(sensory_path + '/Samples/YesNo/yesNo')
	return spawn(sensory_path + '/Samples/YesNo/yesNo');
	/*
	self.sensoryChild.on('exit', function() {
		console.log('[error] sensoryChild process has exited.');
		self.reset();
	});
	*/
}

function bluezDbusCheck(self) {
	if (self.isBluezDbusOnline) return;
	dbusUtils.getManagedObjects('org.bluez', function(managedObjects) {
		self.isBluezDbusOnline = true;
		console.log('Bluez dbus is online');
		self.emit('bluez_online', managedObjects);
	}, function() {
		setTimeout(function() {
			bluezDbusCheck(self);
		}, 500);
	});
}

function ofonoDbusCheck(self) {
	if (self.isOfonoDbusOnline) return;
	getModems(function(modems) {
		self.isOfonoDbusOnline = true;
		console.log('Ofono dbus is online');
		self.emit('ofono_online', modems);
	}, function() {
		setTimeout(function() {
			ofonoDbusCheck(self);
		}, 500);
	});
}

function getModems(onModems, onErr) {
	bus.getInterface('org.ofono', '/', 'org.ofono.Manager', function(err, iface) {
		if (err) {
			console.log('[ProcessManager.js] failed to getModems: ' + err);
			if (onErr) onErr(err);
			return;
		}
		iface.GetModems['timeout'] = 5000;
		iface.GetModems['finish'] = function(data) {
			onModems(data);
		};
		iface.GetModems['error'] = function(err) {
			console.log(err);
			if (onErr) onErr(err);
		};
		iface.GetModems();
	});
}

