#pragma once
#include <stdint.h>
#include "health_bar.hpp"

namespace spire {
enum class CardGridSurface : uint8_t { Pile, Library, DeckChoice, CombatChoice, WorldChoice, Count };

// NCardGrid / ScrollHelper, projected from a 1440x1080 viewport to 320x240.
// The fixed-point position belongs to UI presentation, never to a saved pile.
struct CardGridScroll {
    static constexpr int Unit=256, Columns=4, CardW=53, CardH=75, StepX=62, StepY=84;
    static constexpr int Left=39, BottomMargin=71, ThumbSize=16;
    uint32_t identity=0,epoch=0,hover_changed=0;
    int32_t position=0,target=0;
    int16_t count=0,top=0,bottom=0,origin=0,focused_index=-1,left=Left,custom_height=-1;
    uint8_t columns=Columns;bool center_short=true;
    bool bound=false,dragging=false,thumb_drag=false,thumb_hover=false;
    struct FaceMotion {uint32_t start=0;int16_t index=-1;bool focused=false;};
    FaceMotion faces[16]{};

    static int clamp(int value,int low,int high){return value<low?low:value>high?high:value;}
    static int fraction(int value,int thousandths){return (value/1000)*thousandths+(value%1000)*thousandths/1000;}
    int rows()const{return (count+columns-1)/columns;}
    int height()const{return bottom-top;}
    int cards_height()const{return custom_height>=0?custom_height:rows()?CardH+(rows()-1)*StepY:0;}
    int extent()const{return custom_height>=0?custom_height:origin+cards_height()+BottomMargin;}
    int minimum()const{return center_short&&extent()<height()?-(height()-extent())*Unit/2:0;}
    int maximum()const{return extent()>height()?(extent()-height())*Unit:minimum();}
    bool has_scrollbar()const{return count&&origin+cards_height()>height();}
    int offset()const{return position/Unit;}
    int card_y(int index)const{return top+origin+(index/columns)*StepY-offset();}
    int card_x(int index)const{return left+(index%columns)*StepX;}
    int first()const{
        const int above=offset()-origin-CardH-4;
        return clamp((above>0?(above/StepY+1)*columns:0),0,count);
    }
    int last()const{
        const int below=height()+offset()-origin+4;
        return clamp((below>0?(below/StepY+1)*columns:0),0,count);
    }
    int bar_top()const{return top+29;}
    int bar_bottom()const{return bottom-29;}
    int thumb_y()const{
        const int range=maximum();
        return bar_top()+(range>0?(bar_bottom()-bar_top())*clamp(thumb_drag?target:position,0,range)/range:0);
    }
    void reset(){*this=CardGridScroll{};}
    void configure(uint32_t key,uint32_t generation,int total,int y0,int y1,int inset,int column_count=Columns,int x0=Left,bool centered=true,int content_height=-1){
        const bool changed=!bound||identity!=key||epoch!=generation||count!=total||top!=y0||bottom!=y1||origin!=inset||columns!=column_count||left!=x0||center_short!=centered||custom_height!=content_height;
        identity=key;epoch=generation;count=int16_t(total);top=int16_t(y0);bottom=int16_t(y1);origin=int16_t(inset);columns=uint8_t(column_count);left=int16_t(x0);center_short=centered;custom_height=int16_t(content_height);bound=true;
        if(changed){position=target=minimum();dragging=thumb_drag=false;thumb_hover=false;hover_changed=0;focused_index=-1;for(auto&face:faces)face=FaceMotion{};}
    }
    int face_scale(int index,uint32_t now)const{
        const auto&face=faces[unsigned(index)%16];if(face.index!=index)return 800;
        if(face.focused)return 1000;const unsigned elapsed=now-face.start;
        if(elapsed>=500)return 800;
        return 800+200*int(4096-health_bar::expo_out(elapsed*2))/4096;
    }
    int observe_face(int index,bool focused,uint32_t now){
        auto&face=faces[unsigned(index)%16];
        if(face.index!=index){face={};face.index=int16_t(index);face.start=now-500;}
        if(face.focused!=focused){face.focused=focused;face.start=now;}
        return face_scale(index,now);
    }
    void wheel(int rows){
        // ScrollHelper uses 40 source pixels per notch: 80/9 display pixels.
        target+=clamp(rows,-64,64)*80*Unit/9;
        target=clamp(target,minimum()-height()*Unit,maximum()+height()*Unit);
    }
    void focus(int index){
        if(index<0||index>=count)return;
        target=clamp((origin+(index/columns)*StepY+CardH/2-height()/2)*Unit,minimum(),maximum());
    }
    void drag_content(int delta){
        target=clamp(target-delta*Unit,minimum()-height()*Unit,maximum()+height()*Unit);
    }
    void drag_thumb(int y){
        target=maximum()>0?maximum()*clamp(y-bar_top(),0,bar_bottom()-bar_top())/(bar_bottom()-bar_top()):minimum();
    }
    void advance(unsigned elapsed){
        if(!bound||!elapsed)return;
        if(elapsed>250)elapsed=250;
        // NCardGrid interpolates at 15/s and snaps at half a display pixel.
        const int amount=clamp(int(elapsed)*15,0,1000);
        position+=fraction(target-position,amount);
        const int distance=target-position;if(distance>-Unit/2&&distance<Unit/2)position=target;
        if(!dragging&&!thumb_drag){
            const int end=clamp(target,minimum(),maximum());
            const int bounce=clamp(int(elapsed)*12,0,1000);
            target+=fraction(end-target,bounce);
            const int remaining=end-target;if(remaining>-Unit/2&&remaining<Unit/2)target=end;
        }
    }
};
static_assert(sizeof(CardGridScroll)*unsigned(CardGridSurface::Count)<=2048,"Card grid state must stay bounded");
}
