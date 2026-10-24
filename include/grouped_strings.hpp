#pragma once
#include <stdint.h>
namespace spire {
// Version 33 preserves all string ids and UTF-8 bytes. Only this storage bit
// changes the interpretation of the offset; the locale marker remains raw.
constexpr unsigned GroupedStringContentVersion=33;
constexpr uint32_t GroupedStringFlag=0x80000000u;
constexpr unsigned GroupedStringRawLimit=32u*1024u;
constexpr unsigned GroupedStringDescriptorBytes=16;
}
