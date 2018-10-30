#include <iostream>
#include "ttmath-0.9.3/ttmath/ttmath.h"
#include "polynomial.hpp"

template <typename T>
T p(int n)
{
    Polynomial<T> x = taylor<T>(n, 1);

    for (int ii = 2; ii <= n; ++ii)
    {
        x *= taylor<T>(n, ii);
        x = x.drop_after(n);
    }

    return x[n];
}

int main()
{
    int n;
    std::cout << "n = "; std::cin >> n;
    std::cout << "p(" << n << ") = " << p<ttmath::UInt<4>>(n) << std::endl;
    return 0;
}