var encoder = require('bindings')('aacencoder.node')


console.log('Step 1');
var obj = new encoder.AACEncoder(8000, 2);
console.log('Step 2');


var buf = new ArrayBuffer(10);
var u8array = new Uint8Array(buf);

var i;
for (i = 0; i < u8array.length; i++) {
    u8array[i] = 0x30+i;
}

console.log('Step 3');

var outbuffer =  obj.encode(buf);

console.log('Step 4');

const outarray = new Uint8Array(outbuffer);

console.log('Step 5');

var i;
for (i = 0; i < outarray.length; i++) {
    console.log(outarray[i]);
}
