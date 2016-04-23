global.print("Test equal... ");

let assert = global.import('./assert.cr');

assert.assert(1 == 1);
assert.assert("test" == "test");
assert.assert(global.undefined == global.undefined);

let a = {};
assert.assert(a == a);
assert.assert(global.undefined == a.a);

let b = [];
assert.assert(b == b);
assert.assert(global.undefined == b[0]);

let c = a;
assert.assert(a == c);

global.println('Done');
