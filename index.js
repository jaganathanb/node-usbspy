var ap = require('bindings')('asyncprog.node');
var EventEmitter = require('events');

class AsyncProg extends EventEmitter {

}

var asyncProgEE = new AsyncProg();


asyncProgEE.spyOn = () => {
    ap.doProgress(10, (data) => {
        asyncProgEE.emit('progress', data);
    }, (data) => {
        asyncProgEE.emit('data', data);
    });

    ap.spyOn();
}

asyncProgEE.spyOff = () => {
    ap.spyOff();
}


module.exports = asyncProgEE;