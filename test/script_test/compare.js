'use strict';

let n = 1000000;
let a = [];
for (let i = 0; i < n; ++i) {
    a.push(Math.random() % n + n);
}

a.sort();

