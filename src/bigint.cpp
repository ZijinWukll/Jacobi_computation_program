#include "bigint.h"
#include <algorithm>
#include <string>
using namespace std;

// ============ 128位算术（利用 __int128 编译器支持）============

uint64_t BigInt::div_128_64(uint64_t hi, uint64_t lo, uint64_t div, uint64_t& rem) {
    unsigned __int128 val = (unsigned __int128)hi << 64 | lo;
    rem = (uint64_t)(val % div);
    return (uint64_t)(val / div);
}

// ============ 构造与规范化 ============

BigInt::BigInt() : d(1, 0) {}
BigInt::BigInt(uint64_t v) : d(1, v) {}

BigInt::BigInt(const string& dec) {
    d.push_back(0);
    size_t len = dec.size();
    const uint64_t POW9 = 1000000000ULL;
    size_t pos = 0;

    // 处理开头的非 9 位组（余数部分）
    if (size_t rem = len % 9) {
        uint64_t chunk = 0;
        for (size_t j = 0; j < rem; j++)
            chunk = chunk * 10 + (uint64_t)(dec[j] - '0');
        d[0] = chunk;
        pos = rem;
    }

    // 后续每 9 位一组处理
    while (pos < len) {
        uint64_t chunk = 0;
        for (int j = 0; j < 9; j++)
            chunk = chunk * 10 + (uint64_t)(dec[pos + j] - '0');
        pos += 9;

        // d *= 10^9
        uint64_t carry = 0;
        for (auto& w : d) {
            unsigned __int128 prod = (unsigned __int128)w * POW9 + carry;
            w = (uint64_t)prod;
            carry = (uint64_t)(prod >> 64);
        }
        if (carry) d.push_back(carry);

        // d += chunk
        if (chunk) {
            unsigned __int128 sum = (unsigned __int128)d[0] + chunk;
            d[0] = (uint64_t)sum;
            uint64_t sc = (uint64_t)(sum >> 64);
            for (size_t i = 1; sc; i++) {
                if (i < d.size()) {
                    unsigned __int128 s2 = (unsigned __int128)d[i] + sc;
                    d[i] = (uint64_t)s2;
                    sc = (uint64_t)(s2 >> 64);
                } else {
                    d.push_back(sc);
                    break;
                }
            }
        }
    }
    normalize();
}

void BigInt::normalize() {
    while (d.size() > 1 && d.back() == 0) d.pop_back();
}

// ============ 基础查询 ============

bool BigInt::is_zero() const { return d.size() == 1 && d[0] == 0; }
bool BigInt::is_one() const  { return d.size() == 1 && d[0] == 1; }
int  BigInt::mod4() const    { return (int)(d[0] & 3); }
int  BigInt::mod8() const    { return (int)(d[0] & 7); }

int BigInt::trailing_zeros() const {
    if (is_zero()) return 0;
    int t = 0;
    for (size_t i = 0; i < d.size(); i++) {
        if (d[i] == 0) { t += 64; continue; }
        t += __builtin_ctzll(d[i]);
        break;
    }
    return t;
}

// ============ 右移 ============

void BigInt::shr(int n) {
    if (n == 0) return;
    int ws = n / 64, bs = n % 64;
    if (ws >= (int)d.size()) { d.assign(1, 0); return; }
    if (ws > 0) {
        size_t sz = d.size() - ws;
        for (size_t i = 0; i < sz; i++) d[i] = d[i + ws];
        d.resize(sz);
    }
    if (bs > 0) {
        uint64_t carry = 0;
        for (int i = (int)d.size() - 1; i >= 0; i--) {
            uint64_t cur = d[i];
            d[i] = (cur >> bs) | carry;
            carry = cur << (64 - bs);
        }
    }
    normalize();
}

// ============ 比较 ============

int BigInt::cmp(const BigInt& o) const {
    if (d.size() != o.d.size())
        return d.size() > o.d.size() ? 1 : -1;
    for (int i = (int)d.size() - 1; i >= 0; i--) {
        if (d[i] != o.d[i]) return d[i] > o.d[i] ? 1 : -1;
    }
    return 0;
}

// ============ 减法 *this -= o（*this >= o）============

void BigInt::sub(const BigInt& o) {
    uint64_t borrow = 0;
    size_t i = 0;
    for (; i < o.d.size(); i++) {
        unsigned __int128 diff = (unsigned __int128)d[i] - o.d[i] - borrow;
        d[i] = (uint64_t)diff;
        borrow = (uint64_t)(diff >> 64) != 0;
    }
    for (; borrow && i < d.size(); i++) {
        unsigned __int128 diff = (unsigned __int128)d[i] - 1;
        d[i] = (uint64_t)diff;
        borrow = (uint64_t)(diff >> 64) != 0;
    }
    normalize();
}

// ============ 辅助：*this -= q * b ============

