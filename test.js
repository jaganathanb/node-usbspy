var ap = require('./index');

ap.on('change', (data) => {
    console.log(data);
});

ap.on('end', (data) => {
    console.log(data);
});

ap.spyOn();

console.log(ap.getAvailableUSBDevices());

console.log(ap.getUSBDeviceByDeviceLetter('F:\\'));

// setTimeout(() => {
//     ap.spyOff();
// }, 40000);