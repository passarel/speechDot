if (!String.prototype.contains) {
	String.prototype.contains = function (str) {
	    return this.indexOf(str) != -1;
	};
}

if (!String.prototype.replaceAll) {
	String.prototype.replaceAll = function(search, replacement) {
	    var target = this;
	    return target.replace(new RegExp(search, 'g'), replacement);
	};
}
