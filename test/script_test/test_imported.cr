let export = {};

export.hello = function () {
    global.println('Hello Import');
};

export.export2 = global.import('./test_imported2.cr');

return export;
