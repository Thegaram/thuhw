#include <iostream>
#include <vector>

#include "genperm.hpp"

int main()
{
    int N;
    std::cout << "N = ";
    std::cin >> N;

    std::vector<int> init(N);
    std::generate(std::begin(init), std::end(init), [](){
        static int ii = 1;
        return ii++;
    });

    Lexicographic<int> g(init);
    long count = 0;

    do
    {
        if (g.get() == std::vector<int>({8, 3, 9, 6, 4, 7, 5, 2, 1}))
            break;
        // print_vector(g.get());
        ++count;
    } while (g.next());

    std::cout << "Count = " << count << std::endl;

    return 0;
}