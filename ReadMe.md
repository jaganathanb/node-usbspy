##### Note: Designed for Windows

# node-usbspy
An event based node.js c++ addon/binding to retrive the connected usb storage devices and detect the storage device insertion/removal and notifify the subscribed apps.

## Prerequisties for installation
Before installing this package, make sure you have Node.js addon build tool [node-gyp](https://github.com/nodejs/node-gyp) installed in your machine.

As `node-gyp` uses `Python 2.x`, make sure it is installed in your machine and it is on your EVN path.

If you dont have the python installed, then it is recommanded to install the `windows-build-tools`. To installl windows build tool, open CLI as adminstrator and execute the below command as per the issue discussed [here](https://github.com/felixrieseberg/windows-build-tools/issues/56).

```
npm --add-python-to-path='true' --debug install --global windows-build-tools
```

The above command would install pythin 2.x and build tools needed for windows. It would take few mins to complete the installation.


## Installation
To install `node-usbspy` execute the below command

if you prefer `npm`
```
npm install node-usbspy
```

for `yarn`,

```
yarn add node-usbspy
```
Upon installation, `node-gyp` would start generating the c++ addon project as per the configuration we set in `binding.gyp` and compile the same. If the compilation is successfull, it would have generated the executable lib files in the `build`folder with `Release` configuration.

## How to use it

`require('node-usbspy')` would populate the usbspy object which is of event type.

### Activating the detection(spying on the usb controller)

```
var usbspy = require('node-usbspy');

usbspy.spyOn().then(() => {
    ...
})
```

### Deactivating the detection

```
usbspy.spyOff() // would stop listening for the usb detection.
```

### Listening for change
There is an event `change` which would be emitted when a usb device is inserted or removed. You can subscribe for the event and do necessory action upon the event.

```
usbspy.on('change', (device) => {
    console.log(device);
    /* { device_number: 1,
         device_status: 1,
         device_letter: 'D:\\',
         vendor_id: 'SanDisk ',
         serial_number: '4C530001250818',
         product_id: 'Cruze'
       } */
});

usbspy.on('end', () => {
    // would be triggered when you call `spyOff` function.
})
```

## API

### Properties
When an usb device is inserted, an `Device` object would be generated and emitted.

`Device` object has,

* `device_status` - It is an integer type property. The possible values for this property is 0 or 1 when 0 indicates the device is been removed from the system and the 1 indicated the device is been added.
* `device_letter` - It is an string type property. The letter assigned to device by the operating system when the usb device is inserted.
*  `device_number` - It is an integer type property. The unique number assigned to every usb storage device inserted into the system by OS. Number starts from 1.
*   `serial_number` - It is an string type property. The unique albha-numeric assigned to the usb storage device by the manufacturer.
*    `vendor_id` - It is an string type property. The vendor id is assigned by the USB Implementers Forum to a specific company.
*    `product_id` - It is an string type property. The product id is assigned by the company for the individual product.


### Methods
There are four methods available in `usbspy`.

#### spyOn([callback])
`spyOn` method takes a callback as parameter and returns a promise object. Here `callback` is optional. Since spyOn returns promise, you can use `then` to kick start the detection. 

When the addon is ready, `callback` would be called with `true` which indicates everything is OK if passed otherwise the promise would be resolve/rejected.

#### spyOff()
`spyOff` should be called when you wanted to stop listening for the usb device change.

#### getAvailableUSBDevices()
`getAvailableUSBDevices` would written list of `Device` objects if available otherwise empty list would be returned. This method does not take any arguments.

#### getUSBDeviceByPropertyName(propertyName<string>, value<string|number>)
This method takes two arguments. The `propertyName` could be any of the `Device` properties. The `value` should be the actual value of the property. This method returns `Device` object if the property/value passed matches any of the available usb storage devices.
    

### Events
There are two events emitted from the `usbspy` module. 

#### change - usbspy.on('change' callback(device))
When any usb storage device is been inserted/removed into/from the machine, `change` event would be triggered with the `Device` object.

#### end - usbspy.on('end', callback)
When the `spyOff` method is called, the `end` event would be triggered.

## Example

You can have a look into `example/test.js` for usage and example.

```
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

setTimeout(() => {
    usbspy.spyOff();
}, 5000); // after 5 secs, would stop wathcing for device change.
```
##### Note:
When you DEBUG the c++ code, you have to comment the line#18 in the `usbspy.h`
