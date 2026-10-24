#pragma once
#include "combat_layout.hpp"
#include "card_reward_presentation.hpp"
#include "room_ui7_presentation.hpp"
namespace spire {
// CardSelectCmd owns three different views. These are presentation choices,
// independent of the saved pending command and its instance/model tokens.
enum class CombatSelectionKind:uint8_t {None,Hand,HandUpgrade,Pile,Offer,SimpleGrid};
inline bool combat_selection_hand(CombatSelectionKind kind){return kind==CombatSelectionKind::Hand||kind==CombatSelectionKind::HandUpgrade;}
struct CombatSelectionPrompt {const char*table;const char*key;};
struct NativeConfirmPresentation {
    uint32_t key=0,start=0;int16_t from=34;bool enabled=false,bound=false;
    static uint32_t identity(uint32_t opened,uint32_t seed,unsigned action,unsigned phase){return opened^seed^action*0x9e3779b9u^phase*0x85ebca6bu;}
    int sample(uint32_t now)const{
        const unsigned age=now-start;const int target=enabled?0:34;if(age>=350)return target;
        const int q=enabled?room_curve::back_out(age*4096/350):int(room_curve::expo_out(age,350));
        return from+(target-from)*q/4096;
    }
    int observe(uint32_t identity,bool active,uint32_t opened,uint32_t now){
        if(!bound||key!=identity){bound=true;key=identity;enabled=active;from=int16_t(active&&opened==UINT32_MAX?0:34);start=opened==UINT32_MAX?now:opened;}
        else if(enabled!=active){from=int16_t(sample(now));enabled=active;start=now;}
        return sample(now);
    }
};
struct CombatSelectionPresentation {
    CardRewardPresentation row{};
    CombatSelectionKind kind=CombatSelectionKind::None;
    struct Holder {HandPose current{},from{},target{};uint32_t at=0,last=0;int16_t id=-1;bool selected=false;};
    Holder holders[MaxHand]{};
    uint32_t selected_at=0;
    int16_t selected_count=0,from_scale=1000,from_y=110;
    bool selected_moved=false;
    void reset(){*this=CombatSelectionPresentation{};}
    static bool same(HandPose a,HandPose b){return a.cx==b.cx&&a.cy==b.cy&&a.scale==b.scale&&a.sine==b.sine&&a.cosine==b.cosine;}
    static int mix(int a,int b,unsigned q){return a+int(int64_t(b-a)*q/4096);}
    static HandPose mix(HandPose a,HandPose b,unsigned q){return {mix(a.cx,b.cx,q),mix(a.cy,b.cy,q),mix(a.scale,b.scale,q),mix(a.sine,b.sine,q),mix(a.cosine,b.cosine,q)};}
    int selected_scale(uint32_t now)const{return mix(from_scale,selected_count>6?550:selected_count>3?800:1000,room_curve::sine_in_out(now-selected_at,500));}
    int selected_y(uint32_t now)const{return !selected_moved?from_y:mix(from_y,120-(selected_count>6?hand_project(150):selected_count>3?hand_project(75):0),room_curve::sine_in_out(now-selected_at,500));}
    void select_count(unsigned count,uint32_t now){if(selected_count==int(count))return;from_scale=int16_t(selected_scale(now));from_y=int16_t(selected_y(now));selected_count=int16_t(count);selected_at=now;selected_moved=true;}
    bool observe(CombatSelectionKind next,uint32_t key,unsigned count,uint32_t now){
        if(next==CombatSelectionKind::None){row.active=false;kind=next;return false;}
        if(kind==next&&row.active&&row.key==key&&row.count==count)return false;
        reset();kind=next;row.observe(true,key,count,now);selected_at=now;return true;
    }
    int offer_x(unsigned index,uint32_t now)const{
        const int twice=int(index)*2-int(row.count)+1;
        const int offset=CardRewardPresentation::rounded(twice*680,21);
        return 160+CardRewardPresentation::rounded(offset*int(CardRewardPresentation::expo(now-row.start,500)),4096);
    }
    // NChooseACardSelectionScreen deliberately uses >350, not >=350.
    bool offer_clickable(uint32_t now)const{return row.active&&uint32_t(now-row.opened)>350;}
    HandPose hand_pose_for(int id,HandPose target,bool selected,uint32_t now){
        Holder*holder=nullptr;for(auto&h:holders)if(h.id==id){holder=&h;break;}
        if(!holder)for(auto&h:holders)if(h.id<0){holder=&h;h.id=int16_t(id);h.current=h.from=h.target=target;h.at=h.last=now;h.selected=selected;break;}
        if(!holder)return target;auto&h=*holder;
        if(h.selected!=selected){h.from=h.current;h.at=now;h.selected=selected;}
        h.target=target;
        if(selected){h.current=mix(h.from,target,CardRewardPresentation::cubic(now-h.at,150));}
        else{
            const unsigned elapsed=now-h.last,position=elapsed>=143?4096:elapsed*7*4096/1000;
            const unsigned scale=elapsed>=125?4096:elapsed*8*4096/1000,angle=elapsed>=100?4096:elapsed*10*4096/1000;
            h.current.cx=mix(h.current.cx,target.cx,position);h.current.cy=mix(h.current.cy,target.cy,position);
            h.current.scale=mix(h.current.scale,target.scale,scale);h.current.sine=mix(h.current.sine,target.sine,angle);h.current.cosine=mix(h.current.cosine,target.cosine,angle);
            if(h.current.cx-target.cx<2&&h.current.cx-target.cx> -2)h.current.cx=target.cx;
            if(h.current.cy-target.cy<2&&h.current.cy-target.cy> -2)h.current.cy=target.cy;
        }
        h.last=now;return h.current;
    }
};
}
