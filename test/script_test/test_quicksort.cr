global.print('Test quicksort... ');

let n = 100000;
let array = [];
let i = 0;

let count = [];

while (i < n) {
    array[i] = global.random() % n + n;
    count[array[i]] = (count[array[i]] || 0) + 1;
    i = i + 1;
}

let export = {};

export.quicksort = function (arr, lo, hi, dep) {
    if (lo < hi) {
        let p = export.partition(arr, lo, hi);
        export.quicksort(arr, lo, p - 1, dep + 1);
        export.quicksort(arr, p + 1, hi, dep + 1);
    }
};

export.partition = function (arr, lo, hi) {
    let pivot = arr[hi];
    let i = lo;
    let j = lo;
    while (j < hi) {
        if (arr[j] <= pivot) {
            let t = arr[i];
            arr[i] = arr[j];
            arr[j] = t;
            i = i + 1;
        }
        j = j + 1;
    }
    let t = arr[i];
    arr[i] = arr[hi];
    arr[hi] = t;
    return i;
};

export.quicksort(array, 0, n - 1, 0);

let assert = global.import('./assert.cr');
i = 1;
assert.assert(count[array[0]]);
count[array[0]] = count[array[0]] - 1;
while (i < n) {
    assert.assert(
        array[i - 1] <= array[i],
        function () { return global.concat('error on ', global.to_string(i)); }
    );
    assert.assert(count[array[i]]);
    count[array[i]] = count[array[i]] - 1;
    i = i + 1;
}

i = 0;
while (i < n) {
    assert.assert(
        !count[array[i]],
        function () {
            return global.concat('number ', global.to_string(array[i]), ' is ', global.to_string(i));
        }
    );
    i = i + 1;
}

global.println('Done');
