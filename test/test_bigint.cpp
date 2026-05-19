#include "bigint.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>

static int tests = 0, passed = 0;

#define CHECK(cond, msg) do { \
    tests++; \
    if (cond) { passed++; } \
    else { fprintf(stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); std::exit(1); } \
} while(0)

// ============ 构造和基本属性 ============
void test_construction() {
    BigInt zero;
    CHECK(zero.is_zero(), "default constructor should be zero");
    CHECK(!zero.is_one(), "zero is not one");

    BigInt one(1);
    CHECK(one.is_one(), "BigInt(1) should be one");
    CHECK(!one.is_zero(), "one is not zero");

    BigInt two(2);
    CHECK(!two.is_zero(), "two is not zero");
    CHECK(!two.is_one(), "two is not one");
}

// ============ mod4 / mod8 ============
void test_mod4_mod8() {
    CHECK(BigInt(0).mod4() == 0, "0 mod 4 = 0");
    CHECK(BigInt(0).mod8() == 0, "0 mod 8 = 0");
    CHECK(BigInt(1).mod4() == 1, "1 mod 4 = 1");
    CHECK(BigInt(1).mod8() == 1, "1 mod 8 = 1");
    CHECK(BigInt(3).mod4() == 3, "3 mod 4 = 3");
    CHECK(BigInt(7).mod8() == 7, "7 mod 8 = 7");
    CHECK(BigInt(8).mod8() == 0, "8 mod 8 = 0");
    CHECK(BigInt(9).mod4() == 1, "9 mod 4 = 1");
    CHECK(BigInt(10).mod8() == 2, "10 mod 8 = 2");
    CHECK(BigInt(15).mod4() == 3, "15 mod 4 = 3");
    CHECK(BigInt(15).mod8() == 7, "15 mod 8 = 7");

    // 大数测试
    BigInt big42("42");
    CHECK(big42.mod4() == 2, "42 mod 4 = 2");
    CHECK(big42.mod8() == 2, "42 mod 8 = 2");

    BigInt big255("255");
    CHECK(big255.mod4() == 3, "255 mod 4 = 3");
    CHECK(big255.mod8() == 7, "255 mod 8 = 7");
}

// ============ 比较 ============
void test_compare() {
    CHECK(BigInt(5).cmp(BigInt(3)) == 1, "5 > 3");
    CHECK(BigInt(3).cmp(BigInt(5)) == -1, "3 < 5");
    CHECK(BigInt(42).cmp(BigInt(42)) == 0, "42 == 42");
    CHECK(BigInt(0).cmp(BigInt(0)) == 0, "0 == 0");
    CHECK(BigInt(1).cmp(BigInt(0)) == 1, "1 > 0");
    CHECK(BigInt(0).cmp(BigInt(1)) == -1, "0 < 1");

    // 多字测试
    BigInt big1("18446744073709551616");  // 2^64
    BigInt big2("18446744073709551617");  // 2^64 + 1
    CHECK(big1.cmp(big2) == -1, "2^64 < 2^64+1");
    CHECK(big2.cmp(big1) == 1, "2^64+1 > 2^64");
    CHECK(big1.cmp(big1) == 0, "2^64 == 2^64");
}

// ============ 减法 ============
void test_sub() {
    BigInt a(10), b(3);
    a.sub(b);
    CHECK(a.cmp(BigInt(7)) == 0, "10 - 3 = 7");

    BigInt c("18446744073709551616");  // 2^64
    BigInt d("1");
    c.sub(d);
    CHECK(c.to_string() == "18446744073709551615", "2^64 - 1 correct");

    // 带借位多字减法
    BigInt e("10000000000000000000");
    BigInt f("1");
    e.sub(f);
    CHECK(e.to_string() == "9999999999999999999", "10^19 - 1 correct");
}

// ============ trailing_zeros ============
void test_trailing_zeros() {
    CHECK(BigInt(0).trailing_zeros() == 0, "0 has 0 trailing zeros (special)");
    CHECK(BigInt(1).trailing_zeros() == 0, "1 has 0 trailing zeros");
    CHECK(BigInt(2).trailing_zeros() == 1, "2 has 1 trailing zero");
    CHECK(BigInt(4).trailing_zeros() == 2, "4 has 2 trailing zeros");
    CHECK(BigInt(8).trailing_zeros() == 3, "8 has 3 trailing zeros");
    CHECK(BigInt(12).trailing_zeros() == 2, "12 has 2 trailing zeros");
    CHECK(BigInt(7).trailing_zeros() == 0, "7 has 0 trailing zeros");

    // 大数
    BigInt b256("256");
    CHECK(b256.trailing_zeros() == 8, "256 has 8 trailing zeros");

    // 多字 trailing zeros
    BigInt mw("18446744073709551616");  // 2^64
    CHECK(mw.trailing_zeros() == 64, "2^64 has 64 trailing zeros");
}

// ============ 右移 ============
void test_shr() {
    BigInt a(255);
    a.shr(4);
    CHECK(a.cmp(BigInt(15)) == 0, "255 >> 4 = 15");

    BigInt b("18446744073709551616");  // 2^64
    b.shr(1);
    CHECK(b.to_string() == "9223372036854775808", "2^64 >> 1 = 2^63");

    BigInt c("1");
    c.shr(1);
    CHECK(c.is_zero(), "1 >> 1 = 0");

    BigInt d("1024");
    d.shr(10);
    CHECK(d.is_one(), "1024 >> 10 = 1");
}

// ============ mod 基本 ============
void test_mod_basic() {
    BigInt a(10), b(3);
    a.mod(b);
    CHECK(a.cmp(BigInt(1)) == 0, "10 % 3 = 1");

    BigInt c(100), d(7);
    c.mod(d);
    CHECK(c.cmp(BigInt(2)) == 0, "100 % 7 = 2");

    BigInt e(7), f(10);
    e.mod(f);
    CHECK(e.cmp(BigInt(7)) == 0, "7 % 10 = 7");

    BigInt g(25), h(5);
    g.mod(h);
    CHECK(g.is_zero(), "25 % 5 = 0");

    // 大数模
    BigInt x("100000000000000000000");
    BigInt y("7");
    x.mod(y);
    // 10^20 = 100,000,000,000,000,000,000
    // 10^20 mod 7 = ?
    // 10 mod 7 = 3, so 10^20 mod 7 = 3^20 mod 7
    // 3^6 = 729 = 1 mod 7, 3^20 = 3^2 = 9 = 2 mod 7
    CHECK(x.cmp(BigInt(2)) == 0, "10^20 % 7 = 2");
}

// ============ from_decimal / to_string ============
void test_decimal_roundtrip() {
    const char* nums[] = {
        "0", "1", "42", "255", "1000000000000",
        "18446744073709551615",  // 2^64 - 1
        "18446744073709551616",  // 2^64
        "340282366920938463463374607431768211455",  // 2^128 - 1
    };
    for (auto* s : nums) {
        BigInt a(s);
        std::string back = a.to_string();
        CHECK(back == s, ("decimal roundtrip: " + std::string(s) + " -> " + back).c_str());
    }
}

int main() {
    auto t0 = std::chrono::high_resolution_clock::now();

    test_construction();
    test_mod4_mod8();
    test_compare();
    test_sub();
    test_trailing_zeros();
    test_shr();
    test_mod_basic();
    test_decimal_roundtrip();

    auto t1 = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    std::cout << "All " << passed << " tests passed in " << us << " us." << std::endl;
    return 0;
}
