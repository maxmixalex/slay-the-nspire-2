#pragma once
#include "core.hpp"
#include <stddef.h>

namespace spire {
struct App;
enum class Key;
enum class HoverTipKind:uint8_t { Keyword,Static,Power,Orb,Card,Enchantment,Affliction,Special,Relic,Potion,Description };
struct HoverTip {
    HoverTipKind kind=HoverTipKind::Keyword;
    const char* key="";
    Card card{};
    int16_t amount=0,actor=-1;
    bool smart=false,debuff=false,instanced=false;
    bool has_facts=false;
    EventCardFacts facts{};
};
constexpr unsigned MaxCardHoverTips=32;
inline bool hover_key_equal(const char*a,const char*b){
    if(!a||!b)return a==b;while(*a&&*a==*b){++a;++b;}return *a==*b;
}
struct HoverTipList {
    HoverTip items[MaxCardHoverTips]{};
    unsigned count=0;
    bool overflow=false;
    void erase(unsigned at){if(at>=count)return;for(unsigned i=at+1;i<count;++i)items[i-1]=items[i];--count;}
    void add(const HoverTip&tip){
        for(unsigned i=0;i<count;++i){const auto&old=items[i];
            if(old.kind!=tip.kind||!hover_key_equal(old.key,tip.key)||old.instanced||
               (tip.kind==HoverTipKind::Card&&old.card.upgraded!=tip.card.upgraded))continue;
            // CardModel.Distinct retains cached keyword objects first. For
            // other identities NHoverTipSet prefers smart, then later tips.
            if(tip.kind==HoverTipKind::Keyword||(old.smart&&!tip.smart))return;
            erase(i);break;
        }
        if(count==MaxCardHoverTips){overflow=true;return;}items[count++]=tip;
    }
    int find(HoverTipKind kind,const char*key)const{
        for(unsigned i=0;i<count;++i)if(items[i].kind==kind&&hover_key_equal(items[i].key,key))return int(i);return -1;
    }
    void finish(){
        if(find(HoverTipKind::Static,"SUMMON_STATIC")>=0){int at;
            while((at=find(HoverTipKind::Static,"SUMMON_DYNAMIC"))>=0)erase(unsigned(at));}
    }
};
struct HoverRect {int x=0,y=0,w=0,h=0;};
struct CardHoverTarget {
    Card card{};
    int instance=-1,deck=-1;
    const EventCardFacts* facts=nullptr;
    EventCardFacts preview_facts{};
    bool has_preview_facts=false;
    const EventCardFacts* card_facts()const{return has_preview_facts?&preview_facts:facts;}
    HoverRect owner{};
    uint32_t identity=0;
    bool detail=false;
    int event_option=-1;
    int relic=-1;
    int potion=0;
    int power=-1;
    bool power_left=false;
    // History stores relic IDs, not their acquisition-time mutable models.
    bool historical=false;
    bool inspect_relic=false,canonical_relic=false;
};
struct HoverPiece {unsigned tip=0;int skip=0,lines=0;HoverRect box{};};
struct HoverPage {HoverPiece pieces[MaxCardHoverTips]{};unsigned count=0;int pages=1;};
// Presentation-only paging. It never appears in State or a save payload.
struct KeywordHoverState {
    uint32_t owner=0;
    int page=0;
    bool details=false;
};
inline HoverRect card_hover_column(HoverRect owner){
    constexpr int margin=3,gap=1,desired=69;
    const int left=owner.x-gap-margin,right=320-margin-(owner.x+owner.w+gap);
    bool use_left=owner.x>240;
    if(!use_left&&right<desired&&left>right)use_left=true;
    if(use_left&&left<desired&&right>left)use_left=false;
    const int width=desired;
    int x=use_left?owner.x-gap-width:owner.x+owner.w+gap;
    if(x<margin)x=margin;if(x+width>320-margin)x=320-margin-width;
    return {x,24,width,194};
}
void collect_card_hover_tips(const App&,Card,HoverTipList&,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr);
bool card_hover_target(const App&,CardHoverTarget&);
bool event_hover_target(const App&,CardHoverTarget&);
bool power_hover_target(const App&,CardHoverTarget&);
void collect_event_hover_tips(const App&,int option,HoverTipList&);
void collect_relic_hover_tips(const App&,RelicId,HoverTipList&,bool historical=false);
void collect_potion_hover_tips(const App&,Potion,HoverTipList&);
void collect_power_hover_tips(const App&,uint16_t token,HoverTipList&);
void format_card_hover_tip(const App&,const CardHoverTarget&,const HoverTip&,char*title,size_t title_cap,char*body,size_t body_cap);
HoverPage card_hover_page(App&,const CardHoverTarget&,const HoverTipList&,int page=0);
bool keyword_hover_scroll(App&,int rows);
bool keyword_detail_input(App&,Key);
}
