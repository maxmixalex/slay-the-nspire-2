#pragma once
#include <stdint.h>
namespace spire {
namespace popup_layout {
// GenericPopup is centered at1680x1260. Its VerticalPopup instance overrides
// the base scene with(-51,-59)..(522,600), so it is573x659, not522x600.
constexpr int project(int value){return (value*4+10)/21;}
constexpr int X=project(528),Y=project(271),W=project(573),H=project(659);
constexpr int HeaderY=project(327),BodyX=project(578),BodyY=project(386);
constexpr int BodyW=project(473),BodyH=project(389);
constexpr int ButtonY=project(778),ButtonW=project(180),ButtonH=project(72);
constexpr int ImageY=project(770),ImageW=project(196),ImageH=project(96);
constexpr int LabelY=project(767),LabelW=project(150),LabelH=project(96);
constexpr int button_x(bool yes){return project(yes?921:528);}
constexpr int image_x(bool yes){return project(yes?904:528);}
constexpr int label_x(bool yes){return project(yes?935:543);}
constexpr int pivot_x(bool yes){return project(yes?1095:533);}
constexpr int PivotY=project(820);
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
