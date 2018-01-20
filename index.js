var usbspyBinding = require('bindings')('usbspy.node');
var events = require('events');

var index = require('./package.json');

if (global[index.name] && global[index.name].version === index.version) {
    module.exports = global[index.name];
} else {

    var usbspy = new events.EventEmitter()

    usbspy.spyOn = function () {
        usbspyBinding.spyOn(function (data) {
            usbspy.emit('change', data);
        }, function (data) {
            usbspy.emit('end', data);
        });
    }

    usbspy.spyOff = function () {
        usbspyBinding.spyOff();
    }

    usbspy.getAvailableUSBDevices = function () {
        return usbspyBinding.getAvailableUSBDevices();
    };

    usbspy.getUSBDeviceByPropertyName = function (propertyName, value) {
        return usbspyBinding.GetUSBDeviceByPropertyName(propertyName, value);
    };

    usbspy.version = index.version;
	global[index.name] = usbspy;

	module.exports = usbspy;
}