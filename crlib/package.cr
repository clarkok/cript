let export = {};
let undefined = global.undefined;

if (global.__modules__ == undefined) {
    global.__modules__ = {};
}

let modules = global.__modules__;
let import = global.import;

export.import = function (module) {
    if (modules[module] != undefined) { return modules[module]; }
    else {
        let ret = import(module);
        modules[module] = ret;
        return ret;
    }
};

export.imported = function (module) {
    return modules[module] != undefined;
};

return export;
