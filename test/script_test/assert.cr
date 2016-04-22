let export = {};

export.assert = function (cond, msg) {
    msg = msg || '';
    if (!cond) {
        global.println('Assert Failed! ', msg);
        halt;
    }
};

export.assertTypeof = function (expected, actual, msg) {
    msg = msg || '';
    export.assert(
        global.typeof(expected) == 'string',
        'typeof expected should be string'
    );

    export.assert(
        global.typeof(actual) == expected,
        global.concat(
            msg,
            ' expected type:',
            expected,
            ' but actual type: ',
            global.typeof(actual)
        )
    );
};

export.assertIntEq = function (expected, actual, msg) {
    msg = msg || '';
    export.assertTypeof('integer', expected, '`expected`');
    export.assertTypeof('integer', actual, '`actual`');

    export.assert(
        expected == actual, 
        global.concat(
            msg,
            ' expect: ',
            global.to_string(expected),
            ' but actual: ',
            global.to_string(actual)
        )
    );
};

export.assertStringEq = function (expected, actual, msg) {
    msg = msg || '';
    export.assertTypeof('string', expected, '`expected`');
    export.assertTypeof('string', actual, '`actual`');

    export.assert(
        expected == actual,
        global.concat(
            msg,
            ' expect: "',
            expected,
            '" but actual: "',
            actual,
            '"'
        )
    );
};

return export;
