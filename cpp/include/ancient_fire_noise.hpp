#pragma once
#include <stdint.h>
namespace spire::ancient_fire_noise {
constexpr unsigned First = 59732, Count = 8, PageBytes = 131072, Reconstructed = 8;
constexpr bool page(unsigned id) {
    return id >= First && id < First + Count;
}
struct Page {
    unsigned used = 0, stride = 0;
};
inline Page layout(unsigned id) {
    if (!page(id))
        return {};
    unsigned index = id - First;
    constexpr unsigned rows[][2] = {
        {262144, 512}, {187500, 750}, {250000, 500}, {25000, 50}, {10000, 20}};
    for (const auto& row : rows) {
        const unsigned pages = (row[0] + PageBytes - 1) / PageBytes;
        if (index < pages) {
            const unsigned left = row[0] - index * PageBytes;
            return {left < PageBytes ? left : PageBytes, row[1]};
        }
        index -= pages;
    }
    return {};
}
// Each source page starts with its own literal row. There is no dependency
// on another page or on an earlier source image. Padding is never predicted.
inline bool restore(uint8_t* data, unsigned bytes, unsigned id, unsigned mode) {
    const auto p = layout(id);
    if (!data || !p.used || bytes != (p.used + 639) / 640 * 640 || mode > 1)
        return false;
    for (unsigned i = p.used; i < bytes; ++i)
        if (data[i])
            return false;
    if (mode)
        for (unsigned i = p.stride; i < p.used; ++i)
            data[i] = uint8_t(data[i] + data[i - p.stride]);
    return true;
}
} // namespace spire::ancient_fire_noise
