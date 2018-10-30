#include <iostream>
#include <vector>
#include <numeric>

#include "ttmath-0.9.3/ttmath/ttmath.h"
#include <omp.h>

#include "polynomial.hpp"

template <typename T>
T p_parallel(int n)
{
    int num_threads = omp_get_max_threads();

    std::vector<int> taylor_exponents(n);
    std::iota(std::begin(taylor_exponents), std::end(taylor_exponents), 1);

    std::vector<Polynomial<T>> subresults;
    subresults.reserve(num_threads);

    #pragma omp parallel num_threads(num_threads)
    {
        int id = omp_get_thread_num();
        int total = omp_get_num_threads();

        int from = (taylor_exponents.size() / total) * id;
        int to = (taylor_exponents.size() / total) * (id + 1);

        Polynomial<T> sub = T(1);

        for (int ii = from; ii < to; ++ii)
        {
            sub *= taylor<T>(n, taylor_exponents[ii]);
            sub = sub.drop_after(n);
        }

        #pragma omp critical
        subresults.push_back(sub);
    }

    Polynomial<T> acc = T(1);

    for (const Polynomial<T>& p : subresults)
    {
        acc *= p;
        acc = acc.drop_after(n);
    }

    return acc[n];
}


int main()
{
    int n;
    std::cout << "n = "; std::cin >> n;
    std::cout << "p(" << n << ") = " << p_parallel<ttmath::UInt<4>>(n) << std::endl;
    return 0;
}