global.print('Test typeof... ');

let assert = global.import('./assert.cr');

let int = 1;
let str = 'string';
let obj = {};
let arr = [];
let clos = function () {};
let func = global.print;

assert.assertStringEq('integer', global.typeof(int));
assert.assertStringEq('string', global.typeof(str));
assert.assertStringEq('object', global.typeof(obj));
assert.assertStringEq('array', global.typeof(arr));
assert.assertStringEq('closure', global.typeof(clos));
assert.assertStringEq('light_function', global.typeof(func));

global.println('Done');
