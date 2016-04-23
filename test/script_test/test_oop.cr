global.print('Test OOP... ');

let Student = function (name, age) {
    let ret = {};
    ret.name = name;
    ret.age = age;

    ret.getName = function () {
        return this.name;
    };

    ret.getAge = function () {
        return this.age;
    };

    return ret;
};

let student = Student('Clarkok Zhang', 20);

let assert = global.import('./assert.cr');

assert.assertStringEq('Clarkok Zhang', student.getName());
assert.assertIntEq(20, student.getAge());

global.println('Done');
