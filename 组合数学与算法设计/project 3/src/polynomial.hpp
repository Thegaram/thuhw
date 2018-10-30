#include <iostream>
#include <initializer_list>
#include <vector>
#include <algorithm>
#include <exception>

template <typename T>
class Polynomial
{
public:
    Polynomial(const std::vector<T> cs): coefficients(cs)
    {
        if (cs.size() == 0)
        {
            throw std::invalid_argument("cannot initialize empty polynomial");
        }
    }

    Polynomial(const std::initializer_list<T> cs): Polynomial(std::vector<T>(cs)) {}

    // allow implicit conversion
    Polynomial(const T& c): Polynomial({c}) {}

    template <typename F>
    Polynomial(int order, F gen): coefficients(std::vector<T>(order + 1, T(0)))
    {
        for (int ii = 0; ii <= order; ++ii)
            coefficients[ii] = gen(ii);
    }

    T operator[](int index) const
    {
        return (index <= this->order()) ? coefficients[index] : T(0);
    }

    int order() const
    {
        return coefficients.size() - 1;
    }

    Polynomial<T> operator*(const Polynomial<T>& p) const
    {
        int N = this->order();
        int M = p.order();
        int newOrder = N + M;

        std::vector<T> cs(newOrder + 1, T(0));

        for (int ii = 0; ii <= N; ++ii)
        for (int jj = 0; jj <= M; ++jj)
        {
            int exp = ii + jj;
            cs[exp] += this->operator[](ii) * p[jj];
        }

        return Polynomial<T>(cs);
    }

    Polynomial<T>& operator*=(const Polynomial<T>& p)
    {
        return *this = *this * p;
    }

    Polynomial<T> operator+(const Polynomial<T>& p) const
    {
        int N = this->order();
        int M = p.order();
        int newOrder = std::max(N, M);

        std::vector<T> cs(newOrder + 1, T(0));

        for (int ii = 0; ii <= newOrder; ++ii)
        {
            cs[ii] = this->operator[](ii) + p[ii];
        }

        return Polynomial<T>(cs);
    }

    Polynomial<T>& operator+=(const Polynomial<T>& p)
    {
        return *this = *this + p;
    }

    Polynomial<T> drop_after(int exp) const
    {
        auto first = std::begin(coefficients);
        auto last = first + exp + 1;
        return Polynomial<T>(std::vector<T>(first, last));
    }

    bool operator==(const Polynomial<T>& p) const
    {
        if (this->order() != p.order())
        {
            return false;
        }

        for (int ii = 0; ii <= this->order(); ++ii)
        {
            if (this->operator[](ii) != p[ii])
                return false;
        }

        return true;
    }

private:
    std::vector<T> coefficients;
};

template <typename T>
Polynomial<T> operator*(const T& c, const Polynomial<T>& p)
{
    return p * c;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Polynomial<T>& p)
{
    int order = p.order();

    T first = p[order];
    if (first != 0)
    {
        if (first != 1)
            os << first;

        os << "x";

        if (order != 1)
            os << "^" << order;
    }

    for (int ii = order - 1; ii >= 1; --ii)
    {
        T coeff = p[ii];
        if (coeff != 0)
        {
            os << " + ";

            if (coeff != 1)
                os << coeff << "x";

            if (ii != 1)
                os << "^" << ii;
        }
    }

    T last = p[0];
    if (order > 0 && last > 0)
        os << " + " << last;

    return os;
}

// generate 1 + x^d + x^2d + ... up to x^order
template <typename T>
Polynomial<T> taylor(int order, int d)
{
    return Polynomial<T>(order, [d](int index){
        return (index % d == 0) ? 1 : 0;
    });
}

namespace poly {
auto x  = Polynomial<int>({0, 1});
auto x2 = Polynomial<int>({0, 0, 1});
auto x3 = Polynomial<int>({0, 0, 0, 1});
auto x4 = Polynomial<int>({0, 0, 0, 0, 1});
auto x5 = Polynomial<int>({0, 0, 0, 0, 0, 1});
auto x6 = Polynomial<int>({0, 0, 0, 0, 0, 0, 1});
auto x7 = Polynomial<int>({0, 0, 0, 0, 0, 0, 0, 1});
auto x8 = Polynomial<int>({0, 0, 0, 0, 0, 0, 0, 0, 1});
auto x9 = Polynomial<int>({0, 0, 0, 0, 0, 0, 0, 0, 0, 1});
}