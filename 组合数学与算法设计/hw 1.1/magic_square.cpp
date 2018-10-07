#include <iostream>
#include <iomanip>
#include <vector>
#include <set>

#define N 4
#define M 34

//  ---- ---- ---- ----
// |  1 |  2 |  3 |  4 |
//  ---- ---- ---- ----
// |  5 |  6 |  7 |  8 |
//  ---- ---- ---- ----
// |  9 | 10 | 11 | 12 |
//  ---- ---- ---- ----
// | 13 | 14 | 15 | 16 |
//  ---- ---- ---- ----

bool satisfies_base_constraints(const std::vector<int>& c, int target)
{
    if (target ==  5) return ((c[ 1] + c[ 2] + c[ 3] + c[ 4]) == M);   // row  1
    if (target ==  9) return ((c[ 5] + c[ 6] + c[ 7] + c[ 8]) == M);   // row  2
    if (target == 13) return ((c[ 9] + c[10] + c[11] + c[12]) == M);   // row  3

    if (target == 14) return ((c[ 1] + c[ 5] + c[ 9] + c[13]) == M) && // col  1
                             ((c[ 4] + c[ 7] + c[10] + c[13]) == M);   // diag 1

    if (target == 15) return ((c[ 2] + c[ 6] + c[10] + c[14]) == M);   // col  2
    if (target == 16) return ((c[ 3] + c[ 7] + c[11] + c[15]) == M);   // col  3

    if (target == 17) return ((c[ 4] + c[ 8] + c[12] + c[16]) == M) && // col  4
                             ((c[ 1] + c[ 6] + c[11] + c[16]) == M);   // diag 2

    // note: there are many other constraints we could apply,
    //       e.g. checking for overly large sums for unfinished
    //       rows/cols, etc.

    return true;
}

bool satisfies_special_constraints(const std::vector<int>& c, int target)
{
    if (target ==  2) return (c[1] == 2);
    if (target ==  3) return (c[2] == 3);
    if (target ==  6) return (c[5] == 4);

    return true;
}

void print_square(const std::vector<int>& square)
{
    for (int row = 0; row < N; ++row)
    {
        for (int col = 0; col < N; ++col)
        {
            int position = (row * N + col) + 1;
            std::cout << std::setw(2) << square[position] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template <typename F>
void solve(std::vector<int>& candidate, std::set<int>& used, int target, F func)
{
    // backtrack
    if (!satisfies_base_constraints(candidate, target) || ! satisfies_special_constraints(candidate, target)){
        return;
    }

    if (target == (N * N + 1)){
        func(candidate);
        return;
    }

    for (int ii = 1; ii <= (N * N); ++ii)
    {
        // we can use each value only once
        if (used.find(ii) != used.end())
            continue;

        used.insert(ii);
        candidate[target] = ii;
        solve(candidate, used, target + 1, func);
        used.erase(used.find(ii));
    }
}

int get_count()
{
    std::vector<int> candidate((N * N + 1), 0);
    std::set<int> used;
    int count = 0;

    solve(candidate, used, 1, [&count](auto _) {
        ++count;
    });

    return count;
}

void print_each()
{
    std::vector<int> candidate((N * N + 1), 0);
    std::set<int> used;

    solve(candidate, used, 1, [](const std::vector<int>& solution) {
        print_square(solution);
    });
}

int main()
{
    // print_each();
    std::cout << "Count = " << get_count() << std::endl;

    return 0;
}