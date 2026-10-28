#pragma once

namespace spire { namespace card_face {
// Authored card.tscn coordinates (300 x 422), pinned v0.111.0. Shared by
// hand, rewards, collection, shop and upgrade comparisons so the portrait
// cannot drift into the title or body when a card also has a star cost.
struct Rect {int x=0,y=0,w=0,h=0;};
struct Layout {Rect frame,portrait,border,banner,title,type,description,energy,star;};
inline Rect focus_bounds(int x,int y,int w,int h,int inset=2){
    return {x-inset,y-inset,w+inset*2,h+inset*2};
}
inline int scaled(int value,int scale,int base){
    return value<0?-((-value*scale+base/2)/base):(value*scale+base/2)/base;
}
inline Rect region(int x,int y,int w,int h,int left,int top,int width,int height){
    return {x+scaled(left,w,300),y+scaled(top,h,422),scaled(width,w,300),scaled(height,h,422)};
}
inline Layout layout(int x,int y,int w,int h){
    return {{x,y,w,h},region(x,y,w,h,25,43,250,190),
        region(x,y,w,h,13,47,275,210),region(x,y,w,h,-13,4,327,83),
        region(x,y,w,h,45,7,210,54),region(x,y,w,h,120,212,61,37),
        region(x,y,w,h,28,248,243,136),region(x,y,w,h,-16,-16,64,64),
        region(x,y,w,h,-36,22,58,58)};
}
// Original card.tscn fonts: title26, body21 (minimum12), type16,
// energy32 and stars22. Geometry helpers retain the authored scale; the
// renderer enlarges titles/descriptions for readability, while the compact
// type label retains its independent 16px source size with a 5px LCD floor.
inline int font_pixels(int source_size,int width){const int pixels=scaled(source_size,width,300);return pixels<5?5:pixels;}
// This follows the pinned CardRarity values, not reward rarity ordering.
// Basic/Token/None share Common; Ancient uses its scene's separate banner.
inline unsigned material_group(unsigned rarity){
    switch(rarity){case 3:return 1;case 4:return 2;case 9:return 3;
    case 8:return 4;case 6:return 5;case 10:return 6;case 5:return 7;default:return 0;}
}
}}
