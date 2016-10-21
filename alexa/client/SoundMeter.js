var EventEmitter = require('events').EventEmitter;
var util = require('util');
var spawn = require('child_process').spawn;

util.inherits(SoundMeter, EventEmitter);

const consecutiveSilentSamples = 12;
const threshold = 10.0;

function SoundMeter() {
	this.ts = null;
	this.samples = [];
	this.counter = 0;
	this.isSpeechStarted = false;
    EventEmitter.call(this);
}

SoundMeter.prototype.write = function (data) {
    for (var i = 0; i < data.length; i += 2) {
		const sample = data.readIntLE(i, 2) / 32768;
		this.samples.push(sample);
		if (this.samples.length == 2048) {
			const loudness = calculateLoudness(this.samples);
			if (!this.isSpeechStarted) {
				if (loudness > threshold) {
					this.isSpeechStarted = true;
					this.emit('speech_begin', data);
					console.log('speech_begin');
					console.log();
				}
			} else {
			    if (loudness <= threshold) {
			    	this.counter++;
					if (this.counter == consecutiveSilentSamples) {
						//console.log('time[' + new Date() + '], volume=' + loudness);
						/*
						if (this.ts == null) {
							this.ts = new Date().getTime()
						} else {
							const timeNow = new Date().getTime();
							console.log(timeNow - ts)
							this.ts = timeNow
						}
						*/
					    this.emit('speech_end');
					    console.log('speech_end');
					    this.isSpeechStarted = false;
					    this.counter = 0;
					}
			    } else {
			    	this.counter = 0;
			    }
			}
			this.samples = [];
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

module.exports = SoundMeter;
