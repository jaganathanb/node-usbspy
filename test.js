var ap = require('./index');

ap.on('progress', (data) => {    console.log(data);
});

ap.on('data', (data) => {
    console.log(data);
});

ap.spyOn();

setTimeout(() => {
    ap.spyOff();
}, 40000);