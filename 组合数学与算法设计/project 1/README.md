# Combinatorics Project 1

## Project description

This application helps us enumerate 9-digit Android-style swipe passcodes, allowing us to do security analysis.

## How to build

To build `android_code.cpp`, simply compile and run it:

```
$ g++ --std=c++11 -Wall -o android android_code.cpp
$ ./android
```

... or:

```
$ clang++ --std=c++11 -Wall -o android android_code.cpp
$ ./android
```

(Note: The code uses modern C++ features like range-for loops and lambdas. As a result, we need to compile using C++11 or higher.)

## Printing valid passcodes

Normally, the code only counts the valid passcodes. To print them, change the main function to use `print_perm`:

```c++
int main()
{
    apply_to_code_candidates([](const std::vector<int>& code) {
        if (is_valid_passcode(code))
        {
            print_perm(code);
        }
    });
}
```