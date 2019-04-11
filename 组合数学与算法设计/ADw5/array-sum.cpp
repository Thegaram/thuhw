#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <numeric>

int solve_naive(const std::vector<int>& nums, int lower, int upper)
{
    int count = 0;
    auto begin = std::begin(nums);

    for (int ii =  0; ii < nums.size(); ++ii)
    for (int jj = ii; jj < nums.size(); ++jj)
    {
        int sum = std::accumulate(begin + ii, begin + jj + 1, 0);
        if (sum >= lower && sum <= upper) ++count;
    }

    return count;
}

int solve_rec(std::vector<int>& sums, int lower, int upper, int begin, int end)
{
    if (begin >= end)     return 0;
    if (begin == end - 1) return (sums[begin] >= lower && sums[begin] <= upper);

    int middle = (begin + end) / 2;
    int left_count  = solve_rec(sums, lower, upper, begin, middle);
    int right_count = solve_rec(sums, lower, upper, middle, end);

    int middle_count = 0;
    for (int ii = begin, jj = middle, kk = middle; ii < middle; ++ii)
    {
        // looking for ranges [jj..kk] in the right half
        // s.t. (S[jj] - S[ii]), ..., (S[kk-1] - S[ii]) are all valid pairs
        while (jj < end && (sums[jj] - sums[ii] <  lower)) ++jj;
        while (kk < end && (sums[kk] - sums[ii] <= upper)) ++kk;
        middle_count += kk - jj;
    }

    auto b = std::begin(sums);
    std::inplace_merge(b + begin, b + middle, b + end);

    return left_count + right_count + middle_count;
}

int solve_dac(const std::vector<int>& nums, int lower, int upper)
{
    if (nums.empty()) return 0;

    // calculate prefix sums
    std::vector<int> sums(nums.size());
    std::partial_sum(std::begin(nums), std::end(nums), std::begin(sums));

    return solve_rec(sums, lower, upper, 0, sums.size());
}

std::vector<int> read_problem(int& lower, int& upper)
{
    int N;
    std::vector<int> nums;

    std::cin >> N;
    nums.reserve(N);

    for (int ii = 0; ii < N; ++ii)
    {
        int next;
        std::cin >> next;
        nums.push_back(next);
    }

    std::cin >> lower >> upper;

    return nums;
}

int main()
{
    int lower, upper;
    auto nums = read_problem(lower, upper);

    int result = solve_dac(nums, lower, upper);
    std::cout << result << std::endl;

    return 0;
}