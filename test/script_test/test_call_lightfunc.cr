global.println('Test use light function as callback');
global.println('should println 0 - 9');

let a = [],
    i = 0;
while (i < 10) {
    a[i] = i;
    i = i + 1;
}

global.foreach(a, global.println);
