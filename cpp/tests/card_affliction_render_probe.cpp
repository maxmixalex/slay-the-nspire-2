// Optional private-pack acceptance. Production App, range reader and draw
// paths; host CPU timing is not calculator FPS. No synthetic overlay renderer.
#include "app.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static App app;
struct Source {
    FILE* file;
    uint32_t length;
    uint64_t calls = 0, bytes = 0;
};
static bool read(void* context, uint32_t at, void* out, uint32_t size) {
    auto& s = *static_cast<Source*>(context);
    ++s.calls;
    s.bytes += size;
    return at <= s.length && size <= s.length - at && !std::fseek(s.file, long(at), SEEK_SET) &&
           std::fread(out, 1, size, s.file) == size;
}
static void check(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "Card acceptance: %s (%s)\n", what, app.debug_reply);
        std::exit(1);
    }
}
static void command(const char* text) {
    check(app.debug_command(text), text);
}
static uint64_t pixels() {
    uint64_t hash = 1469598103934665603ull;
    for (auto p : app.pixels) {
        hash ^= p;
        hash *= 1099511628211ull;
    }
    return hash;
}
static uint64_t hand_pixels() {
    uint64_t hash = 1469598103934665603ull;
    for (int y = 185; y < 240; ++y)
        for (int x = 45; x < 290; ++x) {
            hash ^= app.pixels[y * 320 + x];
            hash *= 1099511628211ull;
        }
    return hash;
}
static void picture(const char* directory, unsigned frame) {
    if (!directory)
        return;
    char name[1024];
    std::snprintf(name, sizeof name, "%s/card-%03u.ppm", directory, frame);
    FILE* f = std::fopen(name, "wb");
    check(f, "screenshot");
    std::fprintf(f, "P6\n320 240\n255\n");
    for (auto p : app.pixels) {
        const unsigned char b[] = {static_cast<unsigned char>((p >> 11) * 255 / 31),
                                   static_cast<unsigned char>(((p >> 5) & 63) * 255 / 63),
                                   static_cast<unsigned char>((p & 31) * 255 / 31)};
        std::fwrite(b, 1, 3, f);
    }
    std::fclose(f);
}
int main(int argc, char** argv) {
    check(argc >= 2, "pack argument");
    Source source{std::fopen(argv[1], "rb"), 0};
    check(source.file, "open");
    std::fseek(source.file, 0, SEEK_END);
    source.length = uint32_t(std::ftell(source.file));
    check(app.init({&source, source.length, read, nullptr}, {}), "App init");
    command("new Ironclad 73");
    command("fight BOWLBUGS_WEAK");
    while (app.game.hand.count < 7)
        command("card BASH Hand");
    const char* names[] = {"BOUND",   "ENTANGLED", "GALVANIZED", "HEXED",
                           "TAINTED", "RINGING",   "SMOG"};
    for (unsigned i = 0; i < 7; ++i) {
        char text[80];
        std::snprintf(text, sizeof text, "afflict %s 1 %u", names[i], i);
        command(text);
    }
    app.debug_open = app.paused = app.help = false;
    app.pointer.visible = false;
    app.render();
    unsigned preparation = 0;
    while (app.combat_preparation.active) {
        app.advance(33);
        check(++preparation < 10000, "preparation bounded");
    }
    // Cover the longest ordinary idle loop before testing sustained residency.
    for (unsigned i = 0; i < 360; ++i) {
        app.advance(33);
        app.render();
    }
    const uint64_t calls = source.calls, bytes = source.bytes;
    uint64_t previous = 0, previous_hand = 0;
    unsigned changes = 0, hand_changes = 0;
    auto start = std::chrono::steady_clock::now();
    for (unsigned frame = 0; frame < 120; ++frame) {
        app.advance(33);
        app.render();
        const uint64_t hash = pixels(), hand_hash = hand_pixels();
        changes += hash != previous;
        previous = hash;
        hand_changes += hand_hash != previous_hand;
        previous_hand = hand_hash;
        if (frame == 0 || frame == 30 || frame == 60 || frame == 90)
            picture(argc > 2 ? argv[2] : nullptr, frame);
    }
    const double elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    const auto used = app.content.image_cache_usage();
    std::printf("{\"frames\":120,\"families\":7,\"changed_frames\":%u,\"hand_changed_frames\":%u,"
                "\"host_mean_ms\":%.6f,\"warm_calls\":%llu,\"warm_bytes\":%llu,\"cache_pages\":%u,"
                "\"cache_entries\":%u,\"preparation_steps\":%u,\"reduced\":%s}\n",
                changes, hand_changes, elapsed / 120, (unsigned long long)(source.calls - calls),
                (unsigned long long)(source.bytes - bytes), used.pages, used.entries, preparation,
                app.content.reduced_animations() ? "true" : "false");
    check(app.content.healthy(), "healthy after hand render");
    check(changes > 100 && hand_changes > 100, "continuous presented hand clock");
    check(source.calls == calls, "seven-family warm cache residency");
    std::fclose(source.file);
}
