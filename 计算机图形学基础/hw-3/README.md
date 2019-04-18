# FCG - HW3

![screenshot](screenshot.png)

OpenGL (3.3+), GLAD, GLFW3, GLM, SOIL, FreeType.

## How to build?

Make sure you installed all the dependencies. To build and run, execute the following commands.

```
$ g++ --std=c++14 -lglfw3 -lglad -lsoil -lfreetype -O3 -Wall -o main main.cpp
$ ./main
```

## Tested on

```
Apple LLVM version 10.0.1 (clang-1001.0.46.3)
Target: x86_64-apple-darwin18.5.0
```