const a2dp = require('bindings')('a2dp');

a2dp.play = function(fd, config) {
	
	console.log();
	console.log('!! ----- GOT HERE ----- a2dp.play() fd: ' + fd + ' config: ' + config);

	a2dp.sbcNew(Buffer.from(config));
	
}

module.exports = a2dp;