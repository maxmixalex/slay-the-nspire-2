#include "app.hpp"
#include "status.hpp"
namespace spire {
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
void App::add_hit(int x,int y,int w,int h,HitKind kind,int value,bool enabled){
    if(w<=0||h<=0||x>=ScreenW||y>=ScreenH||x+w<=0||y+h<=0||hit_count>=MaxHits)return;
    int right=clamp(x+w,0,ScreenW),bottom=clamp(y+h,0,ScreenH);
    x=clamp(x,0,ScreenW);y=clamp(y,0,ScreenH);
    hits[hit_count++]={(int16_t)x,(int16_t)y,(int16_t)(right-x),(int16_t)(bottom-y),kind,(int16_t)value,enabled};
}
const HitRegion* App::hit_at(int x,int y)const{
    for(int i=hit_count-1;i>=0;--i){const auto&h=hits[i];
        if(x>=h.x&&x<h.x+h.w&&y>=h.y&&y<h.y+h.h)return &h;}
    return nullptr;
}
uint32_t App::input_context()const{
    return (uint32_t)game.phase|((uint32_t)shop_open<<6)|((uint32_t)event_reader<<7)|((uint32_t)title<<8)|((uint32_t)paused<<9)|
        ((uint32_t)help<<10)|((uint32_t)panel<<11)|((uint32_t)(detail_card>=0)<<16)|
        ((uint32_t)pile_tab<<17)|((uint32_t)map_preview.open<<24)|((uint32_t)map_preview.editing<<25)|((uint32_t)library<<26)|((uint32_t)character_select<<27)|((uint32_t)reward_cards<<28)|((uint32_t)(detail_item>=0)<<29)|((uint32_t)(deck_candidate>=0)<<30)|((uint32_t)(status_actor>=0)<<31);
}
void App::pointer_cancel(){pointer.held=pointer.dragging=false;pointer.armed_card=-1;pointer.captured={};}
void App::pointer_key(Key k){pointer.dispatching=true;input(k);pointer.dispatching=false;pointer.visible=true;}
void App::open_card_detail(Card card){
    if(card.kind>=CardKinds)return;
    detail_card=card.kind+256*card.upgraded;detail_scroll=0;pointer_cancel();
}
void App::pointer_move(int x,int y){
    pointer.x=clamp(x,0,319);pointer.y=clamp(y,0,239);pointer.visible=true;
    if(pointer.held){int dx=pointer.x-pointer.down_x,dy=pointer.y-pointer.down_y;
        if(dx*dx+dy*dy>=25)pointer.dragging=true;}
    const auto*h=hit_at(pointer.x,pointer.y);if(!h||(!h->enabled&&h->kind!=HitKind::Hand&&h->kind!=HitKind::Choice))return;
    switch(h->kind){
    case HitKind::UiAction:
        if(!pointer.held&&game.phase==Phase::Reward&&!reward_cards){
            if((UiAction)h->value==UiAction::RewardGold)cursor=0;
            else if((UiAction)h->value==UiAction::RewardCards)cursor=1;
            else if((UiAction)h->value==UiAction::RewardPotion)cursor=2;
            else if((UiAction)h->value==UiAction::LeaveRewards)cursor=3;
        }break;
    case HitKind::Menu:if(!pointer.held)menu=h->value;break;
    case HitKind::Choice:if(!pointer.held)cursor=h->value;break;
    case HitKind::LibraryItem:if(!pointer.held)library_selected=h->value;break;
    case HitKind::Relic:if(!pointer.held){scroll=h->value;detail_scroll=0;}break;
    case HitKind::Hand:
        if(!pointer.held)for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==h->value){cursor=i;break;}
        break;
    case HitKind::CombatChoice:
        if(!pointer.held){const auto& source=choice_pile(game);
            for(int i=0;i<source.count;i++)if(source.ids[i]==h->value){cursor=i;break;}}
        break;
    case HitKind::StatusItem:if(!pointer.held)status_selected=h->value;break;
    case HitKind::Enemy:target=h->value;break;
    case HitKind::ReferenceNode:if(!pointer.held)map_preview.selected=h->value;break;
    default:break;
    }
}
void App::pointer_scroll(int rows){
    if(!rows)return;
    // A release after scrolling must not activate the control now underneath.
    pointer_cancel();
    if(event_reader){event_reader_scroll=clamp(event_reader_scroll+rows,0,2048);return;}
    if(status_actor>=0){
        int count=collect_statuses(game,status_actor,nullptr);
        status_selected=clamp(status_selected+rows,0,count?count-1:0);return;
    }
    if(map_preview.open&&map_preview.editing)return;
    if(map_preview.open){map_preview.first_row=clamp(map_preview.first_row-rows,0,int(map_preview.map.config.rows)-3);return;}
    if(detail_item>=0){detail_scroll=clamp(detail_scroll+rows,0,100);return;}
    if(detail_card>=0){detail_scroll=clamp(detail_scroll+rows,0,100);return;}
    if(library){ui_action(rows>0?UiAction::NextPage:UiAction::PreviousPage);return;}
    if(title||paused||help||character_select||deck_candidate>=0)return;
    if(!panel&&choice_phase(game.phase)){
        ui_action(rows>0?UiAction::NextChoicePage:UiAction::PreviousChoicePage);return;
    }
    if(panel==1){
        // The renderer aligns to ten-card pages. Half-page increments were
        // rounded back to the same page on every wheel event.
        scroll=clamp(scroll+(rows>0?10:-10),0,MaxCards);inspect_cursor=scroll;
    }
    else if(panel==2)scroll=clamp(scroll-rows,0,24);
    else if(panel==3||game.phase==Phase::Map)map_scroll=clamp(map_scroll-rows,-MapRows,MapRows);
    else if(panel==5)scroll=clamp(scroll+rows,0,game.relic_count?game.relic_count-1:0);
    else if(game.phase==Phase::Event)scroll=clamp(scroll+rows,0,100);
    else if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform)
        ui_action(rows>0?UiAction::NextDeckPage:UiAction::PreviousDeckPage);
}
void App::pointer_button(bool down){
    // Input can arrive between timed frames. Hit geometry is rebuilt from the
    // current screen rather than trusting regions from a previous overlay.
    render();pointer_move(pointer.x,pointer.y);
    const auto*found=hit_at(pointer.x,pointer.y);HitRegion hit=found?*found:HitRegion{};
    if(down){
        if(pointer.held)return;
        pointer.held=true;pointer.dragging=false;pointer.down_x=pointer.x;pointer.down_y=pointer.y;
        pointer.captured=hit;pointer.context=input_context();pointer.revision=game.revision;
        return;
    }
    if(!pointer.held)return;
    pointer.held=false;
    HitRegion captured=pointer.captured;pointer.captured={};
    if(pointer.context!=input_context()||pointer.revision!=game.revision){pointer_cancel();return;}
    bool same=captured.kind==hit.kind&&captured.value==hit.value;
    if(captured.kind==HitKind::Hand&&game.phase==Phase::Combat&&captured.enabled){
        int id=captured.value,index=-1;
        for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==id)index=i;
        if(index<0||busy){pointer_cancel();return;}
        Card card=game.cards[id];bool targeted=definitions[card.kind].targeted;
        bool dropping=pointer.dragging&&(hit.kind==HitKind::Enemy||(!targeted&&hit.kind==HitKind::Battlefield));
        if(dropping&&hit.enabled){cursor=index;if(hit.kind==HitKind::Enemy)target=hit.value;
            pointer.armed_card=-1;pointer_key(Key::Confirm);return;}
        if(same&&!pointer.dragging){
            if(pointer.armed_card==id&&!targeted){cursor=index;pointer.armed_card=-1;pointer_key(Key::Confirm);}
            else pointer.armed_card=pointer.armed_card==id?-1:id;
            return;
        }
        pointer.armed_card=-1;return;
    }
    if(!same||!hit.enabled){if(pointer.dragging)pointer.armed_card=-1;return;}
    if(hit.kind==HitKind::Enemy||hit.kind==HitKind::Battlefield){
        if(pointer.armed_card<0||busy||game.phase!=Phase::Combat)return;
        int index=-1;for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==pointer.armed_card)index=i;
        if(index<0){pointer.armed_card=-1;return;}
        bool targeted=definitions[game.cards[pointer.armed_card].kind].targeted;
        if(targeted&&hit.kind!=HitKind::Enemy)return;
        cursor=index;if(hit.kind==HitKind::Enemy)target=hit.value;
        pointer.armed_card=-1;pointer_key(Key::Confirm);return;
    }
    pointer.armed_card=-1;
    switch(hit.kind){
    case HitKind::Menu:menu=hit.value;pointer_key(Key::Confirm);break;
    case HitKind::Choice:cursor=hit.value;if(game.phase==Phase::Map)map_scroll=0;pointer_key(Key::Confirm);break;
    case HitKind::Hand:
        if(game.phase==Phase::ExhaustChoice)for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==hit.value){cursor=i;pointer_key(Key::Confirm);break;}
        break;
    case HitKind::Command:pointer_key((Key)hit.value);break;
    case HitKind::OpenPile:panel=1;pile_tab=hit.value;scroll=inspect_cursor=0;break;
    case HitKind::PileTab:pile_tab=hit.value;scroll=inspect_cursor=0;break;
    case HitKind::PileCard:open_card_detail({(uint8_t)(hit.value&255),(uint8_t)(hit.value>>8)});break;
    case HitKind::Relic:scroll=hit.value;open_owned_relic(hit.value);break;
    case HitKind::OwnedRelic:open_owned_relic(hit.value);break;
    case HitKind::Potion:scroll=hit.value;break;
    case HitKind::Scroll:pointer_scroll(hit.value);break;
    case HitKind::Close:pointer_key(Key::Back);break;
    case HitKind::DetailClose:detail_card=-1;break;
    case HitKind::ReferenceNode:map_preview.selected=hit.value;break;
    case HitKind::ReferenceAction:
        pointer_cancel();
        if(hit.value==0)map_preview.begin_edit();
        else if(hit.value==1)map_preview.change_act();
        else if(hit.value==2){map_preview.settings.increased_elites=!map_preview.settings.increased_elites;map_preview.generate();}
        break;
    case HitKind::SeedCharacter:reference_text(hit.value);break;
    case HitKind::UiAction:ui_action((UiAction)hit.value);break;
    case HitKind::LibraryItem:select_library_item(hit.value);break;
    case HitKind::CombatChoice:toggle_choice(hit.value);break;
    case HitKind::StatusActor:open_statuses(hit.value);break;
    case HitKind::StatusItem:status_selected=hit.value;break;
    default:break;
    }
}
} // namespace spire
