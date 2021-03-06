## Introduction

This project implements a multithreaded library for reducing n-dimensional arrays of integers (where each dimension's length is a power of 2) through modal downsampling.

## Dependencies

This project uses the following:

- [Marray](http://www.andres.sc/marray.html) (developed against commit e4c3d117f8e947642da714207a050c1ceb22006f)

## Getting Started

There are a couple of demos in the `src/demo` directory where you can see examples of usage.

Build example (from the project root directory):

```
mkdir ./build
g++ -O3 -I./include -I./path/to/marray -std=c++14 -o build/1024x1024.out src/demo/1024x1024.cpp src/functions.cpp -lpthread
```
**Note:** The only implementation file you have to compile currently is `src/functions.cpp`.

Just run the file output by the compiler (e.g. `rand_img.out` in the example) in your terminal.

The `process_image()` function will be your primary interface. All you need to do is create an `Image` object and pass it in.

If you want to output results, `write_to_file()` takes an `Image` object and a filepath string and writes the image data to that file. Since the data are not guaranteed to be in a neatly presentable dimensionality, a CSV with index-value pairs is written. The first line should indicate the shape of the image being written.

As an example:
```
[2,16]
<0,0>,1
<1,0>,1
<0,1>,0
<1,1>,0
<0,2>,1
<1,2>,1
<0,3>,0
<1,3>,1
<0,4>,1
<1,4>,0
<0,5>,0
<1,5>,1
<0,6>,1
<1,6>,1
<0,7>,1
<1,7>,1
<0,8>,1
<1,8>,1
<0,9>,1
<1,9>,1
<0,10>,1
<1,10>,1
<0,11>,1
<1,11>,1
<0,12>,1
<1,12>,1
<0,13>,0
<1,13>,1
<0,14>,1
<1,14>,1
<0,15>,1
<1,15>,1
```
