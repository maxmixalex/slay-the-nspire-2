#pragma once

namespace spire {
// v35 appends the exact small scene fonts. Glyph indexes and offsets are
// already 32-bit; this raises only the versioned count limit. The resident
// directory remains the same bounded 2 MiB allocation, including tile tables.
constexpr unsigned SceneFontContentVersion=35;
constexpr unsigned scene_font_glyph_limit(unsigned version){
    return version>=SceneFontContentVersion?65535u:32768u;
}
}
