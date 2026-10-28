#pragma once
#include <stdint.h>

namespace spire {
// Version 32 changes only glyph payload storage. Its directory keys, original
// geometry and RLE bytes retain their v31 meaning after removing this flag.
constexpr unsigned GroupedGlyphContentVersion=32;
constexpr uint16_t GroupedGlyphFlag=0x8000u;
constexpr unsigned GroupedGlyphRawLimit=64u*1024u;
constexpr unsigned GroupedGlyphDescriptorBytes=16;
}
