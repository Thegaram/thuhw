# Combinatorics Project 2

## Project description

Three algorithms for generating permutations from a collection of items.

- `src/genperm.hpp`: The algorithm implementations.
- `src/tests.cpp`, `src/catch.hpp`: Unit tests and testing library.
- `src/perf.cpp`: Performance tests.
- `src/lexicographic.cpp`, `src/sjt.cpp`, `src/inversion.cpp`: Example applications.

- `bin/*`: pre-built executables
    -  `Apple LLVM version 10.0.0 (clang-1000.10.44.2)`
    -  `target: x86_64-apple-darwin17.7.0`

## How to build

```
$ mkdir bin

$ g++ --std=c++11 -Wall -o bin/tests src/tests.cpp
$ bin/tests
===============================================================================
All tests passed (56 assertions in 3 test cases)

$ g++ --std=c++11 -Wall -o bin/perf src/perf.cpp
$ bin/perf
StdLib: 66.225 nanoseconds
Lexico: 71.562 nanoseconds
SJT:    438.79 nanoseconds
Inv:    40.04 nanoseconds

$ g++ --std=c++11 -Wall -o bin/lexicographic src/lexicographic.cpp
$ g++ --std=c++11 -Wall -o bin/sjt src/sjt.cpp
$ g++ --std=c++11 -Wall -o bin/inversion src/inversion.cpp
$ bin/lexicographic
N = 3
1 2 3
1 3 2
2 1 3
2 3 1
3 1 2
3 2 1
Count = 6
```

(Note: The code uses modern C++ features like range-for loops and lambdas. As a result, we need to compile using C++11 or higher.)
