#pragma once
#include <stdint.h>
#include "health_bar.hpp"
#include "reference_viewport.hpp"
namespace spire {
namespace card_inspect_layout {
constexpr int CenterX=160,CenterY=120;
constexpr int CardW=reference_viewport::project(600),CardH=reference_viewport::project(844);
constexpr int ArrowSize=reference_viewport::project(128),ArrowY=reference_viewport::project(630-64);
constexpr int LeftArrowX=reference_viewport::project(840-488),RightArrowX=reference_viewport::project(840+360);
constexpr int Travel=reference_viewport::project(100);
constexpr int TipX=reference_viewport::project(840-353),TipY=reference_viewport::project(630-418);
constexpr int TipW=reference_viewport::project(706),TipH=reference_viewport::project(836);
constexpr int UpgradeY=reference_viewport::project(630+450),UpgradeH=reference_viewport::project(64);
}
// These models are UI-only. Their clocks never enter State or saved RNG.
struct DeckCountPulse {
    uint32_t start=0;unsigned high_water=0;bool active=false;
    int observe(unsigned count,uint32_t now){
        // NTopBarDeckButton only assigns its remembered count on an increase.
        // Preserve that pinned behavior after a removal, too.
        if(count>high_water){high_water=count;start=now;active=true;}
        const unsigned age=now-start;if(!active||age>=500){active=false;return 4096;}
        return 4096+2048*int(4096-health_bar::expo_out(age*2))/4096;
    }
};
struct PotionHolderMotion {
    uint32_t start=0,bounce=0;int16_t scale=4096,from=4096;bool focused=false,bouncing=false;
    int observe(bool next,bool bottle,uint32_t now){
        if(focused!=next){focused=next;start=now;from=scale;if(next&&bottle){bounce=now;bouncing=true;}}
        const unsigned age=now-start;
        if(focused){scale=int16_t(age>=50?4710:from+(4710-from)*int(age)/50);}
        else {const int q=age>=500?0:4096-int(health_bar::expo_out(age*2));
            scale=int16_t(4096+(from-4096)*q/4096);}
        if(now-bounce>=250)bouncing=false;
        return scale;
    }
};
struct PotionBeltMotion {
    uint32_t failed_at=0;bool failed=false;
    PotionHolderMotion holders[16]{};
    void fail(uint32_t now){failed=true;failed_at=now;}
    unsigned error_alpha(uint32_t now)const{
        if(!failed)return 0;const unsigned age=now-failed_at;
        // NPotionContainer: 150ms in, 350ms hold origin, 500ms out;
        // PotionErrorBg has authored self alpha 0.552941 (141/255).
        return age<150?age*141/150:age<350?141:age<850?(850-age)*141/500:0;
    }
};
enum class CardInspectSource:uint8_t { None,Library,DeckPile,InstancePile,DeckChoice,Hand,CombatChoice,WorldChoice,Reward,History };
struct CardInspectPresentation {
    CardInspectSource source=CardInspectSource::None;
    uint32_t opened=0,slid=0,closed=0;
    int closed_token=-1;
    int closed_energy=-2,closed_stars=-2;
    uint16_t closed_scale=8192;
    int16_t closed_x=card_inspect_layout::CenterX;
    uint8_t closed_root=255,closed_backstop=255,closed_face=255,closed_arrow=255;
    int16_t index=0,count=1;int8_t direction=0;
    bool view_all_upgraded=false,description_truncated=false,original_upgraded=false;
    bool upgrade_preview(bool upgraded)const{return upgraded&&!original_upgraded;}
    struct Arrow {
        uint32_t changed=0;uint8_t state=0;int16_t from_scale=4096;bool interacted=false;
        int scale(uint32_t now)const{
            if(state==1)return 4506;
            const unsigned age=now-changed;
            return age>=500?4096:4096+(from_scale-4096)*int(4096-health_bar::expo_out(age*2))/4096;
        }
        int shade(uint32_t now)const{
            if(!interacted)return 230;if(state==1)return 306;if(state==2)return 179;
            const unsigned age=now-changed;return age>=1000?230:230+76*int(4096-health_bar::expo_out(age))/4096;
        }
        void observe(uint8_t next,uint32_t now){if(next!=state){from_scale=int16_t(scale(now));changed=now;state=next;interacted=true;}}
    } arrows[2]{};
    static int spring(unsigned age){
        // Q12 samples of Godot 4.5 Spring::out at 1/32 intervals. The
        // public equation is used only for this 150ms, 1.75 -> 2 scale.
        static constexpr uint16_t samples[]={0,439,841,1211,1553,1870,2168,2450,2717,2972,3214,3444,3659,3857,4034,4185,4305,4390,4434,4437,4398,4325,4228,4126,4040,3990,3987,4024,4078,4120,4132,4119,4096};
        if(age>=150)return 4096;const unsigned t=age*32,index=t/150,remainder=t%150;
        return (int(samples[index])*(150-int(remainder))+int(samples[index+1])*int(remainder)+75)/150;
    }
    unsigned opacity(uint32_t now)const{const unsigned age=now-opened;return age>=250?255:255*health_bar::expo_out(age*4)/4096;}
    int card_scale(uint32_t now)const{const unsigned age=now-opened;return 7168+(age<=100?0:spring(age-100)/4);}
    int card_x(uint32_t now)const{const unsigned age=now-slid;return card_inspect_layout::CenterX+(!direction||age>=250?0:direction*card_inspect_layout::Travel*int(4096-health_bar::expo_out(age*4))/4096);}
    bool closing(uint32_t now)const{return closed_token>=0&&now-closed<250;}
    void close(int token,uint32_t now){
        if(token<0)return;
        const unsigned age=now-opened,arrow_age=age>100?age-100:0;
        closed_token=token;closed=now;closed_scale=uint16_t(card_scale(now));closed_x=int16_t(card_x(now));
        closed_root=uint8_t(opacity(now));closed_backstop=uint8_t(age>=250?255:age*255/250);
        closed_face=uint8_t(direction?255:closed_backstop);
        closed_arrow=uint8_t(arrow_age>=250?255:arrow_age*255/250);
    }
    unsigned close_alpha(uint32_t now,unsigned duration,unsigned initial)const{
        const unsigned age=now-closed;return age>=duration?0:initial*(duration-age)/duration;
    }
};

}
