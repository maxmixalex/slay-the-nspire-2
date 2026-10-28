#pragma once
#include <stdint.h>

namespace spire {
struct TouchSample {
    uint16_t x{}, y{}, width{}, height{};
    bool contact{}, pressed{}, valid{};
};
struct PointerUpdate {
    int x{}, y{};
    bool moved{}, down{}, up{}, tap{}, cancelled{};
};

// Relative cursor motion, with finger-lift clutching and fractional accumulation.
// Neither input filtering nor visual feedback may consume gameplay RNG.
class TouchpadCursor {
    int x_ = 160, y_ = 120;
    int last_x_{}, last_y_{}, fraction_x_{}, fraction_y_{}, travel_{};
    uint32_t contact_start_{};
    bool contact_{}, pressed_{}, clicked_{}, suppress_tap_{}, primed_{};
public:
    void reset(int x = 160, int y = 120) {
        x_ = x; y_ = y; contact_ = pressed_ = primed_ = false;
        fraction_x_ = fraction_y_ = 0; suppress_tap_ = true;
    }
    PointerUpdate sample(TouchSample s, uint32_t now_ms, bool tap_to_click = true) {
        PointerUpdate out{x_, y_};
        if (!s.valid || !s.width || !s.height || s.x > s.width || s.y > s.height) {
            out.cancelled = pressed_ || contact_;
            contact_ = pressed_ = primed_ = false;
            fraction_x_ = fraction_y_ = 0;
            suppress_tap_ = true;
            return out;
        }
        if (!primed_) {
            // Entering a new screen/input mode while the pad is held is not a click.
            primed_ = true; contact_ = s.contact; pressed_ = s.pressed;
            last_x_ = s.x; last_y_ = s.y; contact_start_ = now_ms;
            travel_ = 0; clicked_ = s.pressed; suppress_tap_ = s.contact || s.pressed;
            return out;
        }
        if (s.contact && !contact_) {
            last_x_ = s.x; last_y_ = s.y; contact_start_ = now_ms;
            fraction_x_ = fraction_y_ = travel_ = 0;
            clicked_ = false; suppress_tap_ = false;
        } else if (s.contact && contact_) {
            int dx = (int)s.x - last_x_, dy = last_y_ - (int)s.y;
            last_x_ = s.x; last_y_ = s.y;
            // A discontinuous device report must not fling the cursor or create a tap.
            if (dx > s.width/2 || dx < -(int)s.width/2 ||
                dy > s.height/2 || dy < -(int)s.height/2) {
                suppress_tap_ = true; fraction_x_ = fraction_y_ = 0;
            } else {
                fraction_x_ += dx * 448;
                fraction_y_ += dy * 336;
                int px = fraction_x_ / s.width, py = fraction_y_ / s.height;
                fraction_x_ -= px * s.width; fraction_y_ -= py * s.height;
                travel_ += (px < 0 ? -px : px) + (py < 0 ? -py : py);
                x_ += px; y_ += py;
                if (x_ < 0) x_ = 0; if (x_ > 319) x_ = 319;
                if (y_ < 0) y_ = 0; if (y_ > 239) y_ = 239;
                out.moved = px != 0 || py != 0;
            }
        }
        if (s.pressed && !pressed_) {
            clicked_ = true;
            if (!suppress_tap_) out.down = true;
        }
        if (!s.pressed && pressed_ && !suppress_tap_) out.up = true;
        if (!s.contact && contact_) {
            uint32_t duration = now_ms - contact_start_;
            out.tap = tap_to_click && !clicked_ && !suppress_tap_ &&
                      !s.pressed && duration >= 30 && duration <= 220 && travel_ <= 4;
            fraction_x_ = fraction_y_ = 0;
        }
        if (!s.contact && !s.pressed) suppress_tap_ = false;
        contact_ = s.contact; pressed_ = s.pressed;
        out.x = x_; out.y = y_;
        return out;
    }
};
} // namespace spire
