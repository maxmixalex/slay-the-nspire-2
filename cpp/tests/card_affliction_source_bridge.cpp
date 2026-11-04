// Private source comparison bridge: no game texture is compiled into tests.
#include "card_affliction_scene.hpp"
using namespace spire::card_affliction;
struct View {
    const uint8_t* pixels;
    const uint8_t *alpha = nullptr, *palette = nullptr;
    unsigned w = 256, h = 0, format = 0;
};
extern "C" int affliction_sample(const uint8_t* raw, unsigned size, const uint8_t* bank,
                                 unsigned length, unsigned command, uint32_t clock, int32_t* out) {
    Table t;
    if (!t.read(View{raw, nullptr, nullptr, 256, size / 512, 0}) || length != t.length ||
        command >= t.commands)
        return 0;
    const uint8_t* pages[56]{};
    for (unsigned i = 0; i < t.pages; ++i)
        pages[i] = bank + i * PageBytes;
    Bytes bytes{pages, length};
    Field fields[24];
    for (unsigned i = 0; i < t.fields; ++i)
        if (!fields[i].bind(bytes, dword(raw + t.field_at + i * 8)))
            return 0;
    const auto* c = t.command(command);
    if (c[0] != 1)
        return 0;
    Shader shader;
    int32_t params[13], inverse[6];
    for (unsigned i = 0; i < 4; ++i) {
        const int id = int16_t(word(c + 8 + i * 2));
        shader.field[i] = id < 0 ? nullptr : fields + id;
        shader.tint[i] = int32_t(dword(c + 72 + i * 4));
    }
    for (unsigned i = 0; i < 2; ++i) {
        const int id = int16_t(word(c + 16 + i * 2));
        if (id >= 0) {
            const auto* q = raw + t.curve_at + unsigned(id) * 8;
            shader.curve[i] = {nullptr, word(q + 4), word(q + 6), bytes, dword(q)};
        }
    }
    for (unsigned i = 0; i < 13; ++i)
        params[i] = int32_t(dword(c + 20 + i * 4));
    shader.parameters = params;
    shader.kind = c[1];
    shader.repeat = c[3];
    shader.prepare(clock);
    for (unsigned i = 0; i < 6; ++i)
        inverse[i] = int32_t(dword(c + 88 + i * 4));
    for (unsigned y = 0; y < 218; ++y)
        for (unsigned x = 0; x < 156; ++x) {
            const int u = int((2 * x + 1) * Q / (156 * 2)), v = int((2 * y + 1) * Q / (218 * 2));
            const int a = int((int64_t(inverse[0]) * u + int64_t(inverse[1]) * v) / Q) + inverse[2],
                      b = int((int64_t(inverse[3]) * u + int64_t(inverse[4]) * v) / Q) + inverse[5];
            Pixel p;
            if (unsigned(a) < Q && unsigned(b) < Q)
                p = shader.sample(a, b);
            *out++ = p.r;
            *out++ = p.g;
            *out++ = p.b;
            *out++ = p.a;
        }
    return 1;
}
extern "C" int affliction_particle_sample(const uint8_t* raw, unsigned size, const uint8_t* bank,
                                          unsigned length, unsigned command, uint32_t clock,
                                          unsigned custom, int32_t* out) {
    Table t;
    if (!t.read(View{raw, nullptr, nullptr, 256, size / 512, 0}) || length != t.length ||
        command >= t.commands)
        return 0;
    const uint8_t* pages[56]{};
    for (unsigned i = 0; i < t.pages; ++i)
        pages[i] = bank + i * PageBytes;
    Bytes bytes{pages, length};
    const auto* c = t.command(command);
    if (c[0] != 2)
        return 0;
    Field f;
    if (!f.bind(bytes, dword(raw + t.field_at + word(c + 8) * 8)))
        return 0;
    Shader s;
    s.field[0] = &f;
    s.kind = c[1];
    s.repeat = c[3];
    s.custom = custom;
    const auto* q = raw + t.curve_at + word(c + 16) * 8;
    s.curve[0] = {nullptr, word(q + 4), word(q + 6), bytes, dword(q)};
    s.prepare(clock);
    for (unsigned y = 0; y < 128; ++y)
        for (unsigned x = 0; x < 128; ++x) {
            const auto p = s.sample(int((2 * x + 1) * Q / 256), int((2 * y + 1) * Q / 256));
            *out++ = p.r;
            *out++ = p.g;
            *out++ = p.b;
            *out++ = p.a;
        }
    return 1;
}
