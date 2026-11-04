#pragma once
#include "tez_cinders.hpp"
namespace spire::vakuu_cinders {
constexpr unsigned Auxiliary = 7225, FirstImage = 59730, Images = 59731;
constexpr unsigned Sprite = tez_cinders::FirstImage, Curves = Sprite + 1;
struct Parameters {
    const uint8_t* data = nullptr;
    template <class Picture> bool read(Picture p) {
        using namespace tez_cinders;
        if (!p.pixels || p.w != 32 || p.h != 1 || p.format || p.alpha || p.palette)
            return false;
        const auto* b = p.pixels;
        if (b[0] != 'V' || b[1] != 'K' || b[2] != 'C' || b[3] != '1' || word(b + 4) != 1 ||
            word(b + 6) != 24 || word(b + 8) != Sprite || word(b + 10) != Curves ||
            word(b + 12) != AuxiliaryFirst || word(b + 14))
            return false;
        for (unsigned i = 0; i < 6; ++i)
            if (integer(b + 16 + i * 4) <= -8388608 || integer(b + 16 + i * 4) >= 8388608)
                return false;
        for (unsigned i = 40; i < 64; ++i)
            if (b[i])
                return false;
        data = b;
        return true;
    }
};
struct Table {
    Parameters geometry;
    tez_cinders::Table source;
    auto source_curve(unsigned index) const {
        return source.source_curve(index);
    }
    int32_t coordinate(unsigned, unsigned component, unsigned) const {
        return tez_cinders::integer(geometry.data + 16 + component * 4);
    }
};
} // namespace spire::vakuu_cinders
