global.println('Test OOP');

let Student = function (name, age) {
    let ret = {};
    ret.name = name;
    ret.age = age;

    ret.sayName = function () {
        global.println(this.name);
    };

    return ret;
};

let student = Student('Clarkok Zhang', 20);

student.sayName();
