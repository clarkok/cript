global.println("Test to_string");
global.println("should print 0 - 9");

let i = 0;
while (i < 10) {
    global.println(global.to_string(i));
    i = i + 1;
}
