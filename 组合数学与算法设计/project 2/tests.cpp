#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "genperm.hpp"

#include <string>


TEST_CASE("Lexicographic permutations", "[lexicographic]")
{
    SECTION("cannot use with duplicate items")
    {
        REQUIRE_THROWS([](){
            Lexicographic<int> g = {1, 2, 3, 1, 4};
        }());
    }

    SECTION("initial permutation is correct")
    {
        Lexicographic<int> g = {1, 2, 3, 4, 5};
        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 3, 4, 5}));
    }

    SECTION("initial permutation is correct")
    {
        Lexicographic<int> g = {5, 4, 3, 2, 1};
        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({5, 4, 3, 2, 1}));
    }

    SECTION("empty set has no permutations")
    {
        Lexicographic<int> g = {};
        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("singleton set has no further permutations")
    {
        Lexicographic<int> g = {1};
        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("[1, 2, 3, 4, 5] next permutation is correct")
    {
        Lexicographic<int> g = {1, 2, 3, 4, 5};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 3, 5, 4}));
    }

    SECTION("[3, 5, 4, 1, 8, 7, 6, 2] next permutations are correct")
    {
        Lexicographic<int> g = {3, 5, 4, 1, 8, 7, 6, 2};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({3, 5, 4, 2, 1, 6, 7, 8}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({3, 5, 4, 2, 1, 6, 8, 7}));
    }

    SECTION("[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12] next permutations are correct")
    {
        Lexicographic<int> g = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 11}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 10, 12}));
    }

    SECTION("[9, 8, 7, 6, 5] no more permutations")
    {
        Lexicographic<int> g = {9, 8, 7, 6, 5};

        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("[1, 2, 3, 4, 5] has 120 (5!) permutations")
    {
        Lexicographic<int> g = {1, 2, 3, 4, 5};

        int count = 1;
        while(g.next()) ++count;

        REQUIRE(count == 120);
    }

    SECTION("[1, 2, 3, 5, 4] has 119 (5! - 1) permutations")
    {
        Lexicographic<int> g = {1, 2, 3, 5, 4};

        int count = 1;
        while(g.next()) ++count;

        REQUIRE(count == 119);
    }

    SECTION("['a', 'bc', 'cde'] next permutations are correct")
    {
        Lexicographic<std::string> g = {"a", "bc", "def"};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"a", "def", "bc"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"bc", "a", "def"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"bc", "def", "a"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"def", "a", "bc"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"def", "bc", "a"}));

        success = g.next();
        REQUIRE(success == false);
    }
}

TEST_CASE("SJT permutations", "[SJT]")
{
    SECTION("cannot use with duplicate items")
    {
        REQUIRE_THROWS([](){
            SJT<int> g = {1, 2, 3, 1, 4};
        }());
    }

    SECTION("initial permutation is correct")
    {
        SJT<int> g = {1, 2, 3, 4, 5};
        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 3, 4, 5}));
    }

    SECTION("initial permutation is correct")
    {
        SJT<int> g = {5, 4, 3, 2, 1};
        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({5, 4, 3, 2, 1}));
    }

    SECTION("empty set has no permutations")
    {
        SJT<int> g = {};
        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("singleton set has no further permutations")
    {
        SJT<int> g = {1};
        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("[1, 2, 3, 4, 5] next permutations are correct")
    {
        SJT<int> g = {1, 2, 3, 4, 5};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 3, 5, 4}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 5, 3, 4}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 5, 2, 3, 4}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({5, 1, 2, 3, 4}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({5, 1, 2, 4, 3}));
    }

    SECTION("[9, 8, 7, 6, 5] no more permutations")
    {
        SJT<int> g = {9, 8, 7, 6, 5};

        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("[1, 2, 3, 4, 5] has 120 (5!) permutations")
    {
        int count = 1;
        SJT<int> g = {1, 2, 3, 4, 5};

        while(g.next()) ++count;

        REQUIRE(count == 120);
    }

    SECTION("[1, 2, 3, 5, 4] has 119 (5! - 1) permutations")
    {
        SJT<int> g = {1, 2, 3, 5, 4};

        int count = 1;
        while(g.next()) ++count;

        REQUIRE(count == 119);
    }

    // SECTION("['a', 'bc', 'cde'] next permutations are correct")
    // {
    //     SJT<std::string> g = {"a", "bc", "def"};

    //     bool success = g.next();
    //     REQUIRE(success == true);

    //     auto perm = g.get();
    //     REQUIRE(perm == std::vector<std::string>({"a", "def", "bc"}));

    //     success = g.next();
    //     REQUIRE(success == true);

    //     perm = g.get();
    //     REQUIRE(perm == std::vector<std::string>({"bc", "a", "def"}));

    //     success = g.next();
    //     REQUIRE(success == true);

    //     perm = g.get();
    //     REQUIRE(perm == std::vector<std::string>({"bc", "def", "a"}));

    //     success = g.next();
    //     REQUIRE(success == true);

    //     perm = g.get();
    //     REQUIRE(perm == std::vector<std::string>({"def", "a", "bc"}));

    //     success = g.next();
    //     REQUIRE(success == true);

    //     perm = g.get();
    //     REQUIRE(perm == std::vector<std::string>({"def", "bc", "a"}));

    //     success = g.next();
    //     REQUIRE(success == false);
    // }
}

