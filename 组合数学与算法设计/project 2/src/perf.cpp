#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>

#include "genperm.hpp"

using nano = std::chrono::nanoseconds;

template <typename Gen>
double test(Gen gen)
{
    auto start = std::chrono::high_resolution_clock::now();
    int cnt = 0;
    while(gen.next())
    {
        // gen.get();
        ++cnt;
    }
    auto finish = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<nano>(finish - start).count() / (1. * cnt);
}

int main()
{
    auto init = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    StdLib<int>        g_std = init;
    Lexicographic<int> g_lex = init;
    SJT<int>           g_sjt = init;
    Inversion<int>     g_inv = init;

    std::cout << std::setw(5) << std::setprecision(5);
    std::cout << "StdLib:\t" << test(g_std) << " nanoseconds\n";
    std::cout << "Lexico:\t" << test(g_lex) << " nanoseconds\n";
    std::cout << "SJT:\t"    << test(g_sjt) << " nanoseconds\n";
    std::cout << "Inv:\t"    << test(g_inv) << " nanoseconds\n";

    return 0;
}