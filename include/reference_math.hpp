#pragma once
#include <stdint.h>

namespace spire::reference::math {
// Software binary64. All arithmetic rounds nearest-even without FPU, libc,
// compiler-rt floating-point helpers, 128-bit integers or fused operations.
struct Real {
    uint64_t bits;
    static constexpr Real raw(uint64_t value) { return {value}; }
    static Real integer(int32_t value);
    static Real fraction53(uint64_t value);
    bool finite() const { return (bits & UINT64_C(0x7ff0000000000000)) != UINT64_C(0x7ff0000000000000); }
};
Real operator+(Real, Real);
Real operator-(Real, Real);
Real operator*(Real, Real);
Real operator/(Real, Real);
inline Real operator-(Real x) { return Real::raw(x.bits ^ (UINT64_C(1) << 63)); }
bool operator<(Real, Real);
inline bool operator>(Real a, Real b) { return b < a; }
Real absolute(Real);
Real square_root(Real);
Real logarithm(Real); // Positive finite arguments only; caller validates domain.
Real sine_cycle(Real); // [0, 2*pi] only; not a general-purpose large-angle sine.
bool round_even_int(Real, int32_t&);
// True when a libm-level last-bit difference could make the integer choice
// ambiguous. Callers fail explicitly; they must not silently draw another sample.
bool near_half_integer(Real);
}
