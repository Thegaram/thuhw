# Combinatorics Project 3

## Project description

Parallelized algorithm for calculating partition numbers.

- `src/polynomial.hpp`: Polynomial library.
- `src/serial.cpp`, `src/parallel.hpp`: Serial and parallel partition number implementations.
- `src/testpoly.cpp`, `src/catch.cpp`: Tests and testing library.
- `src/ttmath-0.9.3/`: ttmath big integer library.

- `bin/*`: pre-built executables
    -  `Apple LLVM version 10.0.0 (clang-1000.10.44.2)`
    -  `target: x86_64-apple-darwin17.7.0`
    -  OpenMP extension

## How to build

Note: For using the parallel version, you will need an OpenMP-enabled version of g++/clang++ and the associated OpenMP libraries.

```
$ mkdir bin

$ g++ --std=c++11 -Wall -o bin/tests src/testpoly.cpp
$ bin/tests
===============================================================================
All tests passed (77 assertions in 3 test cases)

$ clang++ --std=c++11 -Wall -O3 -o bin/serial src/serial.cpp
$ bin/serial
n = 1000
p(1000) = 24061467864032622473692149727991

$ clang++ --std=c++11 -O3 -lomp -fopenmp -o bin/parallel src/parallel.cpp
$ bin/parallel
n = 1000
p(1000) = 24061467864032622473692149727991
```

(Note: The code uses modern C++ features like lambdas. As a result, we need to compile using C++11 or higher.)
