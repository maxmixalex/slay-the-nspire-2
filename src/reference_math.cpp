// Adapted from Go runtime/softfloat64.go and math/{log,sqrt,sin}.go.
// Copyright 2009-2011 The Go Authors. All rights reserved.
// BSD-3-Clause: see licenses/Go-BSD-3-Clause.txt.
// Log/sqrt algorithms and constants also carry this notice:
// Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
// Developed at SunPro, a Sun Microsystems, Inc. business.
// Permission to use, copy, modify, and distribute this software is freely
// granted, provided that this notice is preserved.
// Sine coefficients: Cephes Math Library Release 2.8, June 2000.
// Copyright 1984, 1987, 1989, 1992, 2000 by Stephen L. Moshier.
// May be used freely, without support or guarantee.
// C++ adaptation: explicit fixed-width shifts, normal/subnormal rounding,
// integer long division and restricted-domain elementary functions.
#include "reference_math.hpp"

namespace spire::reference::math {
namespace {
constexpr uint64_t Sign = UINT64_C(1) << 63;
constexpr uint64_t Implicit = UINT64_C(1) << 52;
constexpr uint64_t Mask = Implicit - 1;
constexpr uint64_t Inf = UINT64_C(0x7ff0000000000000);
constexpr uint64_t NaN = UINT64_C(0x7ff8000000000000);
constexpr Real One = Real::raw(UINT64_C(0x3ff0000000000000));
constexpr Real Two = Real::raw(UINT64_C(0x4000000000000000));
constexpr Real Half = Real::raw(UINT64_C(0x3fe0000000000000));
struct Parts { uint64_t sign, mantissa; int exponent; bool infinity, nan; };
Parts unpack(Real x) {
    Parts p{x.bits & Sign, x.bits & Mask, int((x.bits >> 52) & 2047), false, false};
    if (p.exponent == 2047) { p.nan = p.mantissa != 0; p.infinity = !p.nan; }
    else if (!p.exponent) {
        if (p.mantissa) {
            p.exponent = -1022;
            while (p.mantissa < Implicit) { p.mantissa <<= 1; --p.exponent; }
        }
    } else { p.mantissa |= Implicit; p.exponent -= 1023; }
    return p;
}
Real pack(uint64_t sign, uint64_t mantissa, int exponent, uint64_t sticky) {
    if (!mantissa) return Real::raw(sign);
    while (mantissa < Implicit) { mantissa <<= 1; --exponent; }
    // Preserve the normalized form, including cancellation before subnormal
    // rounding; saving before normalization is incorrect (Go issue #79964).
    uint64_t original = mantissa, original_sticky = sticky;
    int original_exponent = exponent;
    while (mantissa >= 4 * Implicit) { sticky |= mantissa & 1; mantissa >>= 1; ++exponent; }
    if (mantissa >= 2 * Implicit) {
        if ((mantissa & 1) && (sticky || (mantissa & 2))) {
            ++mantissa;
            if (mantissa >= 4 * Implicit) { mantissa >>= 1; ++exponent; }
        }
        mantissa >>= 1; ++exponent;
    }
    if (exponent >= 1024) return Real::raw(sign | Inf);
    if (exponent < -1022) {
        if (exponent < -1075) return Real::raw(sign);
        mantissa = original; exponent = original_exponent; sticky = original_sticky;
        while (exponent < -1023) { sticky |= mantissa & 1; mantissa >>= 1; ++exponent; }
        if ((mantissa & 1) && (sticky || (mantissa & 2))) ++mantissa;
        mantissa >>= 1; ++exponent;
        if (mantissa < Implicit) return Real::raw(sign | mantissa);
    }
    return Real::raw(sign | (uint64_t(exponent + 1023) << 52) | (mantissa & Mask));
}
void multiply_wide(uint64_t u, uint64_t v, uint64_t& lo, uint64_t& hi) {
    uint64_t u0 = uint32_t(u), u1 = u >> 32, v0 = uint32_t(v), v1 = v >> 32;
    uint64_t w0 = u0 * v0, t = u1 * v0 + (w0 >> 32);
    uint64_t w1 = uint32_t(t), w2 = t >> 32; w1 += u0 * v1;
    lo = u * v; hi = u1 * v1 + w2 + (w1 >> 32);
}
uint32_t small_integer(Real x) {
    auto p = unpack(x);
    if (p.exponent < 0 || !p.mantissa) return 0;
    return p.exponent <= 31 ? uint32_t(p.mantissa >> (52 - p.exponent)) : UINT32_MAX;
}
}
Real Real::integer(int32_t value) {
    uint64_t n = value < 0 ? uint64_t(-int64_t(value)) : uint64_t(value);
    return pack(value < 0 ? Sign : 0, n, 52, 0);
}
Real Real::fraction53(uint64_t value) {
    return pack(0, value & ((UINT64_C(1) << 53) - 1), -1, 0);
}
Real absolute(Real x) { return Real::raw(x.bits & ~Sign); }
Real operator+(Real x, Real y) {
    auto a = unpack(x), b = unpack(y);
    if (a.nan || b.nan || (a.infinity && b.infinity && a.sign != b.sign)) return Real::raw(NaN);
    if (a.infinity) return x;
    if (b.infinity) return y;
    if (!a.mantissa && !b.mantissa) return Real::raw(a.sign & b.sign);
    if (!a.mantissa) return y;
    if (!b.mantissa) return x;
    if (a.exponent < b.exponent || (a.exponent == b.exponent && a.mantissa < b.mantissa)) {
        Parts swap = a; a = b; b = swap;
    }
    unsigned shift = unsigned(a.exponent - b.exponent);
    a.mantissa <<= 2; b.mantissa <<= 2;
    uint64_t sticky = shift >= 64 ? b.mantissa : b.mantissa & ((UINT64_C(1) << shift) - 1);
    b.mantissa = shift >= 64 ? 0 : b.mantissa >> shift;
    if (a.sign == b.sign) a.mantissa += b.mantissa;
    else { a.mantissa -= b.mantissa; if (sticky) --a.mantissa; }
    if (!a.mantissa) a.sign = 0;
    return pack(a.sign, a.mantissa, a.exponent - 2, sticky);
}
Real operator-(Real x, Real y) { return x + (-y); }
Real operator*(Real x, Real y) {
    auto a = unpack(x), b = unpack(y); uint64_t sign = a.sign ^ b.sign;
    if (a.nan || b.nan || (a.infinity && !b.infinity && !b.mantissa) ||
        (b.infinity && !a.infinity && !a.mantissa)) return Real::raw(NaN);
    if (a.infinity || b.infinity) return Real::raw(sign | Inf);
    if (!a.mantissa || !b.mantissa) return Real::raw(sign);
    uint64_t lo, hi; multiply_wide(a.mantissa, b.mantissa, lo, hi);
    return pack(sign, (hi << 13) | (lo >> 51), a.exponent + b.exponent - 1,
                lo & ((UINT64_C(1) << 51) - 1));
}
Real operator/(Real x, Real y) {
    auto a = unpack(x), b = unpack(y); uint64_t sign = a.sign ^ b.sign;
    if (a.nan || b.nan || (a.infinity && b.infinity) ||
        (!a.infinity && !b.infinity && !a.mantissa && !b.mantissa)) return Real::raw(NaN);
    if (a.infinity || (!b.infinity && !b.mantissa)) return Real::raw(sign | Inf);
    if (b.infinity || !a.mantissa) return Real::raw(sign);
    // Both normalized mantissas are 53 bits. Long division never needs a
    // 128-bit intermediate or a target 64-bit division helper.
    uint64_t q = a.mantissa >= b.mantissa ? 1 : 0, r = a.mantissa - q * b.mantissa;
    for (unsigned i = 0; i < 54; ++i) {
        r <<= 1; q <<= 1;
        if (r >= b.mantissa) { r -= b.mantissa; q |= 1; }
    }
    return pack(sign, q, a.exponent - b.exponent - 2, r);
}
bool operator<(Real x, Real y) {
    auto a = unpack(x), b = unpack(y);
    if (a.nan || b.nan) return false;
    if (!a.infinity && !b.infinity && !a.mantissa && !b.mantissa) return false;
    if (a.sign != b.sign) return a.sign != 0;
    return a.sign ? x.bits > y.bits : x.bits < y.bits;
}
bool round_even_int(Real x, int32_t& result) {
    auto p = unpack(x);
    if (p.infinity || p.nan || p.exponent > 31) return false;
    uint64_t n = 0;
    if (p.mantissa && p.exponent >= -1) {
        unsigned shift = unsigned(52 - p.exponent);
        n = p.mantissa >> shift;
        uint64_t rem = p.mantissa & ((UINT64_C(1) << shift) - 1), half = UINT64_C(1) << (shift - 1);
        if (rem > half || (rem == half && (n & 1))) ++n;
    }
    if (n > (p.sign ? UINT64_C(2147483648) : UINT64_C(2147483647))) return false;
    result = int32_t(p.sign ? -int64_t(n) : int64_t(n)); return true;
}
bool near_half_integer(Real x) {
    if (!x.finite()) return true;
    x = absolute(x);
    uint32_t integral = small_integer(x);
    if (integral > 1000000) return true; // Not a supported map-quota input.
    Real remainder = x - Real::integer(int32_t(integral));
    return absolute(remainder - Half) < Real::raw(UINT64_C(0x3db0000000000000)); // 2^-36
}
Real square_root(Real x) {
    if (!(x.bits & ~Sign) || (x.bits & ~Sign) > Inf || x.bits == Inf) return x;
    if (x.bits & Sign) return Real::raw(NaN);
    uint64_t v = x.bits; int exponent = int((v >> 52) & 2047);
    if (!exponent) {
        while (!(v & Implicit)) { v <<= 1; --exponent; }
        ++exponent;
    }
    exponent -= 1023; v = (v & Mask) | Implicit;
    if (exponent & 1) v <<= 1;
    // Floor division, independent of implementation-defined signed shifts.
    exponent = exponent >= 0 ? exponent / 2 : -((-exponent + 1) / 2);
    v <<= 1; uint64_t q = 0, s = 0;
    for (uint64_t r = 2 * Implicit; r; r >>= 1) {
        uint64_t t = s + r;
        if (t <= v) { s = t + r; v -= t; q += r; }
        v <<= 1;
    }
    if (v) q += q & 1;
    return Real::raw((q >> 1) + (uint64_t(exponent - 1 + 1023) << 52));
}
Real logarithm(Real x) {
    if (!(x.bits & ~Sign)) return Real::raw(Sign | Inf);
    if (x.bits & Sign) return Real::raw(NaN);
    if (!x.finite()) return x;
    auto p = unpack(x);
    Real f1 = Real::raw((UINT64_C(1022) << 52) | (p.mantissa & Mask));
    int exponent = p.exponent + 1;
    if (f1 < Real::raw(UINT64_C(0x3fe6a09e667f3bcd))) { f1 = f1 * Two; --exponent; }
    Real f = f1 - One, k = Real::integer(exponent), s = f / (Two + f), s2 = s*s, s4 = s2*s2;
    constexpr Real l1 = Real::raw(UINT64_C(0x3fe5555555555593));
    constexpr Real l2 = Real::raw(UINT64_C(0x3fd999999997fa04));
    constexpr Real l3 = Real::raw(UINT64_C(0x3fd2492494229359));
    constexpr Real l4 = Real::raw(UINT64_C(0x3fcc71c51d8e78af));
    constexpr Real l5 = Real::raw(UINT64_C(0x3fc7466496cb03de));
    constexpr Real l6 = Real::raw(UINT64_C(0x3fc39a09d078c69f));
    constexpr Real l7 = Real::raw(UINT64_C(0x3fc2f112df3e5244));
    Real r = s2*(l1+s4*(l3+s4*(l5+s4*l7))) + s4*(l2+s4*(l4+s4*l6));
    Real hfsq = Half*f*f;
    return k*Real::raw(UINT64_C(0x3fe62e42fee00000)) -
        ((hfsq - (s*(hfsq+r) + k*Real::raw(UINT64_C(0x3dea39ef35793c76)))) - f);
}
Real sine_cycle(Real x) {
    if (x < Real::raw(0) || x > Real::raw(UINT64_C(0x401921fb54442d18)) || !x.finite()) return Real::raw(NaN);
    if (!(x.bits & ~Sign)) return x;
    unsigned octant = small_integer(x * Real::raw(UINT64_C(0x3ff45f306dc9c883)));
    if (octant & 1) ++octant;
    Real y = Real::integer(int32_t(octant)); octant &= 7;
    Real z = ((x-y*Real::raw(UINT64_C(0x3fe921fb40000000))) -
              y*Real::raw(UINT64_C(0x3e64442d00000000))) - y*Real::raw(UINT64_C(0x3ce8469898cc5170));
    bool negative = octant > 3; if (negative) octant -= 4;
    Real zz = z*z;
    constexpr uint64_t sin_coeff[] = {UINT64_C(0x3de5d8fd1fd19ccd),UINT64_C(0xbe5ae5e5a9291f5d),
        UINT64_C(0x3ec71de3567d48a1),UINT64_C(0xbf2a01a019bfdf03),UINT64_C(0x3f8111111110f7d0),UINT64_C(0xbfc5555555555548)};
    constexpr uint64_t cos_coeff[] = {UINT64_C(0xbda8fa49a0861a9b),UINT64_C(0x3e21ee9d7b4e3f05),
        UINT64_C(0xbe927e4f7eac4bc6),UINT64_C(0x3efa01a019c844f5),UINT64_C(0xbf56c16c16c14f91),UINT64_C(0x3fa555555555554b)};
    bool cosine = octant == 1 || octant == 2;
    const auto* coeff = cosine ? cos_coeff : sin_coeff;
    Real poly = Real::raw(coeff[0]);
    for (unsigned i = 1; i < 6; ++i) poly = poly*zz + Real::raw(coeff[i]);
    y = cosine ? One-Half*zz+zz*zz*poly : z+z*zz*poly;
    return negative ? -y : y;
}
} // namespace spire::reference::math
