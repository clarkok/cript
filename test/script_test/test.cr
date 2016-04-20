global.println('Foreach example');
global.println('shoule print a line with 0 to 99');

let i = 0;
let a = [];
while (i < 100) {
    a[i] = i;
    i = i + 1;
}

global.foreach(a, function (x) {
    global.print(x, ' ');
});
global.println();
