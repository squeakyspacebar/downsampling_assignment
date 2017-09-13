## Introduction

This project implements a library for reducing n-dimensional arrays of integers (where each dimension's length is a power of 2) through modal downsampling.

## Dependencies

This project uses the following:

- [Boost](http://www.boost.org/) (developed against v1.65.1) -- Boost will need to be built with the Boost.Thread library.
- [Marray](http://www.andres.sc/marray.html) (developed against commit e4c3d117f8e947642da714207a050c1ceb22006f)

## Getting Started

There are a couple of demos in the `src/demo` directory where you can see examples of usage.

Build example (from the project root directory):

```
mkdir ./build
g++ -I./include -I./path/to/boost -I./path/to/marray -L./path/to/boost/lib -std=c++14 -o build/rand_img.out src/demo/rand_img.cpp src/functions.cpp -lboost_system -lboost_thread -lpthread
```
**Note:** The only implementation file you have to compile currently is `src/functions.cpp`.

Just run the file output by the compiler (e.g. `rand_img.out` in the example) in your terminal.

The `process_image()` function will be your primary interface. All you need to do is create an `Image` object and pass it to the image function with the level of downsampling `l` that you want, where the original image is downsampled by a window where each side is of length `2^l`.
