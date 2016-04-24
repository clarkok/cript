let undefined = global.undefined;

if (global.package == undefined) {
    global.package = global.import('package.cr');
    global.import = undefined;
}

return undefined;
