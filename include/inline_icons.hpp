#pragma once
#include <stdint.h>
namespace spire {
// Presentation-only private-use characters carry original sprite-font icons
// through the existing UTF-8 layout/ellipsis path. They are never save data,
// gameplay state, or replacement font glyphs in the resource file.
constexpr uint32_t InlineStarCodepoint=0xe000;
constexpr const char* InlineStarText="\xee\x80\x80";
constexpr uint32_t InlineEnergyCodepoint=0xe001;
constexpr const char* InlineEnergyText="\xee\x80\x81";
inline bool inline_icon(uint32_t codepoint){return codepoint==InlineStarCodepoint||codepoint==InlineEnergyCodepoint;}
}
