module.exports = {

	parseIpAddress: function(address) {
	    var ip = address.split(':')[3]
	    if (ip === undefined) {
	        ip = address.split(':')[0]
	    }
	    return ip;
	}

}