let assert = {};

assert.assert = function (cond, msg) {
    if (!cond) {
        msg = msg || '';
        if (global.typeof(msg) == 'closure') { msg == msg(); }
        global.println('Assert Failed! ', msg);
        halt;
    }
};

assert.typeEq = function (expected, actual, msg) {
    let typeof = global.typeof;

    assert.assert(
        typeof(expected) == 'string',
        'typeof expected should be string'
    );

    assert.assert(
        typeof(actual) == expected,
        msg
    );
};

assert.intEq = function (expected, actual, msg) {
    assert.typeEq('integer', expected, 'expected should be integer in assert.intEq');
    assert.typeEq('integer', actual, msg);
    assert.assert(expected == actual, msg);
};

assert.strEq = function (expected, actual, msg) {
    assert.typeEq('string', expected, 'expected should be string in assert.strEq');
    assert.typeEq('string', actual, msg);
    assert.assert(expected == actual, msg);
};

return assert;
