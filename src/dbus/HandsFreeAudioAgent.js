const DBus = require('../../lib/dbus');

const hfpio = require('../../lib/fcntl');

//const hfpio = require('../../lib/hfpio');

const spawn = require('child_process').spawn;

const appUtils = require('../utils/app_utils.js');
const dbusUtils = require('../utils/dbus_utils.js');
const bus = dbusUtils.bus;
const notErr = dbusUtils.notErr;
const addSignalHandler = dbusUtils.addSignalHandler;
const removeAllHandlersForSignal = dbusUtils.removeAllHandlersForSignal;

const EventEmitter = require('events').EventEmitter;
const util = require('util');
util.inherits(HandsFreeAudioAgent, EventEmitter);

function HandsFreeAudioAgent() {
	var self = this;
	self.object_path = '/handsfree/audioagent';
	self.service = dbusUtils.dbus.registerService('system', 'handsfree.audioagent');

	self.start = function(onComplete) {
		self.cards = {};
		addCardAddedHandler(addCardRemovedHandler(function() {
			registerAgent(self, function(err) {
				if (notErr(err)) {
					// delay to give enuff time for agent to get registered
					setTimeout(onComplete, 100);
				} else {
					onComplete(err);
				}
			});
		}));
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
	
	self.getCards = function() {
		return self.cards;
	}
	
	var addCardAddedHandler = function(onComplete) {
		removeAllHandlersForSignal('org.ofono', '/', 'org.ofono.HandsfreeAudioManager', 'CardAdded', function() {
			addSignalHandler('org.ofono', '/', 'org.ofono.HandsfreeAudioManager', 'CardAdded', function(path, card) {
				card.path = path;
				self.cards[card.RemoteAddress] = card;
				console.log();
				console.log('HandsfreeAudioManager.CardAdded: ' + path + ',' + JSON.stringify(card));
				console.log();
			}, onComplete);
		});
	}

	var addCardRemovedHandler = function(onComplete) {
		removeAllHandlersForSignal('org.ofono', '/', 'org.ofono.HandsfreeAudioManager', 'CardRemoved', function() {
			addSignalHandler('org.ofono', '/', 'org.ofono.HandsfreeAudioManager', 'CardRemoved', function(path) {	
				var keys = Object.keys(self.cards);
				for (var i=0; i<keys.length; i++) {
					if (self.cards[keys[i]] && self.cards[keys[i]].path == path) {
						self.cards[keys[i]] = undefined;
						break;
					}
				}
			}, onComplete);
		});
	}
}

/*
getCards(function(err, cardsData) {
	if (notErr(err)) {
		if (cardsData.length > 0) {	
			Object.keys(cardsData[0]).forEach(function(key) {
				var card = cardsData[0][key];
				cards[card.RemoteAddress] = card;
			});
		}
	}
});

function getCards(onComplete) {
	bus.getInterface('org.ofono', '/', 'org.ofono.HandsfreeAudioManager', function(err, iface) {
		if (notErr(err)) {
			iface.GetCards['timeout'] = 1000;
			iface.GetCards['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.GetCards['error'] = function(err) {
				console.log('[error] HandsFreeAudioAgent.getCards - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.GetCards();
		}
	});
}
*/

function unregisterAgent(self, onComplete) {
	bus.getInterface('org.ofono', '/', 'org.ofono.HandsfreeAudioManager', function(err, iface) {
		if (notErr(err)) {
			iface.Unregister['timeout'] = 1000;
			iface.Unregister['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.Unregister['error'] = function(err) {
				console.log('[error] HandsFreeAudioAgent.unregisterAgent - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.Unregister(self.object_path);
		}
	});
}

function registerAgent(self, onComplete) {
	if (!self.profile) {
		var obj = self.service.createObject(self.object_path);
		var iface = obj.createInterface('org.ofono.HandsfreeAudioAgent');
		self.profile = {obj: obj, iface: iface};
		iface.addMethod('Release', {}, function(callback) {
			console.log('>> HandsFreeAudioAgent.Release called');
			callback();
		});
		iface.addMethod('NewConnection', {in: [ DBus.Define(Object), DBus.Define('File'), DBus.Define(Number) ]}, function(card, fd, codec, callback) {
			console.log('>>> HandsFreeAudioAgent.NewConnection called');
			console.log('    card: ' + card);
			console.log('    fd: ' + fd);
			console.log('    codec: ' + codec);
			callback();
			if (codec == 2) {
				hfpio.msbcIo(fd);
			} else {
				hfpio.cvsdIo(fd);
			}
		});
		iface.update();
	}
	bus.getInterface('org.ofono', '/', 'org.ofono.HandsfreeAudioManager', function(err, iface) {
		if (notErr(err)) {
			iface.Register['timeout'] = 1000;
			iface.Register['finish'] = function() {
				var args = Array.prototype.slice.call(arguments);
				args.splice(0, 0, null); // there is no error
				if (onComplete) onComplete.apply(null, args);
			};
			iface.Register['error'] = function(err) {
				console.log('[error] HandsFreeAudioAgent.registerAgent - ' + err);
				if (onComplete) onComplete(err);
			};
			iface.Register(self.object_path, [1, 2]);
		}
	});
}

module.exports = new HandsFreeAudioAgent();
