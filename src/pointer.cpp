#include "app.hpp"
#include "status.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "ui_core_observer.hpp"
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
        ((uint32_t)pile_tab<<17)|((uint32_t)grimoire_select<<20)|((uint32_t)map_legend<<21)|((uint32_t)(frontend_confirm!=0)<<22)|((uint32_t)settings_open<<23)|((uint32_t)map_preview.open<<24)|((uint32_t)map_preview.editing<<25)|((uint32_t)library<<26)|((uint32_t)character_select<<27)|((uint32_t)reward_cards<<28)|((uint32_t)(detail_item>=0)<<29)|((uint32_t)(deck_candidate>=0)<<30)|((uint32_t)(status_actor>=0)<<31);
}
void App::pointer_cancel(){hand_focus=false;pointer.held=pointer.dragging=false;pointer.armed_card=-1;pointer.captured={};}
void App::pointer_key(Key k){pointer.dispatching=true;input(k);pointer.dispatching=false;pointer.visible=true;}
void App::open_card_detail(Card card,int deck_index){
    if(card.kind>=CardKinds)return;
    detail_card=card_token(card);detail_scroll=0;detail_instance=-1;detail_cost_override=-2;detail_has_facts=false;detail_genetic=detail_scythe=0;
    detail_deck_index=deck_index>=0&&deck_index<game.deck_count&&game.deck[deck_index].kind==card.kind?deck_index:-1;pointer_cancel();
}
void App::open_card_offer(Card card,const EventCardFacts&facts,uint16_t genetic,uint16_t scythe){open_card_detail(card);detail_facts=facts;detail_has_facts=true;detail_genetic=genetic;detail_scythe=scythe;}
void App::open_choice_detail(int token){
    if(token>=0&&token<game.card_count){
        open_card_detail(game.cards[token]);detail_instance=token;
        detail_cost_override=cost(game,token);
        if(choice_free_energy(game)&&cost(game.cards[token])>=0){
            for(unsigned i=0;i<game.expansion.choices.count;i++)if(game.expansion.choices.ids[i]==token)detail_cost_override=card_free_preview_cost(game,game.cards[token],token);
        }
        return;
    }
    if(game.phase!=Phase::ExtendedChoice||(game.expansion.choice!=XChoice::Offer&&game.expansion.choice!=XChoice::EnemyKnowledge&&game.expansion.choice!=XChoice::EventAbundance)||token<MaxCards||token>=MaxCards+3)return;
    bool offered=false;for(unsigned i=0;i<game.expansion.choices.count;i++)if(game.expansion.choices.ids[i]==token)offered=true;
    if(!offered)return;
    open_card_detail(choice_card(game,token));
    if(choice_free_energy(game)&&cost(choice_card(game,token))>=0)detail_cost_override=card_free_preview_cost(game,choice_card(game,token));
}
void App::pointer_move(int x,int y){
    // The platform pointer API has no Godot motion velocity. An actual
    // coordinate change is the bounded adapter for its mouse-mode switch;
    // repeated stationary samples do not cancel directional focus.
    if(clamp(x,0,319)!=pointer.x||clamp(y,0,239)!=pointer.y){map_directional=false;sync_map_reticles();}
    pointer.x=clamp(x,0,319);pointer.y=clamp(y,0,239);pointer.visible=true;
    if(pointer.held){int dx=pointer.x-pointer.down_x,dy=pointer.y-pointer.down_y;
        if(dx*dx+dy*dy>=25)pointer.dragging=true;}
    const auto*h=hit_at(pointer.x,pointer.y);
    if(!pointer.held)hand_focus=hand_can_focus()&&h&&h->kind==HitKind::Hand;
    if(!h||(!h->enabled&&h->kind!=HitKind::Hand&&h->kind!=HitKind::Choice))return;
    switch(h->kind){
    case HitKind::UiAction:
        if(!pointer.held&&game.phase==Phase::Reward&&!reward_cards&&!grimoire_select){
            UiAction actions[6];unsigned count=ui_reward_actions(game,actions);for(unsigned i=0;i<count;i++)if(actions[i]==(UiAction)h->value)cursor=i;
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
    case HitKind::WorldChoice:
        if(!pointer.held){uint16_t ids[MaxWorldUiChoices];unsigned count=world_choice_options(ids,MaxWorldUiChoices);for(unsigned i=0;i<count&&i<MaxWorldUiChoices;i++)if(ids[i]==h->value){cursor=i;break;}}break;
    case HitKind::CrystalCell:if(!pointer.held)cursor=h->value;break;
    case HitKind::StatusItem:if(!pointer.held&&status_selected!=h->value){status_selected=h->value;status_scroll=0;}break;
    case HitKind::Enemy:case HitKind::EnemyIntent:target=h->value;break;
    case HitKind::ReferenceNode:if(!pointer.held)map_preview.selected=h->value;break;
    default:break;
    }
}
void App::pointer_scroll(int rows){
    if(!rows||map_transition_blocks_pointer())return;
    map_directional=false;sync_map_reticles();
    // A release after scrolling must not activate the control now underneath.
    pointer_cancel();
    if(event_reader){event_reader_scroll=clamp(event_reader_scroll+rows,0,2048);return;}
    if(status_actor>=0){
        int count=collect_statuses(game,status_actor,nullptr);
        status_selected=clamp(status_selected+rows,0,count?count-1:0);status_scroll=0;return;
    }
    if(map_preview.open&&map_preview.editing)return;
    if(map_preview.open){map_preview.first_row=clamp(map_preview.first_row-rows,0,int(map_preview.map.config.rows)-3);return;}
    if(detail_item>=0){detail_scroll=clamp(detail_scroll+rows,0,100);return;}
    if(detail_card>=0){detail_scroll=clamp(detail_scroll+rows,0,100);return;}
    if(library){ui_action(rows>0?UiAction::NextPage:UiAction::PreviousPage);return;}
    if(title||paused||help||character_select||deck_candidate>=0)return;
    if(map_legend&&game.world.active&&(panel==3||(!panel&&game.phase==Phase::Map))){map_legend_item=uint8_t(clamp(int(map_legend_item)+rows,0,5));return;}
    if(!panel&&(game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice)){ui_action(rows>0?UiAction::NextWorldPage:UiAction::PreviousWorldPage);return;}
    if(!panel&&choice_phase(game.phase)){
        ui_action(rows>0?UiAction::NextChoicePage:UiAction::PreviousChoicePage);return;
    }
    if(panel==1){
        // The renderer aligns to ten-card pages. Half-page increments were
        // rounded back to the same page on every wheel event.
        scroll=clamp(scroll+(rows>0?10:-10),0,MaxCards);inspect_cursor=scroll;
    }
    else if(panel==4)scroll=clamp(scroll+(rows>0?8:-8),0,int(potion_slot_count(game))-1);
    else if(panel==2)scroll=clamp(scroll-rows,0,24);
    else if(panel==3||game.phase==Phase::Map)map_scroll=clamp(map_scroll-rows,-reference::MaxRows,reference::MaxRows);
    else if(panel==5)scroll=clamp(scroll+rows,0,ui_owned_relic_count(game)?int(ui_owned_relic_count(game))-1:0);
    else if(game.phase==Phase::Event)scroll=clamp(scroll+rows,0,100);
    else if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform||grimoire_select)
        ui_action(rows>0?UiAction::NextDeckPage:UiAction::PreviousDeckPage);
}
void App::pointer_button(bool down){
    UiCoreObserver observer(*this);
    struct ReticleSync {App&a;~ReticleSync(){a.sync_map_window();a.sync_map_reticles();}} reticle_sync{*this};
    map_directional=false;sync_map_reticles();
    if(map_transition_blocks_pointer()){pointer_cancel();return;}
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
        Card card=game.cards[id];bool targeted=card_targets_enemy(game,card);
        bool dropping=pointer.dragging&&(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent||(!targeted&&hit.kind==HitKind::Battlefield));
        if(dropping&&hit.enabled){cursor=index;if(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent)target=hit.value;
            pointer.armed_card=-1;pointer_key(Key::Confirm);return;}
        if(same&&!pointer.dragging){
            if(pointer.armed_card==id&&!targeted){cursor=index;pointer.armed_card=-1;pointer_key(Key::Confirm);}
            else pointer.armed_card=pointer.armed_card==id?-1:id;
            return;
        }
        pointer.armed_card=-1;return;
    }
    if(!same||!hit.enabled){if(pointer.dragging)pointer.armed_card=-1;return;}
    if(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent||hit.kind==HitKind::Battlefield){
        if(pointer.armed_card<0||busy||game.phase!=Phase::Combat)return;
        int index=-1;for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==pointer.armed_card)index=i;
        if(index<0){pointer.armed_card=-1;return;}
        bool targeted=card_targets_enemy(game,game.cards[pointer.armed_card]);
        if(targeted&&hit.kind!=HitKind::Enemy&&hit.kind!=HitKind::EnemyIntent)return;
        cursor=index;if(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent)target=hit.value;
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
    case HitKind::InstanceCard:open_choice_detail(hit.value);break;
    case HitKind::PileCard:open_card_detail(card_from_token(hit.value));break;
    case HitKind::DeckCard:if(hit.value>=0&&hit.value<game.deck_count)open_card_detail(game.deck[hit.value],hit.value);break;
    case HitKind::Relic:scroll=hit.value;open_owned_relic(hit.value);break;
    case HitKind::OwnedRelic:open_owned_relic(hit.value);break;
    case HitKind::Potion:scroll=hit.value;break;
    case HitKind::Scroll:pointer_scroll(hit.value);break;
    case HitKind::Close:pointer_key(Key::Back);break;
    case HitKind::DetailClose:detail_card=-1;break;
    case HitKind::ReferenceNode:map_preview.selected=hit.value;break;
    case HitKind::WorldNode:
        if(!map_legend&&map_routes_active())begin_map_travel(unsigned(hit.value));break;
    case HitKind::EnemySelect:if(hit.value>=0&&hit.value<game.enemy_count&&game.enemies[hit.value].hp>0)target=hit.value;break;
    case HitKind::WorldChoice:toggle_world_choice(hit.value);break;
    case HitKind::CrystalCell:
        if(game.phase==Phase::OriginalEventChoice&&hit.value>=0&&hit.value<CrystalCells){cursor=hit.value;original_event_crystal_click(game,hit.value%11,hit.value/11);}break;
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
    case HitKind::StatusItem:status_selected=hit.value;status_scroll=0;break;
    default:break;
    }
}
} // namespace spire
