#include "map_preview.hpp"
namespace spire {
namespace {
// Dedicated bounded scratch space. A foreground application generates one map
// at a time; neither gameplay state nor its legacy RNG is used here.
reference::MapWorkspace workspace;
}
bool MapPreview::generate() {
    const char prefix[] = "NSPIRE-";
    unsigned n = 0; while (prefix[n]) { seed[n] = prefix[n]; ++n; }
    char digits[12]; unsigned count = 0; uint32_t value = seed_number;
    do { digits[count++] = char('0' + value % 10); value /= 10; } while (value);
    while (count) seed[n++] = digits[--count]; seed[n] = 0;
    char canonical[24];
    if (!reference::canonicalize_ascii(seed, canonical, sizeof(canonical))) return false;
    unsigned length = 0; while (canonical[length]) { seed[length] = canonical[length]; ++length; } seed[length] = 0;
    reference::Random rng; rng.named(reference::hash64(canonical, length), "act_1_map");
    first_row = 0; selected = -1;
    // Explicit quotas exercise the reference constructor without pretending
    // that Gaussian quota generation or the complete run pipeline is ported.
    generated = reference::generate_map(map, rng, reference::MapConfig{}, workspace);
    return generated;
}
void MapPreview::change_seed(int direction) {
    if (direction < 0) { if (seed_number) --seed_number; }
    else if (seed_number < 999999) ++seed_number;
    generate();
}
}
