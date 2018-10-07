#include <algorithm>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <vector>

template <typename T>
class PermutationGenerator
{
public:
    virtual bool next() = 0;
    virtual std::vector<T> get() const = 0;

protected:
    // source: https://stackoverflow.com/a/2769192
    template <typename C>
    static bool is_unique(C container)
    {
        std::sort(container.begin(), container.end());
        return std::unique(std::begin(container), std::end(container)) == std::end(container);
    }
};

template <typename T>
class StdLib : public PermutationGenerator<T>
{
public:
    StdLib(const std::initializer_list<T>& init): current(init) {}

    std::vector<T> get() const override
    {
        return current;
    }

    bool next() override
    {
        return std::next_permutation(std::begin(current), std::end(current));
    }

private:
    std::vector<T> current;
};

template <typename T>
class Lexicographic : public PermutationGenerator<T>
{
public:
    Lexicographic(const std::vector<T>& init): current(init)
    {
        if (!this->is_unique(init))
        {
            throw std::invalid_argument("Lexicographic can only work with unique items");
        }
    }

    Lexicographic(const std::initializer_list<T>& init): Lexicographic(std::vector<T>(init)) {}

    std::vector<T> get() const override
    {
        return current;
    }

    bool next() override
    {
        int N = current.size();

        if (N <= 2)
            return false;

        // find pivot (first descending)
        int pivot = N - 2;
        while (pivot >= 0 && current[pivot] > current[pivot + 1])
            --pivot;
        if (pivot < 0)
            return false;

        // find first just larger
        T just_larger = current[pivot + 1];
        int just_larger_id = pivot + 1;

        for (int ii = pivot + 2; ii < N; ++ii)
        {
            if (current[ii] > current[pivot] && current[ii] < just_larger)
            {
                just_larger = current[ii];
                just_larger_id = ii;
            }
        }

        // swap
        std::swap(current[pivot], current[just_larger_id]);

        // get smallest suffix
        std::sort(std::begin(current) + pivot + 1, std::end(current));

        return true;
    }

private:
    std::vector<T> current;
};

template <typename T>
class SJT : public PermutationGenerator<T>
{
private:
    static constexpr bool LEFT = false;
    static constexpr bool RIGHT = true;

public:
    SJT(const std::vector<T>& init): current(init), arrows(std::vector<bool>(init.size(), LEFT))
    {
        if (!this->is_unique(init))
        {
            throw std::invalid_argument("SJT can only work with unique items");
        }
    }

    SJT(const std::initializer_list<T>& init): SJT(std::vector<T>(init)) {}

    std::vector<T> get() const override
    {
        return current;
    }

    bool next() override
    {
        int N = current.size();

        if (N <= 2)
            return false;

        // find largest mobile
        T m = -1; // TODO: review (this only works for non-negative integers)
        int m_id = -1;
        for (int ii = 0; ii < N; ++ii)
        {
            if (is_mobile(ii) && current[ii] > m)
            {
                m = current[ii];
                m_id = ii;
            }
        }

        if (m_id == -1)
            return false;

        // swap
        if (arrows[m_id] == LEFT)
        {
            std::swap(current[m_id - 1], current[m_id]);
            std::swap(arrows[m_id - 1], arrows[m_id]);
        }
        else
        {
            std::swap(current[m_id + 1], current[m_id]);
            std::swap(arrows[m_id + 1], arrows[m_id]);
        }

        // switch directions
        for (int ii = 0; ii < N; ++ii)
        {
            if (current[ii] > m)
                arrows[ii] = (arrows[ii] == LEFT ? RIGHT : LEFT);
        }

        return true;
    }

private:
    std::vector<T> current;
    std::vector<bool> arrows;

    bool is_mobile(int id) const
    {
        return ((arrows[id] == LEFT)  && (id > 0)                  && (current[id - 1] < current[id])) ||
               ((arrows[id] == RIGHT) && (id < current.size() - 1) && (current[id + 1] < current[id]));
    }
};

template <typename T> constexpr bool SJT<T>::LEFT;
template <typename T> constexpr bool SJT<T>::RIGHT;

template <typename T>
class Inversion : public PermutationGenerator<T>
{
public:
    Inversion(const std::vector<T>& init)
    {
        if (!this->is_unique(init))
        {
            throw std::invalid_argument("Inversion can only work with unique items");
        }

        // store initial inversion
        for (auto it = std::begin(init); it != std::end(init); ++it)
        {
            // count corresponding inversions
            int count = 0;
            for (auto it2 = std::begin(init); it2 != it; ++it2)
            {
                if (*it2 > *it)
                    ++count;
            }

            inversion.insert(std::pair<T, int>(*it, count));
        }
    }

    Inversion(const std::initializer_list<T>& init): Inversion(std::vector<T>(init)) {}

    std::vector<T> get() const override
    {
        const int N = inversion.size();
        const T FREE = T(); // TODO: review (this restricts the set of possible items)
        std::vector<T> perm(N, FREE);

        for (auto entry : inversion)
        {
            T digit = entry.first;
            int invCount = entry.second;

            int ii = 0; int numFree = 0;
            while(numFree < invCount || perm[ii] != FREE)
            {
                if (perm[ii] == FREE)
                    ++numFree;
                ++ii;
            }

            perm[ii] = digit;
        }

        return perm;
    }

    bool next() override
    {
        return incrementInversion(std::begin(inversion), std::end(inversion), inversion.size());
    }

private:
    std::map<T, int> inversion;

    template <typename Iter>
    bool incrementInversion(Iter it, Iter end, int max)
    {
        if (it == end)
            return false;

        int& count = (*it).second;

        if (count + 1 < max)
        {
            ++count;
            return true;
        }
        else
        {
            count = 0;
            return incrementInversion(++it, end, max - 1);
            // note: as `inversion` tends to have just a few elements,
            //       recursion should not be an issue. Also, most compilers
            //       implement tail-call optimization.
        }
    }
};
