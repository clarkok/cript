global.println('Test typeof');
global.println('should output 6 types');

let int = 1;
let str = 'string';
let obj = {};
let arr = [];
let clos = function () {};
let func = global.print;

global.println(global.typeof(int));
global.println(global.typeof(str));
global.println(global.typeof(obj));
global.println(global.typeof(arr));
global.println(global.typeof(clos));
global.println(global.typeof(func));
