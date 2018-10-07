# Combinatorics Project 2

## Project description

Three algorithms for generating permutations from a collection of items.

- `genperm.hpp`: The algorithm implementations.
- `tests.cpp`, `catch.hpp`: Unit tests and testing library.
- `perf.cpp`: Performance tests.

## How to build

```
$ g++ --std=c++11 -Wall -o tests tests.cpp
$ ./tests
===============================================================================
All tests passed (56 assertions in 3 test cases)

$ g++ --std=c++11 -Wall -o perf perf.cpp
$ ./perf
StdLib: 66.225 nanoseconds
Lexico: 71.562 nanoseconds
SJT:    438.79 nanoseconds
Inv:    40.04 nanoseconds
```

(`clang++` should also work.)

(Note: The code uses modern C++ features like range-for loops and lambdas. As a result, we need to compile using C++11 or higher.)
