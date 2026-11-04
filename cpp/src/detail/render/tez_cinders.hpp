#pragma once
namespace tez_cinders {
using namespace tez_cinders;
template <class SourceTable>
static void particle(Canvas c, const water_reflection::Texture& texture, const Particle& p,
                     const SourceTable& table, unsigned emitter, unsigned frame) {
    if (!p.active)
        return;
    const unsigned age = age_q16(p);
    const int scale = curve(table.source_curve(1), age);
    const int opacity = int((int64_t(curve(table.source_curve(2), age)) * 255 + 524288) >> 20);
    if (scale <= 0 || opacity <= 0)
        return;
    const int32_t m0 = table.coordinate(emitter, 0, frame),
                  m1 = table.coordinate(emitter, 1, frame),
                  m2 = table.coordinate(emitter, 2, frame),
                  m3 = table.coordinate(emitter, 3, frame);
    const int cx =
        table.coordinate(emitter, 4, frame) + int((int64_t(m0) * p.x + int64_t(m1) * p.y) >> 28);
    const int cy =
        table.coordinate(emitter, 5, frame) + int((int64_t(m2) * p.x + int64_t(m3) * p.y) >> 28);
    const int sn = CombatCardMotion::sine(p.orientation),
              cs = CombatCardMotion::sine(p.orientation + 1024);
    const int64_t size = (int64_t(p.scale4096) * scale * 24 + 524288) >> 20;
    const int a = int(((int64_t(m0) * cs + int64_t(m1) * sn) * size) >> 34),
              b = int(((-int64_t(m0) * sn + int64_t(m1) * cs) * size) >> 34);
    const int d = int(((int64_t(m2) * cs + int64_t(m3) * sn) * size) >> 34),
              e = int(((-int64_t(m2) * sn + int64_t(m3) * cs) * size) >> 34);
    const int64_t determinant = int64_t(a) * e - int64_t(b) * d;
    if (determinant <= 0)
        return;
    const int hw = (CombatCardMotion::magnitude(a) + CombatCardMotion::magnitude(b) + 1) / 2;
    const int hh = (CombatCardMotion::magnitude(d) + CombatCardMotion::magnitude(e) + 1) / 2;
    const int left = mx(0, (cx - hw) / 256 - 1), right = mn(c.clip_w, (cx + hw) / 256 + 2);
    const int top = mx(0, (cy - hh) / 256 - 1), bottom = mn(c.clip_h, (cy + hh) / 256 + 2);
    if (left >= right || top >= bottom)
        return;
    c.tint_red = uint8_t(div255(c.tint_red * p.red));
    c.tint_green = uint8_t(div255(c.tint_green * p.green));
    c.tint_blue = uint8_t(div255(c.tint_blue * p.blue));
    c.opacity = uint8_t(div255(c.opacity * unsigned(opacity)));
    const int du = int(int64_t(e) * 16777216 / determinant),
              dv = int(-int64_t(d) * 16777216 / determinant);
    for (int y = top; y < bottom; ++y) {
        const int x = left * 256 + 128 - cx, dy = y * 256 + 128 - cy;
        int u = 32768 + int((int64_t(x) * e - int64_t(dy) * b) * 65536 / determinant);
        int v = 32768 + int((-int64_t(x) * d + int64_t(dy) * a) * 65536 / determinant);
        for (int x = left; x < right; ++x, u += du, v += dv) {
            if (unsigned(u) >= 65536 || unsigned(v) >= 65536)
                continue;
            const auto pixel = texture.bilinear(u, v);
            if (pixel.alpha && (pixel.color || c.coverage))
                c.paint(y * c.pitch + x, pixel.color, pixel.alpha, presentation::Add);
        }
    }
}
static bool draw(Canvas& c, uint32_t elapsed, uint32_t seed) {
    const int ids[] = {int(FirstImage + 1), int(FirstImage), c.a.content.ui_image(I13_RandomLight),
                       c.a.content.ui_image(ancient_lights::AuxiliaryFirst + 1)};
    const auto parameters = c.a.content.image(ids[0]);
    const auto sprite = c.a.content.image(ids[1]);
    const auto light = c.a.content.image(ids[2]);
    const auto light_parameters = c.a.content.image(ids[3]);
    // The complete borrowed window must remain live after the final request.
    for (int id : ids)
        if (!c.a.content.image_cached(id))
            return false;
    Table table;
    ancient_lights::Table light_table;
    const water_reflection::Texture texture(sprite, 24, 24), light_texture(light, 256, 256);
    if (!table.read(parameters) || !light_table.read(light_parameters, 1) || !texture.valid() ||
        !light_texture.valid())
        return false;
    const uint32_t clock = c.a.content.reduced_animations() ? 0 : elapsed;
    auto& state = tez_cinder_room(c.a).state;
    state.advance(
        clock, seed, table.source_curve(0), [&]() { c.poll(); },
        [&](unsigned emitter, int32_t born) { return table.inherit(emitter, born); }, false,
        CalculatorBackgroundStride);
    const unsigned frame = table.frame(clock);
    for (unsigned i = 0; i < 164; ++i) {
        const unsigned command = table.command(i);
        if (command & 0x8000) {
            const unsigned emitter = command & 0x7fff;
            for (unsigned j = presentation_first(emitter); j < 24; j += presentation_step(emitter))
                particle(c, texture, state.particles[emitter * 24 + j], table, emitter, frame);
        }
    }
    // This older resource path contains additive cinders/lights only.
    SoftAddGrid glow(c);
    if (!ancient_lights::nodes(glow.canvas(), light_texture, light_table, 1, clock, seed, 0, ~0u,
                               &glow))
        return false;
    glow.finish();
    return true;
}
} // namespace tez_cinders
