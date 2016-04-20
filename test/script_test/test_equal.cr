global.println("Test equal");
global.println("Should print 1");

let println = global.println;

println(1 == 1);
println("test" == "test");
println(global.undefined == global.undefined);

let a = {};
println(a == a);
println(global.undefined == a.a);

let b = [];
println(b == b);
println(global.undefined == b[0]);

let c = a;
println(a == c);
