#pragma once
#include "nonu_fog.hpp"
namespace spire {
// Advance both authored light UV row equations exactly, including the inner
// truncation of the shear term. Only construction divides wide integers.
struct LightQuadRows {
    nonu_fog::AffineRows u, v;
    int64_t shear_value, shear_step;
    int width;
    LightQuadRows(int dx, int dy, int sine, int cosine, int w, int h, int shear)
        : u((int64_t(dx) * cosine + int64_t(dy) * sine) * 65536 -
                (-int64_t(dx) * sine + int64_t(dy) * cosine) * shear,
            int64_t(sine) * 16777216 - int64_t(cosine) * shear * 256, int64_t(w) * 1024),
          v((-int64_t(dx) * sine + int64_t(dy) * cosine) * 64, int64_t(cosine) * 16384, h),
          shear_value((-int64_t(dx) * sine + int64_t(dy) * cosine) * shear),
          shear_step(int64_t(cosine) * shear * 256), width(w) {}
    int x() const {
        int64_t q = u.quotient,
                r = (u.remainder >> 10) + (shear_value > 0 && (u.remainder & 1023) != 0);
        if (r >= width) {
            ++q;
            r -= width;
        }
        return 32768 + int(q + (q < 0 && r != 0));
    }
    int y() const {
        return 32768 + v.value();
    }
    void next() {
        u.next();
        v.next();
        shear_value += shear_step;
    }
};
// Clip the exact integer UV sequence before visiting pixels. The reciprocal
// needs one division per quad axis; each row then uses multiply/high-word and
// one correction. It is exact for every uint32 numerator, including negative
// slopes and entry/exit at an inclusive lower or exclusive upper boundary.
struct LightQuadSpan {
    int delta;
    unsigned step, inverse;
    explicit LightQuadSpan(int d)
        : delta(d), step(d < 0 ? 0u - unsigned(d) : unsigned(d)),
          inverse(step > 1 ? unsigned((uint64_t(1) << 32) / step) : 0) {}
    unsigned divide(unsigned n) const {
        if (step == 1)
            return n;
        unsigned q = unsigned(uint64_t(n) * inverse >> 32);
        return q + unsigned(n - q * step >= step);
    }
    unsigned ceiling(unsigned n) const {
        return divide(n - 1) + 1;
    }
    void clip(int origin, unsigned low, unsigned extent, unsigned count, unsigned& first,
              unsigned& last) const {
        if (first >= last)
            return;
        const int high = int(low + extent) - 1;
        if (!delta) {
            if (origin < int(low) || origin > high)
                last = 0;
            return;
        }
        const int64_t end = int64_t(origin) + int64_t(delta) * (count - 1);
        unsigned begin = 0, stop = count;
        if (delta > 0) {
            if (origin > high || end < int(low)) {
                last = 0;
                return;
            }
            if (origin < int(low))
                begin = ceiling(low - unsigned(origin));
            if (end > high)
                stop = divide(unsigned(high) - unsigned(origin)) + 1;
        } else {
            if (origin < int(low) || end > high) {
                last = 0;
                return;
            }
            if (origin > high)
                begin = ceiling(unsigned(origin) - unsigned(high));
            if (end < int(low))
                stop = divide(unsigned(origin) - low) + 1;
        }
        if (begin > first)
            first = begin;
        if (stop < last)
            last = stop;
    }
};

} // namespace spire
