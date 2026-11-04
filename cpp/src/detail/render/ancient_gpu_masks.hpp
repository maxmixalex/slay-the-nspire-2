#pragma once
// Constant authored light transforms are sampled from original texture at each
// native LCD center offline. Only their original alpha envelope changes here.
namespace ancient_gpu_masks {
using namespace ancient_gpu_masks;
static bool parameters(Canvas& c, unsigned auxiliary, Parameters& result) {
    const int image = c.a.content.ui_image(auxiliary);
    return image >= 0 && result.read(c.a.content.image(image));
}
static void lights(Canvas& c, unsigned auxiliary, uint32_t elapsed, bool vakuu, unsigned first,
                   unsigned count) {
    Parameters parameters;
    if (!ancient_gpu_masks::parameters(c, auxiliary, parameters))
        return;
    const unsigned alpha = ramp(parameters.data + 128, 1, 0, light_phase(elapsed));
    if (!alpha)
        return;
    const uint16_t color = vakuu ? uint16_t((247 >> 3) << 11 | (199 >> 2) << 5 | (196 >> 3))
                                 : uint16_t((89 >> 3) << 11 | (239 >> 2) << 5 | (255 >> 3));
    // Copy the tiny descriptors before the first image request can invalidate
    // the borrowed parameter bytes. Each mask keeps its authored order.
    Mask masks[MaskCount];
    for (unsigned i = 0; i < MaskCount; ++i)
        masks[i] = parameters.mask(i);
    const bool direct = scene_opaque_canvas(c);
    uint16_t contributions[256];
    unsigned previous = ~0u;
    for (unsigned i = first; i < first + count; ++i) {
        const auto m = masks[i];
        const auto image = c.a.content.image(m.image);
        if (!image.pixels || image.format || image.w * 2 != m.stride || image.h != m.h ||
            image.alpha || image.palette)
            return;
        // Original Vakuu light_overlay4 alone has node modulate alpha140/255.
        const unsigned a = i == 6 ? div255(alpha * 140) : alpha;
        if (direct && previous != a) {
            for (unsigned k = 0; k < 256; ++k)
                contributions[k] = presentation::add_opaque(0, color, div255(k * a));
            previous = a;
        }
        if (m.x >= unsigned(c.clip_w) || m.y >= unsigned(c.clip_h))
            continue;
        const unsigned width = mn(m.w, unsigned(c.clip_w) - m.x),
                       height = mn(m.h, unsigned(c.clip_h) - m.y);
        for (unsigned y = 0; y < height; ++y) {
            const auto* row = image.pixels + y * m.stride;
            const unsigned at = (m.y + y) * c.pitch + m.x;
            for (unsigned x = 0; x < width; ++x) {
                if (direct) {
                    const uint16_t amount = contributions[row[x]];
                    if (amount)
                        c.pixels[at + x] = presentation::add_preweighted(c.pixels[at + x], amount);
                } else if (row[x])
                    c.paint(at + x, color, div255(row[x] * a), presentation::Add);
            }
        }
    }
}
static void sparkles(Canvas& c, unsigned auxiliary, uint32_t elapsed, uint32_t seed) {
    Parameters parameters;
    if (!ancient_gpu_masks::parameters(c, auxiliary, parameters))
        return;
    const auto image = c.a.content.image(parameters.dot);
    if (!image.pixels || image.w != 24 || image.h != 24 || image.format != 1 || !image.alpha)
        return;
    const water_reflection::Texture texture(image);
    struct Node {
        int x, y;
        int16_t angle, sx, sy, ex, ey;
        uint8_t count;
    };
    using namespace fake_merchant;
    static constexpr Node nodes[] = {
        {source(1034), source(1076), -341, fixed(1), fixed(1), fixed(.905), fixed(.235), 20},
        {source(1684), source(852), 0, fixed(1), fixed(1), fixed(.8), fixed(.4), 40},
        {source(1960), source(913.0001), -291, fixed(.965), fixed(.615), fixed(.8), fixed(.4), 20},
        {source(2112), source(823.00006), -422, fixed(.965), fixed(.615), fixed(.41), fixed(.4),
         20},
        {source(2073), source(868), -422, fixed(.965), fixed(.615), fixed(.41), fixed(.4), 20}};
    for (unsigned node = 0; node < 5; ++node) {
        const auto& n = nodes[node];
        const int sn = CombatCardMotion::sine(n.angle), cs = CombatCardMotion::sine(n.angle + 1024);
        for (unsigned index = 0; index < n.count; ++index) {
            const auto p = ancient_gpu_masks::sparkle(elapsed, seed, node, index, n.count);
            const int x = int(int64_t(p.x) * n.ex * n.sx / (Q * Q)),
                      y = int(int64_t(p.y) * n.ey * n.sy / (Q * Q));
            const int cx = n.x + (x * cs - y * sn) / 1024, cy = n.y + (x * sn + y * cs) / 1024;
            const unsigned phase = p.age * 65536u / 5000;
            const auto* colors = parameters.data + 384;
            const ColorKey color = {
                0, uint8_t(ramp(colors, 4, 0, phase)), uint8_t(ramp(colors, 4, 1, phase)),
                uint8_t(ramp(colors, 4, 2, phase)), uint8_t(ramp(colors, 4, 3, phase))};
            const int side = ancient_size(24) * p.scale / Q;
            quad(c, texture, ancient_x(cx), ancient_y(cy), side * n.sx / Q, side * n.sy / Q,
                 n.angle, div255(243 * color.r), div255(210 * color.g), div255(153 * color.b),
                 color.a, presentation::Normal);
        }
    }
}
} // namespace ancient_gpu_masks
