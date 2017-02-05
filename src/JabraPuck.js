var EventEmitter = require('events').EventEmitter;
var util = require('util');
var HID = require('node-hid');

util.inherits(JabraPuck, EventEmitter);

function JabraPuck() {
	
    const jabra_puck = {
        vid: "2830",
        pid: "1042"
    }
	
    var self = this;

    console.log('devices:', HID.devices());
    
    /*
    self.device = new HID.HID(jabra_puck.vid, jabra_puck.pid);

    console.log(self.device);
    
    self.device.on('data', function(data) {
    	console.log('GOT HERE - DATA');
    	for (var i=0; i<data.length; i++) {
    		console.log('data[' + i + '] = ' + data[i]);
    	}
    	console.log();
    });
    */
}

module.exports = new JabraPuck();