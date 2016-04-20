# Cript

A simple and lightweight script language.

## Language feature

 * supported types
   * 30bits signed integer
   * 256byte-max-length internal string
   * hash-based Object and Array
   * value-captured Closure
   * lightweight C function
   * GC managed userdata
 * dynamic types
 * full support of recursion
 * on-the-fly import
 * call script functions from C functions and vise-versa

## Virtual machine feature

 * copy-and-swap GC
 * register-based VM
 * SSA-like IR
 * able to register C functions
 * zero dependency except libc

## Current limitation

 * only support `while` loop and `if` branch statement
 * heap size is hardcoded as 1MB
 * single thread
 * 32bits VM

## Compile

```
git clone https://github.com/clarkok/cript.git
cd cript
mkdir build
cd build
cmake ../
make
```

## Run script

```
./path/to/cript ./path/to/script.cr
```

## Hello world!

```
// save it to hello_world.cr and run it!
global.println('Hello World!');
```

## Example scripts

See `test/script_test`, there is enough examples.