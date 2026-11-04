#pragma once
#include "card_affliction_shader.hpp"

namespace spire::card_affliction {
constexpr unsigned Auxiliary = 5202, Slots = 56;
struct Table {
    const uint8_t* data = nullptr;
    unsigned pages = 0, pictures = 0, fields = 0, curves = 0, commands = 0, tracks = 0, length = 0,
             field_at = 0, curve_at = 0, command_at = 0, track_at = 0;
    template <class Picture> bool read(const Picture& p) {
        if (!p.pixels || p.format || p.alpha || p.palette || p.w != 256 || p.h < 1 || p.h > 32)
            return false;
        const auto* d = p.pixels;
        const unsigned size = unsigned(p.w) * p.h * 2;
        if (size < 64 || dword(d) != 0x31554341u || word(d + 4) != 1 || word(d + 6) != Slots ||
            word(d + 20) != 156 || word(d + 22) != 218 || word(d + 24) != 128 ||
            word(d + 26) != 38 || dword(d + 28) || dword(d + 60))
            return false;
        pages = word(d + 8);
        pictures = word(d + 10);
        fields = word(d + 12);
        curves = word(d + 14);
        commands = word(d + 16);
        tracks = word(d + 18);
        length = dword(d + 32);
        field_at = dword(d + 40);
        curve_at = dword(d + 44);
        command_at = dword(d + 48);
        track_at = dword(d + 52);
        if (!pages || 1 + pages + pictures > Slots || !fields || fields > 24 || curves > 64 ||
            !commands || commands > 64 || tracks != 55 || !length || length > pages * PageBytes ||
            dword(d + 36) != 64 || field_at != 92 || curve_at != field_at + fields * 8 ||
            command_at != curve_at + curves * 8 || track_at != command_at + commands * 128 ||
            dword(d + 56) != track_at + tracks * 16 || dword(d + 56) > size)
            return false;
        data = d;
        return true;
    }
    const uint8_t* command(unsigned index) const {
        return data + command_at + index * 128;
    }
    const uint8_t* track(unsigned index) const {
        return data + track_at + index * 16;
    }
    bool range(unsigned at, unsigned size) const {
        return at <= length && size <= length - at;
    }
    bool mark(bool* slots, unsigned at, unsigned size) const {
        if (!size || !range(at, size))
            return false;
        for (unsigned i = at / PageBytes; i <= (at + size - 1) / PageBytes; ++i)
            slots[i + 1] = true;
        return true;
    }
    bool family(unsigned kind, unsigned& first, unsigned& count) const {
        if (kind < 1 || kind > 7)
            return false;
        first = word(data + 64 + (kind - 1) * 4);
        count = word(data + 66 + (kind - 1) * 4);
        return count && first <= commands && count <= commands - first;
    }
    bool required(unsigned kind, bool* slots) const {
        unsigned first, count;
        if (!family(kind, first, count))
            return false;
        slots[0] = true;
        for (unsigned n = first; n < first + count; ++n) {
            const auto* c = command(n);
            if (c[0] > 2 || c[1] > 10 || c[2] > 1 || c[3] > 1)
                return false;
            for (unsigned i = 0; i < 5; ++i) {
                const int id = int16_t(word(c + (i ? 6 + i * 2 : 4)));
                if (id < 0)
                    continue;
                if (unsigned(id) >= fields)
                    return false;
                const auto* f = data + field_at + unsigned(id) * 8;
                if (dword(f + 4) < 32 || !mark(slots, dword(f), dword(f + 4)))
                    return false;
            }
            if (c[0] &&
                (!c[1] || int16_t(word(c + 8)) < 0 || (c[1] == 1 && int16_t(word(c + 12)) < 0) ||
                 (c[1] == 2 && int16_t(word(c + 14)) < 0) ||
                 ((c[1] == 3 || c[1] == 5) && int16_t(word(c + 10)) < 0) ||
                 (c[1] == 7 && int16_t(word(c + 12)) < 0) ||
                 ((c[1] == 3 || c[1] == 4 || c[1] >= 6) && int16_t(word(c + 16)) < 0) ||
                 (c[1] == 6 && int16_t(word(c + 18)) < 0)))
                return false;
            for (unsigned i = 0; i < 2; ++i) {
                const int id = int16_t(word(c + 16 + i * 2));
                if (id < 0)
                    continue;
                if (unsigned(id) >= curves)
                    return false;
                const auto* q = data + curve_at + unsigned(id) * 8;
                if (word(q + 4) < 2 || word(q + 4) > 1024 || !word(q + 6) || word(q + 6) > 4 ||
                    !mark(slots, dword(q), word(q + 4) * word(q + 6) * 4))
                    return false;
            }
            if (c[0] == 0) {
                const int id = int16_t(word(c + 6));
                if (id < 0 || unsigned(id) >= pictures)
                    return false;
                slots[1 + pages + unsigned(id)] = true;
            }
            if (c[0] == 2) {
                const unsigned at = dword(c + 120), num = word(c + 124);
                if (at > tracks || num > tracks - at)
                    return false;
                for (unsigned i = at; i < at + num; ++i) {
                    const auto* t = track(i);
                    const unsigned life = dword(t), fps = word(t + 8), frames = word(t + 10);
                    if (!life || life > 600000 || dword(t + 4) >= life ||
                        (fps != 30 && fps != 60) || !frames || frames > 181 ||
                        !mark(slots, dword(t + 12), frames * 38))
                        return false;
                }
            }
        }
        return true;
    }
};
} // namespace spire::card_affliction
