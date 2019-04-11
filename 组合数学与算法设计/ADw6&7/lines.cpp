#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <functional>

using solution_t = std::pair<int, std::string>;

solution_t solve_memoized(const std::vector<std::string>& words, int M)
{
    // <id, line> => <cost, solution>
    std::map<std::pair<int, std::string>, solution_t> memo;

    std::function<solution_t(int, std::string)> solve_rec = [&words, M, &memo, &solve_rec](int id, std::string line) -> solution_t {
        // memoization
        if (memo.count({id, line}) > 0)
            return memo[{id, line}];

        // line too long
        if (line.size() > M) return {std::numeric_limits<int>::max(), ""};

        // no more words
        if (id >= words.size()) return {0, ""};

        // a) put word in current line
        auto res_a = solve_rec(id + 1, line + (line.empty() ? "" : " ") + words[id]);
        auto cost_a = res_a.first;
        auto solution_a = res_a.second;

        // b) break line
        int diff = M - line.size();
        auto res_b = solve_rec(id + 1, words[id]);
        auto cost_b = res_b.first + (diff * diff * diff);
        auto solution_b = res_b.second;

        // compare
        auto cost = std::min(cost_a, cost_b);
        auto solution = (cost_a < cost_b)
            ? (line.empty() ? "" : " ") + words[id] + solution_a
            : ("\n" + words[id] + solution_b);

        memo[{id, line}] = {cost, solution};
        return {cost, solution};
    };

    return solve_rec(0, "");
}

int main()
{
    int N, M;

    std::cin >> N >> M;

    std::vector<std::string> words;

    for (int ii = 0; ii < N; ++ii)
    {
        std::string next;
        std::cin >> next;
        words.push_back(next);
    }

    auto x = solve_memoized(words, M);
    std::cout << x.first << std::endl;
    std::cout << x.second << std::endl;

    return 0;
}