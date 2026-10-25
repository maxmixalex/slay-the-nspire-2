#pragma once
#include "app.hpp"
#include "nine_patch.hpp"

// The final rendered skin is checked independently through the public image
// view. Sample its right margin, outside both the source title icon and text.
// This keeps retained-intent tests sensitive to actual drawing after replacing
// the obsolete178px rectangular panel with the readable104px hover column.
inline bool intent_hover_panel(spire::App&a,spire::HoverRect*out=nullptr){
    using namespace spire;
    CardHoverTarget owner{};if(!creature_hover_target(a,owner)||!owner.intent)return false;
    HoverTipList tips{};collect_creature_hover_tips(a,owner,tips);
    if(tips.count!=1||tips.items[0].kind!=HoverTipKind::Intent||tips.items[0].amount!=owner.intent)return false;
    const auto page=card_hover_page(a,owner,tips,a.keyword_hover.page);
    if(page.count!=1||a.keyword_hover.owner!=owner.identity)return false;
    const auto r=page.pieces[0].box;if(out)*out=r;
    if(r.w!=104||r.x<3||r.x+r.w>317||r.y<3||r.y+r.h>237)return false;
    int id=a.content.world_image("common","HOVER_TIP");if(id<0)id=a.content.ui_image(U_HoverTip);
    const auto skin=a.content.image(id);
    // The public format fixture deliberately has no world-image directory.
    // It exercises the renderer's bounded fallback box, while real private
    // packs must still match the sampled source skin below.
    if(!skin.pixels||skin.format>3){
        constexpr uint16_t panel=uint16_t((27>>3)<<11|(29>>2)<<5|(31>>3));
        constexpr uint16_t gold=uint16_t((223>>3)<<11|(186>>2)<<5|(112>>3));
        for(int y=3;y<r.h-2;++y){
            if(a.pixels[(r.y+y)*ScreenW+r.x+r.w-1]!=gold)return false;
            for(int x=r.w-8;x<r.w-2;++x)if(a.pixels[(r.y+y)*ScreenW+r.x+x]!=panel)return false;
        }return r.h>5;
    }
    unsigned sampled=0;
    for(int y=4;y<r.h-3;++y)for(int x=r.w-8;x<r.w-3;++x){
        const unsigned sx=nine_patch::coordinate(x,r.w,65,10,17)*skin.w/65;
        const unsigned sy=nine_patch::coordinate(y,r.h,20,8,6)*skin.h/20;
        const unsigned at=sy*skin.w+sx;if(skin.alpha&&skin.alpha[at]!=255)continue;
        const auto*pixel=skin.palette?skin.palette+2*skin.pixels[at]:skin.pixels+2*at;
        const uint16_t expected=uint16_t(pixel[0]|unsigned(pixel[1])<<8);
        if(a.pixels[(r.y+y)*ScreenW+r.x+x]!=expected)return false;
        ++sampled;
    }
    return sampled>8;
}
