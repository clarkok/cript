global.println('Test sizeof');
global.println('should print some size, view the code');

let sizeof = global.sizeof;
let println = global.println;

println(sizeof(1));
println(sizeof("12345"));
println(sizeof({}));
println(sizeof([]));
println(sizeof({a : 1, b : 2, c : 3}));
println(sizeof([1, 2, 3, 4, 5]));
println(sizeof(function () {}));
println(sizeof(global.println));

