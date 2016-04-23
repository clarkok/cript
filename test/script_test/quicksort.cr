global.println('Quicksort example');
global.println('Should output two lines, first is the random array, second is the sorted one');

let n = 1000000;
let array = [];
let i = 0;

let current_clock = global.clock();

while (i < n) {
    array[i] = global.random() % n + n;
    i = i + 1;
}

global.println('create array: ', global.clock() - current_clock);

let output_array = function (array) {
    global.foreach(array, function (x) {
        global.print(x, ' ');
    });
    global.println();
};

// output_array(array);

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

current_clock = global.clock();

export.quicksort(array, 0, n - 1, 0);
// output_array(array);

global.println('quicksort: ', global.clock() - current_clock);
