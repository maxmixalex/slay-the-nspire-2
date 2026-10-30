#include "map_preview.hpp"
namespace spire {
namespace {
void copy(char* out, const char* in) { unsigned i = 0; do { out[i] = in[i]; } while (in[i++]); }
}
bool MapPreview::generate() {
    char canonical[65];
    if (!reference::canonicalize_seed(seed, canonical, sizeof(canonical))) {
        generated = false; error = "Seed must fit in 64 UTF-8 bytes."; return false;
    }
    copy(seed, canonical); first_row = 0; selected = -1;
    uint64_t value=0;
    generated = reference::seed_hash(seed,value)&&reference::generate_act_map(map,value,settings,quota);
    error = generated ? "" : quota.error == reference::QuotaError::AmbiguousRounding ?
        "舍入临界值：停止生成，未替换结果" : "生成失败，未使用替代地图";
    return generated;
}
bool MapPreview::set_seed(const char* input) {
    char canonical[65];
    if (!reference::canonicalize_seed(input, canonical, sizeof(canonical))) {
        error = "Seed must fit in 64 UTF-8 bytes."; return false;
    }
    copy(seed, canonical); return generate();
}
void MapPreview::change_seed(int direction) {
    if (direction < 0) { if (seed_number) --seed_number; }
    else if (seed_number < 999999) ++seed_number;
    const char prefix[] = "NSP1RE-"; unsigned n = 0;
    while (prefix[n]) { seed[n] = prefix[n]; ++n; }
    char digits[12]; unsigned count = 0; uint32_t value = seed_number;
    do { digits[count++] = char('0' + value % 10); value /= 10; } while (value);
    while (count) seed[n++] = digits[--count]; seed[n] = 0;
    generate();
}
void MapPreview::change_act() {
    settings.act = reference::ActKind((unsigned(settings.act) + 1) % 4); generate();
}
void MapPreview::begin_edit() {
    copy(draft, seed); editing = true; editor_selection = SeedButtonCount - 1; error = "";
}
void MapPreview::edit(int ch) {
    if (!editing) return;
    if (ch == 27) { editing = false; error = ""; }
    else if (ch == 13) { if (set_seed(draft)) editing = false; }
    else if (ch == 8) { reference::seed_backspace(draft); error = ""; }
    else if (ch == -1) { draft[0] = 0; error = ""; }
    else if (ch >= 32) error = reference::seed_append(draft,sizeof draft,uint32_t(ch)) ? "" : "Seed must fit in 64 UTF-8 bytes.";
}
}
