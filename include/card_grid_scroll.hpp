#pragma once
#include <stdint.h>
#include "health_bar.hpp"
#include "reference_viewport.hpp"

namespace spire {
enum class CardGridSurface : uint8_t { Pile, Library, DeckChoice, CombatChoice, WorldChoice, Count };

// NCardGrid / ScrollHelper, using the pinned 1680x1260 FourByThree viewport.
// The fixed-point position belongs to UI presentation, never to a saved pile.
struct CardGridScroll {
    static constexpr int Unit=256, Columns=4, CardW=46, CardH=64, StepX=53, StepY=72;
    static constexpr int Left=52, BottomMargin=61, ThumbSize=14;
    // Keep fractional 377.6px source row spacing until the final projection.
    static int row_offset(int row){return (row*7552+52)/105;}
    uint32_t identity=0,epoch=0,hover_changed=0;
    int32_t position=0,target=0,thumb_position=0,thumb_velocity=0;
    int16_t count=0,top=0,bottom=0,origin=0,focused_index=-1,left=Left,custom_height=-1,bar_inset=25;
    uint8_t columns=Columns;bool center_short=true;
    bool bound=false,dragging=false,thumb_drag=false,thumb_hover=false;
    struct FaceMotion {uint32_t start=0,selection_start=0;int16_t index=-1;uint16_t selection_from=0;bool focused=false,chosen=false;};
    FaceMotion faces[16]{};

    static int clamp(int value,int low,int high){return value<low?low:value>high?high:value;}
    static int fraction(int value,int thousandths){return (value/1000)*thousandths+(value%1000)*thousandths/1000;}
    int rows()const{return (count+columns-1)/columns;}
    int height()const{return bottom-top;}
    int cards_height()const{return custom_height>=0?custom_height:rows()?CardH+row_offset(rows()-1):0;}
    int extent()const{return custom_height>=0?custom_height:origin+cards_height()+BottomMargin;}
    int minimum()const{return center_short&&extent()<height()?-(height()-extent())*Unit/2:0;}
    int maximum()const{return extent()>height()?(extent()-height())*Unit:minimum();}
    bool has_scrollbar()const{return count&&origin+cards_height()>height();}
    int offset()const{return position/Unit;}
    int card_y(int index)const{return top+origin+row_offset(index/columns)-offset();}
    int card_x(int index)const{return left+reference_viewport::project((index%columns)*280);}
    int first()const{
        const int above=offset()-origin-CardH-4;
        int row=above>0?above*105/7552:0;
        while(row<rows()&&row_offset(row)<=above)++row;
        return clamp(row*columns,0,count);
    }
    int last()const{
        const int below=height()+offset()-origin+4;
        int row=below>0?below*105/7552:0;
        while(row<rows()&&row_offset(row)<below)++row;
        return clamp(row*columns,0,count);
    }
    int bar_top()const{return top+bar_inset;}
    int bar_bottom()const{return bottom-25;}
    int thumb_target()const{
        const int range=maximum();
        return bar_top()+(range>0?(bar_bottom()-bar_top())*clamp(thumb_drag?target:position,0,range)/range:0);
    }
    int thumb_y()const{return thumb_position/Unit;}
    void reset(){*this=CardGridScroll{};}
    void configure(uint32_t key,uint32_t generation,int total,int y0,int y1,int inset,int column_count=Columns,int x0=Left,bool centered=true,int content_height=-1){
        const bool changed=!bound||identity!=key||epoch!=generation||count!=total||top!=y0||bottom!=y1||origin!=inset||columns!=column_count||left!=x0||center_short!=centered||custom_height!=content_height;
        identity=key;epoch=generation;count=int16_t(total);top=int16_t(y0);bottom=int16_t(y1);origin=int16_t(inset);columns=uint8_t(column_count);left=int16_t(x0);center_short=centered;custom_height=int16_t(content_height);bound=true;
        if(changed){position=target=minimum();thumb_position=bar_top()*Unit;thumb_velocity=0;dragging=thumb_drag=false;thumb_hover=false;hover_changed=0;focused_index=-1;for(auto&face:faces)face=FaceMotion{};}
    }
    int face_scale(int index,uint32_t now)const{
        const auto&face=faces[unsigned(index)%16];if(face.index!=index)return 800;
        if(face.focused)return 1000;const unsigned elapsed=now-face.start;
        if(elapsed>=500)return 800;
        return 800+200*int(4096-health_bar::expo_out(elapsed*2))/4096;
    }
    int observe_face(int index,bool focused,uint32_t now){
        auto&face=faces[unsigned(index)%16];
        if(face.index!=index){if(!focused)return 800;face={};face.index=int16_t(index);face.start=now-500;}
        if(face.focused!=focused){face.focused=focused;face.start=now;}
        return face_scale(index,now);
    }
    unsigned selection_width(const FaceMotion&face,uint32_t now)const{
        const unsigned age=now-face.selection_start;
        if(age>=500)return face.chosen?4915u:0u;
        const unsigned q=age*4096/500,v=4096-q;
        const unsigned ease=face.chosen?4096-unsigned(uint64_t(v)*v*v/16777216):q;
        const int target=face.chosen?4915:0;
        return unsigned(int(face.selection_from)+(target-int(face.selection_from))*int(ease)/4096);
    }
    unsigned observe_selection(int index,bool chosen,uint32_t now){
        auto&face=faces[unsigned(index)%16];
        if(face.index!=index){face={};face.index=int16_t(index);face.start=now-500;}
        if(face.chosen!=chosen){face.selection_from=uint16_t(selection_width(face,now));face.selection_start=now;face.chosen=chosen;}
        return selection_width(face,now);
    }
    void wheel(int rows){
        // ScrollHelper uses 40 source pixels per notch: 160/21 display pixels.
        target+=clamp(rows,-64,64)*160*Unit/21;
        target=clamp(target,minimum()-height()*Unit,maximum()+height()*Unit);
    }
    void focus(int index){
        if(index<0||index>=count)return;
        target=clamp((origin+row_offset(index/columns)+CardH/2-height()/2)*Unit,minimum(),maximum());
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
        // NScrollbar's handle follows Value through MathHelper.SmoothDamp,
        // with a 50ms smoothing time independently of the content lerp.
        // Q12 evaluates the same critically-damped rational polynomial.
        constexpr int Q=4096;
        const int destination=thumb_target()*Unit,change=thumb_position-destination;
        const int x=int(elapsed)*40*Q/1000;
        const int x2=int(int64_t(x)*x/Q),x3=int(int64_t(x2)*x/Q);
        const int decay=int(int64_t(Q)*Q/(Q+x+x2*48/100+x3*235/1000));
        const int temp=(thumb_velocity+40*change)*int(elapsed)/1000;
        thumb_velocity=int(int64_t(thumb_velocity-40*temp)*decay/Q);
        const int result=destination+int(int64_t(change+temp)*decay/Q);
        if((destination>thumb_position)==(result>destination)){thumb_position=destination;thumb_velocity=0;}
        else thumb_position=result;
        if(thumb_position-destination>-Unit/16&&thumb_position-destination<Unit/16&&thumb_velocity>-Unit&&thumb_velocity<Unit){thumb_position=destination;thumb_velocity=0;}
    }
};
static_assert(sizeof(CardGridScroll)*unsigned(CardGridSurface::Count)<=2048,"Card grid state must stay bounded");
}
