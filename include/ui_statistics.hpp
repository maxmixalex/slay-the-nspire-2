#pragma once
#include "ui_progress.hpp"
#include "ui_profiles.hpp"
#include "reference_viewport.hpp"
namespace spire {
struct UiStatisticsPresentation {
    UiProfileMotion entries[24]{};
    uint8_t selected=0;
};
static_assert(sizeof(UiStatisticsPresentation)<=320,"Statistics presentation must stay bounded");
// stats_screen.tscn at the FourByThree viewport. The MarginContainer's minimum
// is1096+370+500, wider than the viewport; grow-both centers that minimum.
// NScrollableContainer overrides only its Y position while scrolling.
struct UiStatisticsLayout {
    static constexpr int SourceLeft=(1680-(1096+370+500))/2+370;
    static constexpr int Left=reference_viewport::project(SourceLeft),Width=reference_viewport::project(1096);
    static constexpr int OverallTop=264,OverallHeight=480,CharacterHeader=768,CharacterTop=836;
    static constexpr int ScrollTop=58,ScrollLength=153;
    struct Point {int x=0,y=0,w=0,h=0;};
    Point entries[24]{},characters[5]{};
    uint8_t models[5]{},count=9,character_count=0;
    int height=240;
    explicit UiStatisticsLayout(const UiProgress&progress){
        for(unsigned i=0;i<9;++i)entries[i]={SourceLeft+24+int(i%2)*556,OverallTop+12+int(i/2)*92,500,i==8?88:92};
        static constexpr unsigned order[]={0,1,3,4,2};int top=CharacterTop,row_height=0;
        for(unsigned model:order){if(!(progress.characters_started&(1u<<model)))continue;
            const unsigned slot=character_count++;models[slot]=uint8_t(model);const auto&stat=progress.characters[model];
            const int playtime=stat.fastest_win_seconds?92:88,win_loss=stat.highest_ascension?92:88;
            const int h=24+48+4+playtime+win_loss+92,x=SourceLeft+int(slot%2)*564;
            characters[slot]={x,top,532,h};int y=top+12+48+4;
            entries[count++]={x+24,y,500,playtime};y+=playtime;
            entries[count++]={x+24,y,500,win_loss};y+=win_loss;
            entries[count++]={x+24,y,500,92};
            if(h>row_height)row_height=h;
            if(slot%2){top+=row_height+32;row_height=0;}
        }
        const int bottom=character_count?(row_height?top+row_height:top-32):CharacterTop;
        height=reference_viewport::project(bottom+100);if(height<240)height=240;
    }
    static Point projected(Point p){return {reference_viewport::project(p.x),reference_viewport::project(p.y),reference_viewport::project(p.w),reference_viewport::project(p.h)};}
    int neighbor(unsigned current,int dx,int dy)const{
        if(current>=count)return 0;const Point from=entries[current];int best=-1,score=0x7fffffff;
        for(unsigned i=0;i<count;++i){if(i==current)continue;const Point next=entries[i];
            const int x=next.x+next.w/2-from.x-from.w/2,y=next.y+next.h/2-from.y-from.h/2;
            const int along=dx?x*dx:y*dy,cross=dx?(y<0?-y:y):(x<0?-x:x);
            if(along<=0||(dx&&cross>48))continue;const int rank=along*4+cross*7;
            if(rank<score){score=rank;best=int(i);}
        }return best;
    }
};
}
