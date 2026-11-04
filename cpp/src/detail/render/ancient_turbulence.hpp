#pragma once
// Source GPU turbulence is sampled by the desktop converter. Every particle
// retains its authored capacity, lifetime, color/scale envelope and draw order.
// One bounded view owns mask+curves+tracks: no stale pointer across image loads.
namespace ancient_turbulence {
using namespace ancient_turbulence;
static void draw(Canvas& c, unsigned first, unsigned count, uint32_t elapsed, uint32_t seed) {
    const int root = c.a.content.ui_image(AuxiliaryFirst);
    if (root != int(FirstImage) || first > EmitterCount || count > EmitterCount - first)
        return;
    for (unsigned node = first; node < first + count; ++node) {
        const auto image = c.a.content.image(root + int(node));
        Table table;
        if (!image.pixels || image.format || image.alpha || image.palette ||
            !view_geometry(node, image.w, image.h) ||
            !table.read(image.pixels, unsigned(image.w) * image.h * 2))
            return;
        const auto* alpha = table.data + table.texture;
        const Picture mask{
            alpha, alpha, table.data + table.palette, uint16_t(table.width), uint16_t(table.height),
            3};
        const water_reflection::Texture texture(mask);
        const uint32_t node_seed = hash(seed ^ node * 0x85ebca6bu);
        for (unsigned particle = 0; particle < table.count; ++particle) {
            Sample p;
            if (!table.sample(particle, elapsed, node_seed, p))
                continue;
            const unsigned alpha_value =
                (table.ramp(3, p.phase) * table.ramp(4, p.phase) + 32767u) / 65535u;
            const unsigned opacity = (alpha_value * table.tint(3) + 32767u) / 65535u;
            const unsigned scale = (p.scale * table.ramp(5, p.phase) + 2048u) >> 12;
            if (!opacity || !scale)
                continue;
            const int width = int(table.extent_x * scale / 4096u),
                      height = int(table.extent_y * scale / 4096u);
            fake_merchant::quad(c, texture, p.x, p.y, width, height, p.angle,
                                (table.ramp(0, p.phase) * table.tint(0) + 32767u) / 65535u,
                                (table.ramp(1, p.phase) * table.tint(1) + 32767u) / 65535u,
                                (table.ramp(2, p.phase) * table.tint(2) + 32767u) / 65535u, opacity,
                                table.blend ? presentation::Add : presentation::Normal);
        }
    }
}
} // namespace ancient_turbulence
