global.print('Test concat ...');

let assert = global.import('assert.cr');

let a = 'hello';
let b = 'world';

let result = global.concat(a, ' ', b);
assert.assertStringEq('hello world', result);

global.println('Done');
