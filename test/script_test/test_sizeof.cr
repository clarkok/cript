global.print('Test sizeof... ');

let sizeof = global.sizeof;
let assert = global.import('assert.cr');

assert.assertIntEq(-1, sizeof(1));
assert.assertIntEq(5, sizeof("12345"));
assert.assertIntEq(0, sizeof({}));
assert.assertIntEq(0, sizeof([]));
assert.assertIntEq(3, sizeof({a : 1, b : 2, c : 3}));
assert.assertIntEq(5, sizeof([1, 2, 3, 4, 5]));
assert.assertIntEq(-1, sizeof(function () {}));
assert.assertIntEq(-1, sizeof(global.println));

global.println('Done');