TEST_CASE("Inversion permutations", "[inversion]")
{
    SECTION("cannot use with duplicate items")
    {
        REQUIRE_THROWS([](){
            Inversion<int> g = {1, 2, 3, 1, 4};
        }());
    }

    SECTION("initial permutation is correct")
    {
        Inversion<int> g = {1, 2, 3, 4, 5};
        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 3, 4, 5}));
    }

    SECTION("initial permutation is correct")
    {
        Inversion<int> g = {5, 4, 3, 2, 1};
        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({5, 4, 3, 2, 1}));
    }

    SECTION("empty set has no permutations")
    {
        Inversion<int> g = {};
        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("singleton set has no further permutations")
    {
        Inversion<int> g = {1};
        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("[1, 2, 3, 4, 5] next permutation is correct")
    {
        Inversion<int> g = {1, 2, 3, 4, 5};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({2, 1, 3, 4, 5}));
    }

    SECTION("[3, 5, 4, 1, 8, 7, 6, 2] next permutations are correct")
    {
        Inversion<int> g = {3, 5, 4, 1, 8, 7, 6, 2};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<int>({3, 5, 4, 8, 1, 7, 6, 2}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({3, 5, 4, 8, 7, 1, 6, 2}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({3, 5, 4, 8, 7, 6, 1, 2}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({3, 5, 4, 8, 7, 6, 2, 1}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<int>({1, 2, 5 , 3, 4, 8, 7, 6}));
    }

    SECTION("[9, 8, 7, 6, 5] no more permutations")
    {
        Inversion<int> g = {9, 8, 7, 6, 5};

        bool success = g.next();
        REQUIRE(success == false);
    }

    SECTION("[1, 2, 3, 4, 5] has 120 (5!) permutations")
    {
        int count = 1;
        Inversion<int> g = {1, 2, 3, 4, 5};

        while(g.next()) ++count;

        REQUIRE(count == 120);
    }

    SECTION("[2, 1, 3, 4, 5] has 119 (5! - 1) permutations")
    {
        Inversion<int> g = {2, 1, 3, 4, 5};

        int count = 1;
        while(g.next()) ++count;

        REQUIRE(count == 119);
    }

    SECTION("['a', 'bc', 'cde'] next permutations are correct")
    {
        Inversion<std::string> g = {"a", "bc", "def"};

        bool success = g.next();
        REQUIRE(success == true);

        auto perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"bc", "a", "def"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"bc", "def", "a"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"a", "def", "bc"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"def", "a", "bc"}));

        success = g.next();
        REQUIRE(success == true);

        perm = g.get();
        REQUIRE(perm == std::vector<std::string>({"def", "bc", "a"}));
    }
}