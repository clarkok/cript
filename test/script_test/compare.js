'use strict';

let n = 1000000;
let a = [];

let current = Date.now();
for (let i = 0; i < n; ++i) {
    a.push(Math.random() % n + n);
}
console.log('create array: ', (Date.now()) - current);

current = Date.now();
a.sort();
console.log('sort: ', (Date.now()) - current);