void BigInt::sub_mul(const BigInt& b, uint64_t q) {
    if (q == 2) [[likely]] {
        // Fast path: a -= b*2 (left-shift instead of general 128-bit multiply)
        uint64_t mcarry = 0, borrow = 0;
        size_t limit = b.d.size(), i = 0;
        for (; i < limit; i++) {
            uint64_t prod_lo = (b.d[i] << 1) | mcarry;
            mcarry = b.d[i] >> 63;
            unsigned __int128 diff = (unsigned __int128)d[i] - prod_lo - borrow;
            d[i] = (uint64_t)diff;
            borrow = (uint64_t)(diff >> 64) != 0;
        }
        if (i < d.size()) {
            unsigned __int128 diff = (unsigned __int128)d[i] - mcarry - borrow;
            d[i] = (uint64_t)diff;
            borrow = (uint64_t)(diff >> 64) != 0;
            i++;
        } else {
            borrow = borrow || (mcarry != 0);
        }
        while (borrow && i < d.size()) {
            unsigned __int128 diff = (unsigned __int128)d[i] - 1;
            d[i] = (uint64_t)diff;
            borrow = (uint64_t)(diff >> 64) != 0;
            i++;
        }
        normalize();
        return;
    }

    uint64_t mcarry = 0;
    uint64_t borrow = 0;
    size_t limit = b.d.size();
    size_t i = 0;

    for (; i < limit; i++) {
        unsigned __int128 prod = (unsigned __int128)b.d[i] * q + mcarry;
        mcarry = (uint64_t)(prod >> 64);

        unsigned __int128 diff = (unsigned __int128)d[i] - (uint64_t)prod - borrow;
        d[i] = (uint64_t)diff;
        borrow = (uint64_t)(diff >> 64) != 0;
    }

    if (i < d.size()) {
        unsigned __int128 diff = (unsigned __int128)d[i] - mcarry - borrow;
        d[i] = (uint64_t)diff;
        borrow = (uint64_t)(diff >> 64) != 0;
        i++;
    } else {
        borrow = borrow || (mcarry != 0);
    }

    while (borrow && i < d.size()) {
        unsigned __int128 diff = (unsigned __int128)d[i] - 1;
        d[i] = (uint64_t)diff;
        borrow = (uint64_t)(diff >> 64) != 0;
        i++;
    }
    normalize();
}

// ============ 取模（Lehmer 加速）============

void BigInt::mod(const BigInt& b) {
    if (cmp(b) < 0) [[unlikely]] return;

    if (b.d.size() == 1) [[unlikely]] {
        uint64_t div = b.d[0], r = 0;
        for (int i = (int)d.size() - 1; i >= 0; i--) {
            unsigned __int128 val = ((unsigned __int128)r << 64) | d[i];
            d[i] = (uint64_t)(val / div);
            r = (uint64_t)(val % div);
        }
        d.resize(1);
        d[0] = r;
        normalize();
        return;
    }

    while (cmp(b) >= 0) {
        int sa = (int)d.size(), sb = (int)b.d.size();
        int shift = sa - sb;

        if (shift >= 2) [[unlikely]] {
            uint64_t a_hi = d[sa - 1], a_next = d[sa - 2];
            uint64_t b_hi = b.d[sb - 1];
            uint64_t div = b_hi + 1;
            if (div == 0 || a_hi >= div) { sub(b); continue; }
            uint64_t q = div_128_64(a_hi, a_next, div, a_next);
            if (q == 0) q = 1;
            sub_mul(b, q);
        } else if (shift == 1) {
            uint64_t a_hi = d[sa - 1], a_lo = d[sa - 2];
            uint64_t b_hi = b.d[sb - 1];
            uint64_t div = b_hi + 1;
            if (div == 0 || a_hi >= div) { sub(b); continue; }
            uint64_t q = div_128_64(a_hi, a_lo, div, a_lo);
            if (q == 0) q = 1;
            sub_mul(b, q);
        } else {
            uint64_t a_hi = d[sa - 1];
            uint64_t b_hi = b.d[sb - 1];
            if (a_hi > b_hi) [[likely]] {
                uint64_t div = b_hi + 1;
                if (div != 0) {
                    uint64_t q = a_hi / div;
                    if (q >= 2) [[likely]] { sub_mul(b, q); continue; }
                }
            }
            sub(b);
        }
    }
}

// ============ 十进制输出 ============

string BigInt::to_string() const {
    if (is_zero()) return "0";
    BigInt tmp = *this;
    string s;
    const uint64_t POW9 = 1000000000ULL;
    while (!tmp.is_zero()) {
        // 128/64 除法，除以 10^9
        uint64_t r = 0;
        for (int i = (int)tmp.d.size() - 1; i >= 0; i--) {
            unsigned __int128 val = ((unsigned __int128)r << 64) | tmp.d[i];
            tmp.d[i] = (uint64_t)(val / POW9);
            r = (uint64_t)(val % POW9);
        }
        tmp.normalize();
        // 提取余数的 9 位十进制数字
        for (int j = 0; j < 9; j++) {
            s.push_back('0' + (char)(r % 10));
            r /= 10;
        }
    }
    // 移除高位的多余零（最后一个 chunk 可能不足 9 位）
    while (s.size() > 1 && s.back() == '0') s.pop_back();
    reverse(s.begin(), s.end());
    return s;
}
