#pragma once
#include "app.hpp"
#include "rich_text.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace spire_test {
// Compare the real screen's fixed-size Label with independently decoded
// 4px glyphs. In particular, its complete final word must survive; merely
// registering the checkbox hit or finding the right glyphs is insufficient.
inline bool selection_upgrade_label_pixels(spire::App&a){
    using namespace spire;
    struct Letter {unsigned w,h,advance;std::vector<uint8_t>alpha;};
    const std::string text=a.content.world_string("card_selection","VIEW_UPGRADES");
    std::vector<Letter>letters;RichReader reader(text.c_str());RichChar ch{};int width=0;
    while(reader.next(ch)){
        const auto g=a.content.glyph(ch.codepoint,4,5);if(!g.pixels||!g.advance)return false;
        Letter letter{g.w,g.h,g.advance,std::vector<uint8_t>(unsigned(g.w)*g.h)};
        if(g.encoded_size){unsigned at=0;for(unsigned i=0;i<g.encoded_size;i+=2){
            if(i+1>=g.encoded_size||at+g.pixels[i]>letter.alpha.size())return false;
            for(unsigned n=0;n<g.pixels[i];++n)letter.alpha[at++]=g.pixels[i+1];
        }if(at!=letter.alpha.size())return false;}
        else for(unsigned i=0;i<letter.alpha.size();++i)letter.alpha[i]=g.pixels[i];
        width+=g.advance;letters.push_back(letter);
    }
    if(letters.empty()||width>140)return false;
    a.render();bool hit=false;
    for(unsigned i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
        if(h.kind==HitKind::UiAction&&h.value==int(UiAction::DeckViewUpgrades))
            hit=h.enabled&&h.x==3&&h.y==227&&h.w==width+11&&h.h==9;
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
    const auto paint=[&](int dx,int dy,bool outline){int x=13;
        for(const auto&letter:letters){
            for(unsigned yy=0;yy<letter.h;++yy)for(unsigned xx=0;xx<letter.w;++xx){
                const int sx=x+int(xx)+dx,sy=229+int(yy)+dy;
                if(sx<0||sx>=ScreenW||sy<0||sy>=ScreenH)continue;
                unsigned alpha=letter.alpha[yy*letter.w+xx];if(outline)alpha=(alpha*128+127)/255;
                const auto color=uint16_t(outline?0:(239>>3)<<11|(200>>2)<<5|(81>>3));
                const unsigned at=unsigned(sy*ScreenW+sx);expected[at]=blend(expected[at],color,alpha);
            }x+=letter.advance;
        }
    };
    for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)if((dx||dy)&&dx*dx+dy*dy<=4)paint(dx,dy,true);
    paint(0,0,false);
    unsigned visible=0;
    for(int y=227;y<ScreenH;++y)for(int x=13;x<13+width+2;++x){const unsigned at=unsigned(y*ScreenW+x);
        if(actual[at]!=expected[at]){
            std::fprintf(stderr,"View Upgrades full-label pixel mismatch at %d,%d: %u / %u\n",x,y,actual[at],expected[at]);return false;
        }if(actual[at])++visible;
    }
    return visible>0;
}
}
