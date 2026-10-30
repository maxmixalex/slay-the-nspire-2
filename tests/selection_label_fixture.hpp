#pragma once
#include "app.hpp"
#include "rich_text.hpp"
#include "font_metrics.hpp"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace spire_test {
// Compare the real screen's readable Label with independently decoded
// 8px glyphs and the original fractional advances/kerning. Its final word
// must survive; merely
// registering the checkbox hit or finding the right glyphs is insufficient.
inline bool selection_exact_glyphs(spire::App&a,const char*text,unsigned size,unsigned face,const std::vector<uint32_t>&keys){
    using namespace spire;if(!text||!*text)return false;
    RichReader reader(text);RichChar ch{};
    while(reader.next(ch))if(ch.codepoint>32&&!text_format_control(ch.codepoint)&&!inline_icon(ch.codepoint)){
        unsigned style=face?face:(ch.style.bold?5:0);bool exact=false;
        for(;;){const unsigned band=((style&3)<<6)|((style&4)<<3);
            if(std::binary_search(keys.begin(),keys.end(),uint32_t((size|band)<<24)|ch.codepoint)){exact=true;break;}
            if(!style)break;style=(style&4)?style&3:0;
        }
        if(!exact||!a.content.glyph(ch.codepoint,size,face?face:(ch.style.bold?5:0)).pixels){
            std::fprintf(stderr,"selection glyph missing U+%04X size=%u face=%u\n",ch.codepoint,size,face);return false;}
    }return true;
}
inline bool selection_upgrade_label_pixels(spire::App&a){
    using namespace spire;
    struct Letter {unsigned w,h;int x;std::vector<uint8_t>alpha;};
    const std::string text=a.content.world_string("card_selection","VIEW_UPGRADES");
    const unsigned face=a.content.original_label_fonts()?5:0;
    std::vector<Letter>letters;RichReader reader(text.c_str());RichChar ch{};int pen64=0;
    while(reader.next(ch)){
        const auto g=a.content.glyph(ch.codepoint,8,face);if(!g.pixels||!g.advance)return false;
        if(pen64)pen64+=a.content.glyph_kerning64(ch.previous,ch.codepoint,8,face);
        Letter letter{g.w,g.h,text_pixel(pen64)+g.bearing_x,std::vector<uint8_t>(unsigned(g.w)*g.h)};
        if(g.encoded_size){unsigned at=0;for(unsigned i=0;i<g.encoded_size;i+=2){
            if(i+1>=g.encoded_size||at+g.pixels[i]>letter.alpha.size())return false;
            for(unsigned n=0;n<g.pixels[i];++n)letter.alpha[at++]=g.pixels[i+1];
        }if(at!=letter.alpha.size())return false;}
        else for(unsigned i=0;i<letter.alpha.size();++i)letter.alpha[i]=g.pixels[i];
        pen64+=g.advance64?g.advance64:64*g.advance;letters.push_back(letter);
    }
    const int width=text_pixel(pen64);
    if(letters.empty()||width>140)return false;
    a.render();bool hit=false;
    for(unsigned i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
        if(h.kind==HitKind::UiAction&&h.value==int(UiAction::DeckViewUpgrades))
            hit=h.enabled&&h.x==3&&h.y==195&&h.w==width+16&&h.h==14;
    }
    if(!hit)return false;
    const std::vector<uint16_t>actual(a.pixels,a.pixels+ScreenW*ScreenH);
    const int surface=a.active_card_grid();if(surface<0)return false;
    const auto grid=a.card_grids[surface];
    a.pointer.visible=false;a.render();std::vector<uint16_t>expected(a.pixels,a.pixels+ScreenW*ScreenH);
    a.card_grids[surface]=grid;a.pointer.visible=true;a.render();
    const auto blend=[](uint16_t from,uint16_t to,unsigned alpha){
        const auto channel=[&](unsigned x,unsigned y){return (x*(255-alpha)+y*alpha+127)/255;};
        return uint16_t(channel(from>>11,to>>11)<<11|channel((from>>5)&63,(to>>5)&63)<<5|channel(from&31,to&31));
    };
    const auto paint=[&](int dx,int dy,bool outline){
        for(const auto&letter:letters){
            for(unsigned yy=0;yy<letter.h;++yy)for(unsigned xx=0;xx<letter.w;++xx){
                const int sx=18+letter.x+int(xx)+dx,sy=197+int(yy)+dy;
                if(sx<0||sx>=ScreenW||sy<0||sy>=ScreenH)continue;
                unsigned alpha=letter.alpha[yy*letter.w+xx];if(outline)alpha=(alpha*128+127)/255;
                const auto color=uint16_t(outline?0:(239>>3)<<11|(200>>2)<<5|(81>>3));
                const unsigned at=unsigned(sy*ScreenW+sx);expected[at]=blend(expected[at],color,alpha);
            }
        }
    };
    paint(1,1,true);
    paint(0,0,false);
    unsigned visible=0;
    for(int y=195;y<210;++y)for(int x=18;x<18+width+2;++x){const unsigned at=unsigned(y*ScreenW+x);
        if(actual[at]!=expected[at]){
            std::fprintf(stderr,"View Upgrades full-label pixel mismatch at %d,%d: %u / %u\n",x,y,actual[at],expected[at]);return false;
        }if(actual[at])++visible;
    }
    return visible>0;
}
}
