
/*
const A2dpSinkEndPoint = require('./src/dbus/A2dpSinkEndPoint.js');

const Bluez = require('./src/dbus/Bluez.js');

Bluez.on('ready', function(adapters) {
	
	console.log();
	console.log();
	console.log();
	console.log();
	
	//A2dpSinkEndPoint.start();
	
});
*/

/*
const c = new Buffer([255, 255, 2, 64]);

console.log(c);

const a2dp = require('./lib/bluetooth').a2dp;

console.log(a2dp);

console.log( a2dp.sbcConfigFromByteArray(c) );
*/

//t->path=/org/bluez/hci0/dev_40_40_A7_1A_7A_E8/fd0

var test = '/org/bluez/hci0/dev_40_40_A7_1A_7A_E8'
	
console.log(test.substring( test.lastIndexOf('/')+1 ));