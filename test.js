var ap = require('./index');

ap.on('change', (data) => {
    console.log(data);
});

ap.on('end', (data) => {
    console.log(data);
});

ap.spyOn();

// setTimeout(() => {
//     ap.spyOff();
// }, 40000);