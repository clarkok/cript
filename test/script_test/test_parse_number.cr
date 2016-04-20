global.println('Test parse_number');
global.println('should print 0 - 9');

let i = 0;
while (i < 10) {
    let s = global.to_string(i);
    global.print(s);
    let num = global.parse_number(s);
    global.println(' -> ', num);

    i = i + 1;
}
