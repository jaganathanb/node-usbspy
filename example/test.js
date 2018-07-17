var usbspy = require("../index");

usbspy.spyOn().then(function() {
  usbspy.on("change", function(data) {
    console.log(data);
  });

  usbspy.on("end", function(data) {
    console.log(data);
  });

  console.log(usbspy.getAvailableUSBStorageDevices());

  console.log(usbspy.getUSBStorageDeviceByPropertyName("device_letter", "D:\\"));

  console.log(usbspy.getUSBStorageDeviceByPropertyName("device_number", 1));
});

// console.log(usbspy.getUSBStorageDeviceByPropertyName('serial_number', 'known serial number for the product'));

// console.log(usbspy.getUSBStorageDeviceByPropertyName('product_id', 'known product id for the vendor'));

// console.log(usbspy.getUSBStorageDeviceByPropertyName('vendor_id', 'known vendor id'));

// setTimeout(function() {
//     usbspy.spyOff();
// }, 5000);
