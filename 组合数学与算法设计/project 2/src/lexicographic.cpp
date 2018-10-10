#include <iostream>
#include <vector>

#include "genperm.hpp"

template <typename T>
void print_vector(const std::vector<T>& vec)
{
    for (T item : vec)
        std::cout << item << " ";
    std::cout << std::endl;
}

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
        print_vector(g.get());
        ++count;
    } while (g.next());

    std::cout << "Count = " << count << std::endl;

    return 0;
}