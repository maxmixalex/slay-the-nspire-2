#pragma once
#include <stdint.h>
namespace spire {
// Unicode format controls have no independent raster or advance. In
// particular emoji VS15/VS16 select presentation; they are never '?' glyphs.
// Keep them in saved/localized UTF-8, but omit them from the visible stream.
inline bool text_format_control(uint32_t cp){
    return cp==0x00ad||(cp>=0x200b&&cp<=0x200f)||(cp>=0x202a&&cp<=0x202e)||
        (cp>=0x2060&&cp<=0x206f)||cp==0xfeff||(cp>=0xfe00&&cp<=0xfe0f)||
        (cp>=0xe0100&&cp<=0xe01ef);
}
}
