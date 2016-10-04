require('./string_utils.js');
const DBus = require('../../lib/dbus');
const dbus = new DBus();
const bus = dbus.getBus('system');

//console.log(bus);

module.exports = {
		
	addSignalHandler: addSignalHandler,
	
	addPropertyChangedSignalHandler: addPropertyChangedSignalHandler,
	
	removeAllHandlersForSignal: removeAllHandlersForSignal,
	
	removeAllHandlersForPropertyChangedSignal: removeAllHandlersForPropertyChangedSignal,
	
	bus: bus,
	
	dbus: dbus,
	
	notErr: notErr,
	
	getManagedObjects: function(serviceName, onManagedObjects, onErr) {
		getObjectManager(serviceName, function(iface) {
			iface.GetManagedObjects['timeout'] = 2000;
			iface.GetManagedObjects['finish'] = onManagedObjects;
			iface.GetManagedObjects['error'] = function(err) {
				console.log('[error] getManagedObjects for ' + serviceName + ' - ' + err);
				if (onErr) onErr(err);
			};
			iface.GetManagedObjects();
		}, onErr);
	},
	
	getObjectManager: getObjectManager
}

/*
function setProperty(serviceName, path, ifaceName, name, val, onComplete) {
	bus.getInterface(serviceName, path, 'org.freedesktop.DBus.Properties', function(err, iface) {
		if (notErr(err)) {
			iface.Set['timeout'] = 2000;
			iface.Set['finish'] = function() {
				//console.log('setProperty ' + name + '=' + val + ' for ' + path);
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); //there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.Set['error'] = function(err) {
				console.log('[error] setProperty(' + name + '=' + val + ') for ' + path + ' - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.Set(ifaceName, name, val);
		}
	});
}
*/

function removeAllHandlersForPropertyChangedSignal(serviceName, path, ifaceName, onComplete) {
	removeAllHandlersForSignal(serviceName, path, ifaceName, 'PropertyChanged', onComplete);
}

function addPropertyChangedSignalHandler(serviceName, path, ifaceName, handler, onComplete) {
	addSignalHandler(serviceName, path, ifaceName, 'PropertyChanged', handler, onComplete);
}

function getObjectManager(serviceName, onObjectManager, onErr) {
	bus.getInterface(serviceName, '/', 'org.freedesktop.DBus.ObjectManager', function(err, iface) {
		if (notErr(err)) {
			if (onObjectManager) onObjectManager(iface);
		} else {
			onErr(err);
		}
	});
}

function addSignalHandler(serviceName, path, ifaceName, signal, handler, onComplete) {
	bus.getInterface(serviceName, path, ifaceName, function(err, iface) {
		if (notErr(err)) {
			var logInfo = signal + ' on ' + serviceName + ',' + path + ',' + ifaceName;
			if (iface.object.signal[signal]) {
				iface.on(signal, handler);
				console.log('added signalHandler for ' + logInfo);
				if (onComplete) onComplete();
			} else {
				console.log('[error] addSignalHandler - Signal not found for ' + logInfo);
				console.log('        available signals -> ' + Object.keys(iface.object.signal));
				if (onComplete) onComplete();
			}
		} else {
			if (onComplete) onComplete(err);
		}
	});
}

function removeAllHandlersForSignal(serviceName, path, ifaceName, signal, onComplete) {
	bus.getInterface(serviceName, path, ifaceName, function(err, iface) {
		if (notErr(err)) {
			var logInfo = signal + ' on ' + serviceName + ',' + path + ',' + ifaceName;
			if (iface.object.signal[signal]) {
				iface.removeAllListeners(signal);
				console.log('removed all signalHandlers for ' + logInfo);
				if (onComplete) onComplete();
			} else {
				console.log('[error] removeAllHandlersForSignal - Signal not found for ' + logInfo);
				console.log('        available signals -> ' + Object.keys(iface.object.signal));
				if (onComplete) onComplete();
			}
		} else {
			if (onComplete) onComplete(err);
		}
	});
}

function notErr(err) {
	if (err) {
		console.log(err);
		return false;
	}
	return true;
}
