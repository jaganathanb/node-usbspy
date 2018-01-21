var usbspy = require('../index');

usbspy.spyOn().then(function() {

    usbspy.on('change', function(data) {
        console.log(data);
    });
    
    usbspy.on('end', function(data) {
        console.log(data);
    });

    console.log(usbspy.getAvailableUSBDevices());

    console.log(usbspy.getUSBDeviceByPropertyName('device_letter', 'D:\\'));

    console.log(usbspy.getUSBDeviceByPropertyName('device_number', 1));
});

// console.log(usbspy.getUSBDeviceByPropertyName('serial_number', 'known serial number for the product'));

// console.log(usbspy.getUSBDeviceByPropertyName('product_id', 'known product id for the vendor'));

// console.log(usbspy.getUSBDeviceByPropertyName('vendor_id', 'known vendor id'));

// setTimeout(function() {
//     usbspy.spyOff();
// }, 5000);