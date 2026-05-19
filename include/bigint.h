#pragma once
#include <vector>
#include <cstdint>
#include <string>

class BigInt {
public:
    BigInt();
    explicit BigInt(uint64_t v);
    explicit BigInt(const std::string& dec);

    bool is_zero() const;
    bool is_one() const;
    int  trailing_zeros() const;
    void shr(int n);
    int  cmp(const BigInt& o) const;
    void sub(const BigInt& o);
    void mod(const BigInt& o);
    int  mod4() const;
    int  mod8() const;
    std::string to_string() const;

private:
    std::vector<uint64_t> d;
    void normalize();

public:
    // 公开给 profiling 工具
    int  word_count() const { return (int)d.size(); }
    uint64_t limb(int i) const { return d[i]; }
    void sub_mul(const BigInt& b, uint64_t q);
    int  size_words() const { return (int)d.size(); }
    static uint64_t div_128_64(uint64_t hi, uint64_t lo, uint64_t div, uint64_t& rem);
};
