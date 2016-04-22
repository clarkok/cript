global.print('Test parse_number... ');

let assert = global.import('assert.cr');

let i = 0;
while (i < 10) {
    let s = global.to_string(i);
    let num = global.parse_number(s);
    assert.assertIntEq(i, num, 'i should equal to num');

    i = i + 1;
}

global.println('Done');
