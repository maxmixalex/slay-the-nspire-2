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
constexpr int ButtonY=164,ButtonW=98,ButtonH=30;
constexpr int ImageY=160,ImageW=108,ImageH=40;
constexpr int LabelY=161,LabelW=88,LabelH=32;
constexpr int button_x(bool yes){return yes?178:44;}
constexpr int image_x(bool yes){return button_x(yes)-5;}
constexpr int label_x(bool yes){return button_x(yes)+5;}
constexpr int pivot_x(bool yes){return button_x(yes)+ButtonW/2;}
constexpr int PivotY=179;
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
