// compile using: $ g++ --std=c++14 android_code.cpp

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

//  --- --- ---
// | 1 | 2 | 3 |
//  --- --- ---
// | 4 | 5 | 6 |
//  --- --- ---
// | 7 | 8 | 9 |
//  --- --- ---

bool is_valid_passcode(const std::vector<int>& code)
{
    bool is_set[10] = { false };

    int current;
    int last = current = code[0];
    is_set[current] = true;

    for (int ii = 1; ii < code.size(); ++ii)
    {
        last = current;
        current = code[ii];

        if (last == 1 && current == 3 && !is_set[2]) return false;
        if (last == 1 && current == 7 && !is_set[4]) return false;
        if (last == 1 && current == 9 && !is_set[5]) return false;

        if (last == 2 && current == 8 && !is_set[5]) return false;

        if (last == 3 && current == 1 && !is_set[2]) return false;
        if (last == 3 && current == 7 && !is_set[5]) return false;
        if (last == 3 && current == 9 && !is_set[6]) return false;

        if (last == 4 && current == 6 && !is_set[5]) return false;

        if (last == 6 && current == 4 && !is_set[5]) return false;

        if (last == 7 && current == 1 && !is_set[4]) return false;
        if (last == 7 && current == 3 && !is_set[5]) return false;
        if (last == 7 && current == 9 && !is_set[8]) return false;

        if (last == 8 && current == 2 && !is_set[5]) return false;

        if (last == 9 && current == 1 && !is_set[5]) return false;
        if (last == 9 && current == 3 && !is_set[6]) return false;
        if (last == 9 && current == 7 && !is_set[8]) return false;

        is_set[current] = true;
    }

    return true;
}

// generate all permutations of `nums` and apply `func` to each
template <typename F>
void apply_to_all_permutations(std::vector<int> nums, F func)
{
    do {
        func(nums);
    } while (std::next_permutation(nums.begin(), nums.end()));
}

// generate all P(`N`, `K`) and apply `func` to each
// source: https://stackoverflow.com/a/28698654
template <typename F>
void apply_to_all_permutations(int N, int K, F func)
{
    std::string bitmask(K, 1); // K leading 1's
    bitmask.resize(N, 0); // N-K trailing 0's

    // print integers and permute bitmask
    do {
        std::vector<int> v;

        for (int ii = 0; ii < N; ++ii) // [0..N-1] integers
        {
            if (bitmask[ii])
                v.push_back(ii + 1);
        }

        apply_to_all_permutations(v, func);
    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
}

template <typename F>
void apply_to_code_candidates(F func)
{
    for (int ii = 4; ii < 10; ++ii)
        apply_to_all_permutations(9, ii, func);
}

void print_perm(const std::vector<int>& nums)
{
    for (int n : nums)
        std::cout << n << " ";
    std::cout << std::endl;
}

int main()
{
    int count = 0;
    apply_to_code_candidates([&count](const std::vector<int>& code) {
        if (is_valid_passcode(code))
        {
            ++count;
            print_perm(code);
        }
    });
    std::cout << "Count = " << count << std::endl;

    return 0;
}
