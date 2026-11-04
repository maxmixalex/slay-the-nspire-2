#pragma once
#include <stdint.h>

namespace spire {
struct TouchSample {
    uint16_t x{}, y{}, width{}, height{};
    bool contact{}, pressed{}, valid{};
};
// Ndless touchpad_scan reads page4 registers00..0A (inclusive). Coordinates
// are big endian; contact and the mechanical switch are independent bytes.
inline TouchSample decode_touchpad_report(const uint8_t* bytes, unsigned width, unsigned height) {
    return {uint16_t(unsigned(bytes[2]) << 8 | bytes[3]),
            uint16_t(unsigned(bytes[4]) << 8 | bytes[5]),
            uint16_t(width),
            uint16_t(height),
            bytes[0] != 0,
            bytes[10] != 0,
            true};
}
struct PointerUpdate {
    int x{}, y{};
    bool moved{}, down{}, up{}, tap{}, cancelled{};
};

// Relative cursor motion, with finger-lift clutching and fractional accumulation.
// Neither input filtering nor visual feedback may consume gameplay RNG.
class TouchpadCursor {
    int x_ = 160, y_ = 120;
    int last_x_{}, last_y_{}, fraction_x_{}, fraction_y_{}, travel_{};
    int anchor_x_{}, anchor_y_{};
    uint32_t contact_start_{};
    bool contact_{}, pressed_{}, clicked_{}, suppress_tap_{}, primed_{}, mouse_mode_ = true;

  public:
    void reset(int x = 160, int y = 120) {
        x_ = x;
        y_ = y;
        contact_ = pressed_ = primed_ = false;
        fraction_x_ = fraction_y_ = 0;
        suppress_tap_ = true;
        mouse_mode_ = true;
    }
    PointerUpdate sample(TouchSample s, uint32_t now_ms, bool tap_to_click = false,
                         unsigned speed = 2, bool mouse_visible = true) {
        PointerUpdate out{x_, y_};
        if (!s.valid || !s.width || !s.height || s.x > s.width || s.y > s.height) {
            out.cancelled = pressed_ || contact_;
            contact_ = pressed_ = primed_ = false;
            fraction_x_ = fraction_y_ = 0;
            suppress_tap_ = true;
            return out;
        }
        if (!mouse_visible && mouse_mode_) {
            // A keyboard action owns an already touching finger until lift.
            // Its pressure/release jitter cannot immediately steal focus back.
            suppress_tap_ |= contact_ || pressed_;
            fraction_x_ = fraction_y_ = 0;
            anchor_x_ = s.x;
            anchor_y_ = s.y;
        }
        mouse_mode_ = mouse_visible;
        if (!primed_) {
            // Entering a new screen/input mode while the pad is held is not a click.
            primed_ = true;
            contact_ = s.contact;
            pressed_ = s.pressed;
            last_x_ = anchor_x_ = s.x;
            last_y_ = anchor_y_ = s.y;
            contact_start_ = now_ms;
            travel_ = 0;
            clicked_ = s.pressed;
            suppress_tap_ = s.contact || s.pressed;
            return out;
        }
        if (s.contact && !contact_) {
            last_x_ = anchor_x_ = s.x;
            last_y_ = anchor_y_ = s.y;
            contact_start_ = now_ms;
            fraction_x_ = fraction_y_ = travel_ = 0;
            clicked_ = false;
            suppress_tap_ = false;
        } else if (s.contact && contact_ && !suppress_tap_) {
            int dx = (int)s.x - last_x_, dy = last_y_ - (int)s.y;
            const int ax = (int(s.x) - anchor_x_) * 448 / int(s.width),
                      ay = (anchor_y_ - int(s.y)) * 336 / int(s.height);
            const int distance = (ax < 0 ? -ax : ax) + (ay < 0 ? -ay : ay);
            if (distance > travel_)
                travel_ = distance;
            // A discontinuous device report must not fling the cursor or create a tap.
            if (dx > s.width / 2 || dx < -(int)s.width / 2 || dy > s.height / 2 ||
                dy < -(int)s.height / 2) {
                suppress_tap_ = true;
                fraction_x_ = fraction_y_ = 0;
            } else if (s.pressed == pressed_ &&
                       (mouse_visible || (!s.pressed && ax * ax + ay * ay >= 36))) {
                // Six normal-speed pixels from the contact anchor establish
                // deliberate motion. Oscillating noise never accumulates intent.
                if (!mouse_visible) {
                    dx = int(s.x) - anchor_x_;
                    dy = anchor_y_ - int(s.y);
                }
                if (speed > 4)
                    speed = 2;
                fraction_x_ += dx * int(224 + 112 * speed);
                fraction_y_ += dy * int(168 + 84 * speed);
                int px = fraction_x_ / s.width, py = fraction_y_ / s.height;
                fraction_x_ -= px * s.width;
                fraction_y_ -= py * s.height;
                x_ += px;
                y_ += py;
                if (x_ < 0)
                    x_ = 0;
                if (x_ > 319)
                    x_ = 319;
                if (y_ < 0)
                    y_ = 0;
                if (y_ > 239)
                    y_ = 239;
                out.moved = px != 0 || py != 0;
            }
        }
        // Button-edge reports often shift their centroid under pressure.
        // Anchor the following drag here, but click at the preceding cursor.
        last_x_ = s.x;
        last_y_ = s.y;
        if (s.pressed && !pressed_) {
            clicked_ = true;
            if (!suppress_tap_)
                out.down = true;
        }
        if (!s.pressed && pressed_ && !suppress_tap_)
            out.up = true;
        if (!s.contact && contact_) {
            uint32_t duration = now_ms - contact_start_;
            out.tap = tap_to_click && mouse_visible && !clicked_ && !suppress_tap_ && !s.pressed &&
                      duration >= 30 && duration <= 220 && travel_ <= 4;
            fraction_x_ = fraction_y_ = 0;
        }
        if (!s.contact && !s.pressed)
            suppress_tap_ = false;
        contact_ = s.contact;
        pressed_ = s.pressed;
        out.x = x_;
        out.y = y_;
        return out;
    }
};
} // namespace spire
