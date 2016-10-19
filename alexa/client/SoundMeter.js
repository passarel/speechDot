var EventEmitter = require('events').EventEmitter;
var util = require('util');
var spawn = require('child_process').spawn;

util.inherits(SoundMeter, EventEmitter);

function SoundMeter() {
    EventEmitter.call(this);
}

var ts = null;
var samples = [];
var counter = 0;
var timeout = 20;

SoundMeter.prototype.write = function (data) {

    for (var i = 0; i < data.length; i += 2) {
	var sample = data.readIntLE(i, 2) / 32768;
	samples.push(sample);
	if (samples.length == 2048) {
	    var loudness = calculateLoudness(samples);
	    // console.log('time[' + new Date() + '], volume=' + loudness);
	    if (loudness <= 10.0) {
		counter++;
		if (counter == timeout) {
		    /*
                      if (ts == null) {
                      ts = new Date().getTime()
                      } else {
                      var timeNow = new Date().getTime();
                      console.log(timeNow-ts)
                      ts = timeNow
                      }
		    */
		    this.emit('speech_end');
		    counter = 0;
		}
	    } else {
		counter = 0;
	    }
	    samples = []
	}
    }
};

function calculateLoudness(input) {
    var len = input.length;
    var j = 0;
    var total = 0;
    while (j < len) total += Math.abs(input[j++])
    var rms = Math.sqrt(total / len);
    return rms * 100
}

module.exports = new SoundMeter();
