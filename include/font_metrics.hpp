#pragma once
#include <stdint.h>
namespace spire {
// UI40 retains the 16-byte glyph directory. High bit marks a 1/64px advance;
// three spare bits preserve a negative left bearing (0..7px). Legacy integer
// records remain readable. Pair adjustments use one private 20px metadata
// band per face and store 1/2048-em units, shared across all raster sizes.
constexpr uint16_t FractionalGlyphAdvance=0x8000,KerningGlyphAdvance=0x4000;
constexpr uint32_t KerningGlyphFirst=0x100000;
constexpr int KerningMetadataSize=20;
// The original locale chain maps BoldItalic and scene Bold to Bold, and
// scene Italic to Italic. Share their metadata without changing their glyphs.
inline unsigned kerning_font_style(unsigned style){style&=7u;return style==3||style==5?1u:style==6?2u:style;}
inline bool kerning_codepoint(uint32_t cp){return cp>=KerningGlyphFirst&&cp<KerningGlyphFirst+95*95;}
inline uint32_t kerning_codepoint(uint32_t left,uint32_t right){return KerningGlyphFirst+(left-32)*95+right-32;}
inline unsigned glyph_advance64(uint16_t stored){return stored&FractionalGlyphAdvance?stored&4095u:unsigned(stored)*64u;}
inline int glyph_bearing(uint16_t stored){return stored&FractionalGlyphAdvance?-int((stored>>12)&7u):0;}
inline bool valid_glyph_advance(uint32_t cp,uint16_t stored,unsigned version){
 if(version>=40&&kerning_codepoint(cp))return (stored&0xf000u)==KerningGlyphAdvance;
 return stored<=40||(version>=40&&(stored&FractionalGlyphAdvance)&&(stored&4095u)<=40*64);
}
inline int text_pixel(int position64){return (position64+32)>>6;}
}
