#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "polynomial.hpp"


TEST_CASE("Create polynomial", "[polycreate]")
{
    SECTION("no empty polynomial")
    {
        REQUIRE_THROWS([](){
            Polynomial<int> p = {};
        }());
    }

    SECTION("constant polynomial order is 0")
    {
        Polynomial<int> p = {4};
        REQUIRE(p.order() == 0);
    }

    SECTION("linear polynomial order is 1")
    {
        Polynomial<int> p = {4, 5}; // 5x + 4
        REQUIRE(p.order() == 1);
    }

    SECTION("large polynomial order is correct")
    {
        Polynomial<int> p = {1, 2, 3, 4, 5, 6, 7, 8}; // 8x^7 + 7x^6 + ... + 2x + 1
        REQUIRE(p.order() == 7);
    }

    SECTION("large polynomial with gaps order is correct")
    {
        Polynomial<int> p = {1, 2, 0, 0, 0, 0, 7, 8}; // 8x^7 + 7x^6 + 2x + 1
        REQUIRE(p.order() == 7);
    }

    SECTION("constant polynomial coefficient is correct")
    {
        Polynomial<int> p = {4};

        REQUIRE(p[0] == 4);
        REQUIRE(p[1] == 0);
        REQUIRE(p[2] == 0);
        REQUIRE(p[3] == 0);
        REQUIRE(p[100] == 0);
    }

    SECTION("linear polynomial coefficients are correct")
    {
        Polynomial<int> p = {4, 5}; // 5x + 4

        REQUIRE(p[0] == 4);
        REQUIRE(p[1] == 5);
        REQUIRE(p[2] == 0);
        REQUIRE(p[3] == 0);
        REQUIRE(p[100] == 0);
    }

    SECTION("large polynomial coefficients are correct")
    {
        Polynomial<int> p = {1, 2, 3, 4, 5, 6, 7, 8}; // 8x^7 + 7x^6 + ... + 2x + 1

        REQUIRE(p[0] == 1);
        REQUIRE(p[1] == 2);
        REQUIRE(p[2] == 3);
        REQUIRE(p[3] == 4);
        REQUIRE(p[4] == 5);
        REQUIRE(p[5] == 6);
        REQUIRE(p[6] == 7);
        REQUIRE(p[7] == 8);
        REQUIRE(p[8] == 0);
        REQUIRE(p[9] == 0);
        REQUIRE(p[100] == 0);
    }

    SECTION("large polynomial with gaps coefficients are correct")
    {
        Polynomial<int> p = {1, 2, 0, 0, 0, 0, 7, 8}; // 8x^7 + 7x^6 + 2x + 1

        REQUIRE(p[0] == 1);
        REQUIRE(p[1] == 2);
        REQUIRE(p[2] == 0);
        REQUIRE(p[3] == 0);
        REQUIRE(p[4] == 0);
        REQUIRE(p[5] == 0);
        REQUIRE(p[6] == 7);
        REQUIRE(p[7] == 8);
        REQUIRE(p[8] == 0);
        REQUIRE(p[9] == 0);
        REQUIRE(p[100] == 0);
    }
}

TEST_CASE("Create polynomial with syntactic sugar", "[polycreate_sugar]")
{
    using namespace poly; // for x, x2, x3, ...

    SECTION("constant polynomial order is 0")
    {
        Polynomial<int> p = 4;
        REQUIRE(p.order() == 0);
    }

    SECTION("linear polynomial order is 1")
    {
        Polynomial<int> p = 5*x + 4;
        REQUIRE(p.order() == 1);
    }

    SECTION("large polynomial order is correct")
    {
        Polynomial<int> p = 8*x7 + 7*x6 + 6*x5 + 5*x4 + 4*x3 + 3*x2 + 2*x + 1;
        REQUIRE(p.order() == 7);
    }

    SECTION("large polynomial with gaps order is correct")
    {
        Polynomial<int> p = 8*x7 + 7*x6 + 2*x + 1;
        REQUIRE(p.order() == 7);
    }

    SECTION("constant polynomial coefficient is correct")
    {
        Polynomial<int> p = 4;

        REQUIRE(p[0] == 4);
        REQUIRE(p[1] == 0);
        REQUIRE(p[2] == 0);
        REQUIRE(p[3] == 0);
        REQUIRE(p[100] == 0);
    }

    SECTION("linear polynomial coefficients are correct")
    {
        Polynomial<int> p = 5*x + 4;

        REQUIRE(p[0] == 4);
        REQUIRE(p[1] == 5);
        REQUIRE(p[2] == 0);
        REQUIRE(p[3] == 0);
        REQUIRE(p[100] == 0);
    }

    SECTION("large polynomial coefficients are correct")
    {
        Polynomial<int> p = 8*x7 + 7*x6 + 6*x5 + 5*x4 + 4*x3 + 3*x2 + 2*x + 1;

        REQUIRE(p[0] == 1);
        REQUIRE(p[1] == 2);
        REQUIRE(p[2] == 3);
        REQUIRE(p[3] == 4);
        REQUIRE(p[4] == 5);
        REQUIRE(p[5] == 6);
        REQUIRE(p[6] == 7);
        REQUIRE(p[7] == 8);
        REQUIRE(p[8] == 0);
        REQUIRE(p[9] == 0);
        REQUIRE(p[100] == 0);
    }

    SECTION("large polynomial with gaps coefficients are correct")
    {
        Polynomial<int> p = 8*x7 + 7*x6 + 2*x + 1;

        REQUIRE(p[0] == 1);
        REQUIRE(p[1] == 2);
        REQUIRE(p[2] == 0);
        REQUIRE(p[3] == 0);
        REQUIRE(p[4] == 0);
        REQUIRE(p[5] == 0);
        REQUIRE(p[6] == 7);
        REQUIRE(p[7] == 8);
        REQUIRE(p[8] == 0);
        REQUIRE(p[9] == 0);
        REQUIRE(p[100] == 0);
    }
}

TEST_CASE("Multiplication", "[polymult]")
{
    using namespace poly; // for x, x2, x3, ...

    SECTION("multiply constants")
    {
        REQUIRE(Polynomial<int>(4) * 9 == 36);
    }

    SECTION("multiply constant with linear")
    {
        REQUIRE(4 * (9*x + 2) == (36*x + 8));
    }

    SECTION("multiply linear with linear")
    {
        REQUIRE((3*x + 4) * (2*x + 9) == (6*x2 + 35*x + 36));
    }

    SECTION("multiply long polynomials")
    {
        Polynomial<int> p1  = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        Polynomial<int> p2  = {11, 12, 13, 14, 15, 16, 17, 18, 19};
        Polynomial<int> res = {11, 34, 70, 120, 185, 266, 364, 480, 615, 640, 644, 626, 585, 520, 430, 314, 171};

        REQUIRE(p1 * p2 == res);
    }
}