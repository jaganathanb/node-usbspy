var usbspyBinding = require('bindings')('usbspy.node');
var events = require('events');

require('es6-promise/auto');

var index = require('./package.json');

if (global[index.name] && global[index.name].version === index.version) {
    module.exports = global[index.name];
} else {

    var usbspy = new events.EventEmitter()
    var ready = false;

    usbspy.spyOn = function (callback) {
        var promise = new Promise(function(resolve, reject) {
            console.log(ready);
            if (!ready) {
                usbspyBinding.spyOn(function (data) {
                    usbspy.emit('change', data);
                }, function (data) {
                    usbspy.emit('end', data);
                }, function() {
                    resolve(true);
                    callback(true);
                });
                ready = true;
            } else {
                reject();
                callback(false);
            }
        });
        
        return promise;
    }

    usbspy.spyOff = function () {
        if (!ready) {
            usbspyBinding.spyOff();
            ready = false;
        }
    }

    usbspy.getAvailableUSBDevices = function () {
        return usbspyBinding.getAvailableUSBDevices();
    };

    usbspy.getUSBDeviceByPropertyName = function (propertyName, value) {
        return usbspyBinding.getUSBDeviceByPropertyName(propertyName, value);
    };

    usbspy.version = index.version;
	global[index.name] = usbspy;

	module.exports = usbspy;
}