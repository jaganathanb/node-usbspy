var ap = require('./index');

ap.on('progress', (data) => {
    console.log('from progress ');
    console.log(data);
});

ap.on('data', (data) => {
    console.log('from data ');
    console.log(data);
});

ap.spyOn();

setTimeout(() => {
    ap.spyOff();
}, 5000);