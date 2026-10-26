#pragma once
#include <stdint.h>
namespace spire {
namespace popup_layout {
// GenericPopup retains the source artwork and two-button semantics. Its LCD
// plaque is reflowed so destructive confirmations are readable at10/12px.
constexpr int project(int value){return (value*4+10)/21;}
constexpr int X=30,Y=32,W=260,H=176;
constexpr int HeaderY=45,BodyX=44,BodyY=80;
constexpr int BodyW=232,BodyH=76;
constexpr int ButtonY=166,ButtonW=80,ButtonH=24;
constexpr int ImageY=162,ImageW=88,ImageH=32;
constexpr int LabelY=165,LabelW=72,LabelH=26;
constexpr int button_x(bool yes){return yes?186:54;}
constexpr int image_x(bool yes){return button_x(yes)-4;}
constexpr int label_x(bool yes){return button_x(yes)+4;}
constexpr int pivot_x(bool yes){return button_x(yes)+ButtonW/2;}
constexpr int PivotY=178;
}
// NModalContainer uses independent300ms linear black-backstop entry/exit.
// The disappearing modal releases input immediately; only its backdrop fades.
struct PopupBackdrop {
    uint32_t started=0;uint16_t from=0;bool open=false;
    unsigned sample(uint32_t now)const{
        const unsigned age=now-started,q=age>=300?300:age;
        return unsigned(int(from)+(int(open?217:0)-int(from))*int(q)/300);
    }
    void observe(bool active,uint32_t now){if(active!=open){from=uint16_t(sample(now));open=active;started=now;}}
};
}
