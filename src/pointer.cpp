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
        if(x<h.x||x>=h.x+h.w||y<h.y||y>=h.y+h.h)continue;
        if(h.kind==HitKind::Hand){
            const auto hand=combat_hand();bool inside=false;
            for(unsigned slot=0;slot<hand.count;++slot)if(hand.ids[slot]==h.value){
                inside=hand_contains(combat_hand_pose(slot,hand_focus&&int(slot)==cursor),x,y);break;}
            if(!inside)continue;
        }
        if((h.kind==HitKind::CombatChoice||h.kind==HitKind::WorldChoice)&&combat_selection_hand(combat_selection.kind)&&
           !(combat_selection.kind==CombatSelectionKind::HandUpgrade&&choice_selected_count&&choice_selected[0]==h.value)){
            bool inside=false;for(const auto&holder:combat_selection.holders)if(holder.id==h.value){inside=hand_contains(holder.current,x,y);break;}
            if(!inside)continue;
        }
        return &h;
    }
    return nullptr;
}
uint32_t App::input_context()const{
    if(ftue.active())return 0x46540000u^(ftue.serial*256u)^unsigned(ftue.id);
    if(game_over_available())return 0x474f0000u^uint32_t(game.world.run_seed)^uint32_t(game.world.run_seed>>32)^unsigned(game_over_summary);
    if(auxiliary.screen!=AuxiliaryScreen::None)return 0x41555800u|unsigned(auxiliary.screen)|(uint32_t(auxiliary.category_open)<<4)|(uint32_t(auxiliary.closing)<<5);
    if(settings_open&&binding_listening>=0)return 0x4b455900u|unsigned(binding_listening);
    if(bestiary.open)return 0x42535400u|unsigned(bestiary.actions);
    if(run_mode.screen!=RunMenu::Closed)return 0x52554e00u|unsigned(run_mode.screen)|(uint32_t(map_preview.open)<<4)|(uint32_t(frontend_confirm!=0)<<5);
    return (uint32_t)game.phase|((uint32_t)shop_open<<6)|((uint32_t)event_reader<<7)|((uint32_t)title<<8)|((uint32_t)paused<<9)|
        ((uint32_t)help<<10)|((uint32_t)panel<<11)|((uint32_t)potion_targeting<<15)|((uint32_t)(detail_card>=0)<<16)|
        ((uint32_t)pile_tab<<17)|((uint32_t)grimoire_select<<20)|((uint32_t)map_legend<<21)|((uint32_t)(frontend_confirm!=0)<<22)|((uint32_t)settings_open<<23)|((uint32_t)map_preview.open<<24)|((uint32_t)map_preview.editing<<25)|((uint32_t)library<<26)|((uint32_t)character_select<<27)|((uint32_t)reward_cards<<28)|((uint32_t)(detail_item>=0)<<29)|((uint32_t)(deck_candidate>=0||(deck_selection.preview&&world_selection_kind()!=WorldSelectionKind::None))<<30)|((uint32_t)(status_actor>=0)<<31);
}
void App::pointer_cancel(bool keep_keyboard_card){reward_presentation.list_scroll.dragging=reward_presentation.list_scroll.thumb=false;map_drawing.cancel();auxiliary.scroll.dragging=auxiliary.scroll.thumb=false;progress_page_scroll.dragging=progress_page_scroll.thumb=false;hand_focus=false;map_dragging=false;card_grid_drag=-1;bestiary.dragging=bestiary.thumb_drag=false;for(auto&grid:card_grids)grid.dragging=grid.thumb_drag=false;pointer.held=pointer.dragging=false;pointer.armed_card=-1;pointer.captured={};if(!keep_keyboard_card)keyboard_card=-1;}
void App::pointer_key(Key k){pointer.dispatching=true;input(k);pointer.dispatching=false;pointer.visible=true;}
#include "card_inspect_input.inc"
void App::open_card_detail(Card card,int deck_index){
    if(card.kind>=CardKinds)return;
    keyword_hover={};
    detail_card=card_token(card);detail_scroll=0;detail_instance=-1;detail_cost_override=-2;detail_has_facts=false;detail_genetic=detail_scythe=0;
    detail_deck_index=deck_index>=0&&deck_index<game.deck_count&&game.deck[deck_index].kind==card.kind?deck_index:-1;pointer_cancel();bind_card_inspect();
}
void App::open_card_offer(Card card,const EventCardFacts&facts,uint16_t genetic,uint16_t scythe){open_card_detail(card);detail_facts=facts;detail_has_facts=true;detail_genetic=genetic;detail_scythe=scythe;bind_card_inspect();}
void App::open_choice_detail(int token){
    if(token>=0&&token<game.card_count){
        open_card_detail(game.cards[token]);detail_instance=token;
        detail_cost_override=cost(game,token);
        if(choice_free_energy(game)&&cost(game.cards[token])>=0){
            for(unsigned i=0;i<game.expansion.choices.count;i++)if(game.expansion.choices.ids[i]==token)detail_cost_override=card_free_preview_cost(game,game.cards[token],token);
        }
        bind_card_inspect();return;
    }
    if(game.phase!=Phase::ExtendedChoice||(game.expansion.choice!=XChoice::Offer&&game.expansion.choice!=XChoice::EnemyKnowledge&&game.expansion.choice!=XChoice::EventAbundance)||token<MaxCards||token>=MaxCards+3)return;
    bool offered=false;for(unsigned i=0;i<game.expansion.choices.count;i++)if(game.expansion.choices.ids[i]==token)offered=true;
    if(!offered)return;
    open_card_detail(choice_card(game,token));
    if(choice_free_energy(game)&&cost(choice_card(game,token))>=0)detail_cost_override=card_free_preview_cost(game,choice_card(game,token));
    bind_card_inspect();
}
void App::pointer_move(int x,int y){
#if SPIRE_DEBUG_COMMANDS
    if(debug_open){pointer.x=clamp(x,0,319);pointer.y=clamp(y,0,239);pointer.visible=true;return;}
#endif
    const int px=clamp(x,0,319),py=clamp(y,0,239),dx=px-pointer.x,dy=py-pointer.y;
    const unsigned distance=unsigned(dx*dx+dy*dy),elapsed=map_clock_ms-pointer.motion_time;
    pointer.x=px;pointer.y=py;
    if(distance){
        // NControllerManager accepts motion faster than 10 source px/s and
        // rejects relative warps beyond 500 source px. Hosts supply positions
        // rather than Godot velocity: derive it from the presentation clock,
        // using the UI projection 2/9 and no floating-point/backend rounding.
        const unsigned dt=elapsed?elapsed:1;
        const bool mouse=pointer.visible||!keyboard_only||
            (distance*81u<=1000000u&&dt<50000u&&uint64_t(distance)*20250000u>uint64_t(dt)*dt*100u);
        pointer.motion_time=map_clock_ms;
        if(mouse){pointer.visible=true;hud_focus=-1;map_directional=false;keyboard_card=-1;if(auxiliary.screen==AuxiliaryScreen::None){sync_map_reticles();map_drawing_pointer_motion();}}
    }
    if(!pointer.visible)return;
    if(ftue.active())return;
    if(auxiliary.screen!=AuxiliaryScreen::None){
        if(pointer.held){const int xx=pointer.x-pointer.down_x,yy=pointer.y-pointer.down_y;if(xx*xx+yy*yy>=25)pointer.dragging=true;}
        auto&scroll=auxiliary.scroll;
        if(pointer.held&&scroll.dragging){
            if(pointer.context!=input_context()){pointer_cancel();return;}
            if(scroll.thumb)scroll.set_thumb(pointer.y,33,154);
            else scroll.target=UiHistoryScroll::clamp(scroll.target-(pointer.y-scroll.last_y)*256,-scroll.viewport*256,scroll.maximum+scroll.viewport*256);
            scroll.last_y=pointer.y;
        }return;
    }
    if(map_drawing.pressed)return;
    if(pointer.held){int dx=pointer.x-pointer.down_x,dy=pointer.y-pointer.down_y;
        if(dx*dx+dy*dy>=25)pointer.dragging=true;}
    if(pointer.held&&reward_presentation.list_scroll.dragging){
        auto&list=reward_presentation.list_scroll;
        if(pointer.context!=input_context()||!reward_list_interactive()){pointer_cancel();return;}
        const int dy=reward_presentation.window(map_clock_ms).y;
        if(list.thumb)list.set_thumb(pointer.y,89+dy,85);
        else if(pointer.dragging)list.target=UiHistoryScroll::clamp(list.target-(pointer.y-list.last_y)*256,-76*256,list.maximum+76*256);
        list.last_y=pointer.y;return;
    }
    if(pointer.held&&progress_page_scroll.dragging){
        if(progress_page_scroll.thumb)progress_page_scroll.set_thumb(pointer.y,run_mode.screen==RunMenu::Custom?42:statistics_open?UiStatisticsLayout::ScrollTop:61,run_mode.screen==RunMenu::Custom?158:statistics_open?UiStatisticsLayout::ScrollLength:128);
        else progress_page_scroll.target=UiHistoryScroll::clamp(progress_page_scroll.target-(pointer.y-progress_page_scroll.last_y)*256,-progress_page_scroll.viewport*256,progress_page_scroll.maximum+progress_page_scroll.viewport*256);
        progress_page_scroll.last_y=pointer.y;return;
    }
    if(pointer.held&&map_dragging){
        const int dy=pointer.y-map_drag_y;map_drag_y=pointer.y;
        if(dy){pointer.dragging=true;drag_map(dy);}return;
    }
    const auto*h=hit_at(pointer.x,pointer.y);
    if(bestiary.open){
        hand_focus=false;
        if(pointer.held&&(bestiary.dragging||bestiary.thumb_drag)){
            if(pointer.context!=input_context()){pointer_cancel();return;}
            if(bestiary.thumb_drag)bestiary.drag_thumb(pointer.y);
            else if(pointer.dragging)bestiary.target=BestiaryPresentation::clamp(bestiary.target-(pointer.y-bestiary.drag_y)*BestiaryPresentation::Unit,
                -(BestiaryPresentation::Bottom-BestiaryPresentation::Top)*BestiaryPresentation::Unit,
                bestiary.maximum()+(BestiaryPresentation::Bottom-BestiaryPresentation::Top)*BestiaryPresentation::Unit);
            bestiary.drag_y=int16_t(pointer.y);
        }return;
    }
    // Modal controls own pointer focus even before the next timed render has
    // replaced stale hit geometry from the covered pause/title page. Their
    // own hover rendering reads coordinates; no underlying selection changes.
    if(settings_open||frontend_confirm||help){hand_focus=false;return;}
    if(pointer.held&&card_grid_drag>=0){
        if(pointer.context!=input_context()||pointer.revision!=game.revision){pointer_cancel();return;}
        auto&grid=card_grids[card_grid_drag];
        if(grid.thumb_drag)grid.drag_thumb(pointer.y);
        else if(pointer.dragging)grid.drag_content(pointer.y-card_grid_drag_y);
        card_grid_drag_y=pointer.y;return;
    }
    if(!pointer.held)hand_focus=hand_can_focus()&&h&&h->kind==HitKind::Hand;
    if(!h||(!h->enabled&&h->kind!=HitKind::Hand&&h->kind!=HitKind::Choice))return;
    if(!combat_choice_ready()&&(h->kind==HitKind::Hand||h->kind==HitKind::Choice||
       h->kind==HitKind::CombatChoice||h->kind==HitKind::WorldChoice||h->kind==HitKind::CrystalCell))return;
    switch(h->kind){
    case HitKind::UiAction:
        // Hover changes the button feedback only. Selection belongs to the
        // captured press/release action, including cancel when dragged away.
        if(!pointer.held&&game.phase==Phase::Reward&&!reward_cards&&!grimoire_select){
            UiAction actions[6];unsigned count=ui_reward_actions(game,actions);for(unsigned i=0;i<count;i++)if(actions[i]==(UiAction)h->value)cursor=i;
        }break;
    case HitKind::Menu:if(!pointer.held)menu=h->value;break;
    case HitKind::Choice:if(!pointer.held)cursor=h->value;break;
    case HitKind::LibraryItem:if(!pointer.held){library_selected=h->value;card_library.control=-1;}break;
    case HitKind::LibraryControl:if(!pointer.held)card_library.control=h->value;break;
    case HitKind::Relic:if(!pointer.held){scroll=h->value;detail_scroll=0;}break;
    case HitKind::Hand:{
        const auto hand=combat_hand();
        if(!pointer.held)for(int i=0;i<hand.count;i++)if(hand.ids[i]==h->value){cursor=i;break;}
        break;}
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
    if(ftue.active())return;
    if(auxiliary.screen!=AuxiliaryScreen::None){pointer_cancel();auxiliary_scroll(rows);return;}
    if(map_drawing_ready()&&map_drawing.mode!=MapDrawingMode::None)return;
#if SPIRE_DEBUG_COMMANDS
    if(debug_open){if(rows)debug_input(rows<0?Key::Up:Key::Down);return;}
#endif
    if(bestiary.open){bestiary_scroll(rows);return;}
    if(!rows||map_transition_blocks_pointer()||frontend_confirm)return;
    if(settings_open){settings_action(rows>0?UiAction::SettingsScrollDown:UiAction::SettingsScrollUp);return;}
    if(const auto*hit=hit_at(pointer.x,pointer.y);hit&&hit->kind==HitKind::HistoryJournalScroll){pointer_cancel();history_tooltip.scroll(rows);return;}
    if(keyword_hover_scroll(*this,rows))return;
    if((statistics_open||history_open||run_mode.screen==RunMenu::Custom||run_mode.screen==RunMenu::Daily)&&detail_card<0&&detail_item<0){pointer_cancel();progress_page_scroll.wheel(rows);return;}
    sync_map_window();
    map_directional=false;sync_map_reticles();
    // A release after scrolling must not activate the control now underneath.
    pointer_cancel();
    if(event_reader){event_reader_scroll=clamp(event_reader_scroll+rows,0,2048);return;}
    if(scroll_ancient_dialogue(rows))return;
    if(status_actor>=0){
        int count=combat_statuses(status_actor,nullptr);
        status_selected=clamp(status_selected+rows,0,count?count-1:0);status_scroll=0;return;
    }
    if(map_preview.open&&map_preview.editing)return;
    if(map_preview.open){map_preview.first_row=clamp(map_preview.first_row-rows,0,int(map_preview.map.config.rows)-3);return;}
    if(detail_item>=0)return; // Inspector prose is a fitted VBox, not a legacy scroll panel.
    if(detail_card>=0){detail_scroll=clamp(detail_scroll+rows,0,100);return;}
    const int grid=active_card_grid();
    if(grid>=0&&card_grids[grid].bound){card_grids[grid].wheel(rows);return;}
    if(library)return;
    if(title||paused||help||character_select||deck_candidate>=0)return;
    if(reward_list_interactive()){reward_presentation.scroll_list(rows);return;}
    if(map_legend&&game.world.active&&(panel==3||(!panel&&game.phase==Phase::Map))){map_legend_item=uint8_t(clamp(int(map_legend_item)+rows,0,5));return;}
    if(!panel&&(choice_peeking||combat_selection_hand(combat_selection_kind())||combat_selection_kind()==CombatSelectionKind::Offer))return;
    if(!panel&&(game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice)){ui_action(rows>0?UiAction::NextWorldPage:UiAction::PreviousWorldPage);return;}
    if(!panel&&choice_phase(game.phase)){
        ui_action(rows>0?UiAction::NextChoicePage:UiAction::PreviousChoicePage);return;
    }
    if(panel==1){
        // The renderer aligns to ten-card pages. Half-page increments were
        // rounded back to the same page on every wheel event.
        scroll=clamp(scroll+(rows>0?10:-10),0,MaxCards);inspect_cursor=scroll;pile_control_focus=-1;
    }
    else if(panel==4){if(!potion_targeting)potion_input(rows>0?Key::Right:Key::Left);}
    else if(panel==2)scroll=clamp(scroll-rows,0,24);
    else if(panel==3||game.phase==Phase::Map)scroll_map(-rows);
    else if(game.phase==Phase::Event)scroll=clamp(scroll+rows,0,100);
    else if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform||grimoire_select)
        ui_action(rows>0?UiAction::NextDeckPage:UiAction::PreviousDeckPage);
}
void App::pointer_button(bool down){
    sync_ftues();
    if(ftue.active()){
        pointer.visible=true;pointer.motion_time=map_clock_ms;render();
        const auto*found=hit_at(pointer.x,pointer.y);const HitRegion hit=found?*found:HitRegion{};
        if(down){if(pointer.held)return;pointer.held=true;pointer.dragging=false;pointer.captured=hit;pointer.context=input_context();}
        else if(pointer.held){const auto captured=pointer.captured;const bool same=pointer.context==input_context()&&captured.kind==hit.kind&&captured.value==hit.value;
            pointer_cancel();if(same&&hit.enabled&&hit.kind==HitKind::FtueControl)ftue_control(hit.value);}
        return;
    }
    if(auxiliary.screen!=AuxiliaryScreen::None){
        pointer.visible=true;pointer.motion_time=map_clock_ms;render();
        if(down&&auxiliary.screen==AuxiliaryScreen::Credits){close_auxiliary();return;}
        const auto*found=hit_at(pointer.x,pointer.y);const HitRegion hit=found?*found:HitRegion{};
        if(down&&auxiliary.screen==AuxiliaryScreen::Feedback&&auxiliary.category_open&&
           !(hit.kind==HitKind::AuxiliaryControl&&(hit.value==AuxiliaryFeedbackCategory||
             (hit.value>=AuxiliaryFeedbackCategoryFirst&&hit.value<AuxiliaryFeedbackCategoryFirst+4)))){
            auxiliary.category_open=false;pointer_cancel();return;
        }
        if(down){
            if(pointer.held)return;
            pointer.held=true;pointer.dragging=false;pointer.down_x=pointer.x;pointer.down_y=pointer.y;
            pointer.captured=hit;pointer.context=input_context();
            if(auxiliary.screen==AuxiliaryScreen::PatchNotes&&hit.kind==HitKind::AuxiliaryControl&&hit.enabled&&
               (hit.value==AuxiliaryScrollArea||hit.value==AuxiliaryScrollThumb)){
                auto&scroll=auxiliary.scroll;scroll.dragging=true;scroll.thumb=hit.value==AuxiliaryScrollThumb;scroll.last_y=pointer.y;
                if(scroll.thumb)scroll.set_thumb(pointer.y,33,154);else scroll.target=scroll.position;
            }
        }else if(pointer.held){
            const auto captured=pointer.captured;const bool same=pointer.context==input_context()&&!pointer.dragging&&
                !auxiliary.scroll.thumb&&captured.kind==hit.kind&&captured.value==hit.value;
            pointer_cancel();
            if(same&&hit.enabled&&hit.kind==HitKind::AuxiliaryControl)auxiliary_control(hit.value);
        }return;
    }
#if SPIRE_DEBUG_COMMANDS
    if(debug_open){
        const auto*h=hit_at(pointer.x,pointer.y);
        if(down){pointer.held=true;pointer.captured=h?*h:HitRegion{};}
        else{const bool same=pointer.held&&h&&h->kind==pointer.captured.kind&&h->value==pointer.captured.value;pointer.held=false;
            if(same&&h->enabled){const HitRegion chosen=*h;
                if(chosen.kind==HitKind::DebugItem){debug_selection=chosen.value;debug_variant=0;debug_editing=false;debug_preset();debug_command(debug_line);}
                else if(chosen.kind==HitKind::Close)debug_input(Key::Back);
                else if(chosen.kind==HitKind::Command)debug_input(Key(chosen.value));}}
        return;
    }
#endif
    UiCoreObserver observer(*this);
    if(down)keyboard_card=-1;
    // Mouse buttons always restore mouse mode in the original, including
    // when KeyboardMode is enabled in preferences.
    pointer.visible=true;hud_focus=-1;pointer.motion_time=map_clock_ms;
    struct ReticleSync {App&a;~ReticleSync(){a.sync_map_window();a.sync_map_reticles();}} reticle_sync{*this};
    map_directional=false;sync_map_reticles();
    if(!bestiary.open&&map_transition_blocks_pointer()){pointer_cancel();return;}
    // Input can arrive between timed frames. Hit geometry is rebuilt from the
    // current screen rather than trusting regions from a previous overlay.
    render();pointer_move(pointer.x,pointer.y);
    const auto*found=hit_at(pointer.x,pointer.y);HitRegion hit=found?*found:HitRegion{};
    if(map_drawing_pointer_button(down,hit))return;
    if(progress_pointer_button(down,hit))return;
    if(down&&library&&!library_tab&&library_search_editing&&
       !(hit.kind==HitKind::LibraryControl&&(hit.value==LibrarySearch||hit.value==LibraryClearSearch)))
        library_search_editing=card_library.editing=false;
    if(down){
        if(pointer.held)return;
        if(bestiary.open){
            pointer.held=true;pointer.dragging=false;pointer.down_x=pointer.x;pointer.down_y=pointer.y;
            pointer.captured=hit;pointer.context=input_context();pointer.revision=game.revision;
            if(pointer.x>=BestiaryPresentation::Left-3&&pointer.y>=BestiaryPresentation::Top&&pointer.y<BestiaryPresentation::Bottom){
                bestiary.thumb_drag=hit.kind==HitKind::BestiaryScroll;bestiary.dragging=!bestiary.thumb_drag;
                bestiary.drag_y=int16_t(pointer.y);
                if(bestiary.thumb_drag)bestiary.drag_thumb(pointer.y);else bestiary.target=bestiary.position;
            }return;
        }
        const bool map_background=game.world.active&&map_window.is_open&&
            (panel==3||(!panel&&game.phase==Phase::Map))&&!map_legend&&!paused&&!help&&!library&&!settings_open&&!frontend_confirm&&
            detail_card<0&&detail_item<0&&status_actor<0&&!event_reader&&
            hit.kind==HitKind::None&&pointer.y>WorldMapTop&&pointer.y<WorldMapBottom&&pointer.x<297;
        if(map_background){
            if(!map_can_scroll())return;
            if(cancel_map_act_intro())return;
            map_dragging=true;map_drag_y=pointer.y;map_act_intro.target_q12=map_act_intro.y_q12;
        }
        pointer.held=true;pointer.dragging=false;pointer.down_x=pointer.x;pointer.down_y=pointer.y;
        pointer.captured=hit;pointer.context=input_context();pointer.revision=game.revision;
        if(reward_list_interactive()&&reward_presentation.list_scroll.maximum){
            auto&list=reward_presentation.list_scroll;const int dy=reward_presentation.window(map_clock_ms).y;
            if(hit.kind==HitKind::RewardScroll||(pointer.x>=114&&pointer.x<205&&pointer.y>=82+dy&&pointer.y<174+dy)){
                list.dragging=true;list.thumb=hit.kind==HitKind::RewardScroll;list.last_y=pointer.y;
                if(list.thumb)list.set_thumb(pointer.y,89+dy,85);else list.target=list.position;
            }
        }
        const int grid_index=active_card_grid();
        if(grid_index>=0){auto&grid=card_grids[grid_index];
            const bool card=hit.kind==HitKind::DeckCard||hit.kind==HitKind::InstanceCard||hit.kind==HitKind::LibraryItem||hit.kind==HitKind::CombatChoice||hit.kind==HitKind::WorldChoice||hit.kind==HitKind::Choice;
            if(grid.bound&&(!library||library_tab||pointer.x>=64)&&pointer.y>=grid.top&&pointer.y<grid.bottom&&(card||hit.kind==HitKind::None||hit.kind==HitKind::GridScroll)){
                card_grid_drag=grid_index;card_grid_drag_y=pointer.y;grid.thumb_drag=hit.kind==HitKind::GridScroll;grid.dragging=!grid.thumb_drag;
                if(grid.thumb_drag)grid.drag_thumb(pointer.y);else grid.target=grid.position;
            }
        }
        return;
    }
    if(!pointer.held)return;
    if(reward_presentation.list_scroll.dragging){auto&list=reward_presentation.list_scroll;const bool dragged=list.thumb||pointer.dragging;
        list.dragging=list.thumb=false;if(dragged){pointer_cancel();return;}}
    if(bestiary.open&&(bestiary.dragging||bestiary.thumb_drag)){
        const bool dragged=bestiary.thumb_drag||pointer.dragging;bestiary.dragging=bestiary.thumb_drag=false;
        if(dragged){pointer_cancel();return;}
    }
    if(map_dragging){pointer_cancel();return;}
    if(card_grid_drag>=0){
        auto&grid=card_grids[card_grid_drag];const bool dragged=grid.thumb_drag||pointer.dragging;
        grid.dragging=grid.thumb_drag=false;card_grid_drag=-1;if(dragged){pointer_cancel();return;}
    }
    pointer.held=false;
    HitRegion captured=pointer.captured;pointer.captured={};
    if(pointer.context!=input_context()||pointer.revision!=game.revision){pointer_cancel();return;}
    bool same=captured.kind==hit.kind&&captured.value==hit.value;
    if(captured.kind==HitKind::Hand&&game.phase==Phase::Combat&&captured.enabled){
        int id=captured.value,index=-1;
        for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==id)index=i;
        if(index<0||combat_actions_locked()){pointer_cancel();return;}
        Card card=game.cards[id];bool targeted=card_targets_enemy(game,card);
        bool dropping=pointer.dragging&&(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent||(!targeted&&hit.kind==HitKind::Battlefield));
        if(dropping&&hit.enabled){cursor=index;if(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent)target=hit.value;
            pointer.armed_card=id;pointer_key(Key::Confirm);return;}
        if(same&&!pointer.dragging){
            if(pointer.armed_card==id&&!targeted){cursor=index;pointer_key(Key::Confirm);}
            else pointer.armed_card=pointer.armed_card==id?-1:id;
            return;
        }
        pointer.armed_card=-1;return;
    }
    if(!same||!hit.enabled){if(pointer.dragging)pointer.armed_card=-1;return;}
    if(bestiary.open){bestiary_pointer(hit);return;}
    if(panel==4&&potion_targeting&&(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent||hit.kind==HitKind::PotionMerchant)){
        finish_potion_use(hit.kind==HitKind::PotionMerchant?-1:hit.value);return;
    }
    if(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent||hit.kind==HitKind::Battlefield){
        if(pointer.armed_card<0||combat_actions_locked()||game.phase!=Phase::Combat)return;
        int index=-1;for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==pointer.armed_card)index=i;
        if(index<0){pointer.armed_card=-1;return;}
        bool targeted=card_targets_enemy(game,game.cards[pointer.armed_card]);
        if(targeted&&hit.kind!=HitKind::Enemy&&hit.kind!=HitKind::EnemyIntent)return;
        cursor=index;if(hit.kind==HitKind::Enemy||hit.kind==HitKind::EnemyIntent)target=hit.value;
        pointer_key(Key::Confirm);return;
    }
    pointer.armed_card=-1;
    switch(hit.kind){
    case HitKind::Menu:menu=hit.value;pointer_key(Key::Confirm);break;
    case HitKind::Choice:cursor=hit.value;if(game.phase==Phase::Map)map_scroll=0;pointer_key(Key::Confirm);break;
    case HitKind::Hand:
        if(game.phase==Phase::ExhaustChoice)for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==hit.value){cursor=i;pointer_key(Key::Confirm);break;}
        break;
    case HitKind::Command:
        if(hit.y==0&&Key(hit.value)==Key::Back&&!title){pointer_cancel();paused=true;menu=0;}
        else pointer_key((Key)hit.value);break;
    case HitKind::OpenPile:panel=1;pile_tab=hit.value;scroll=inspect_cursor=0;pile_control_focus=-1;card_grids[unsigned(CardGridSurface::Pile)].reset();break;
    case HitKind::PileTab:pile_tab=hit.value;scroll=inspect_cursor=0;break;
    case HitKind::InstanceCard:open_choice_detail(hit.value);break;
    case HitKind::PileCard:open_card_detail(card_from_token(hit.value));break;
    case HitKind::DeckCard:if(hit.value>=0&&hit.value<game.deck_count){auto card=game.deck[hit.value];if(panel==1&&!pile_tab&&pile_upgraded&&card_upgradeable(card))card.upgraded=1;open_card_detail(card,hit.value);}break;
    case HitKind::PileSort:sort_pile(unsigned(hit.value));break;
    case HitKind::PileUpgrades:if(panel==1&&!pile_tab)pile_upgraded=!pile_upgraded;break;
    case HitKind::Relic:scroll=hit.value;open_owned_relic(hit.value);break;
    case HitKind::OwnedRelic:open_owned_relic(hit.value);break;
    case HitKind::Potion:if(panel==4&&scroll==hit.value)start_potion_use();break;
    case HitKind::PotionSlot:open_potion(hit.value);break;
    case HitKind::PotionPage:{const unsigned pages=(potion_slot_count(game)+2)/3;close_potion();potion_page=uint8_t((potion_page+1)%pages);break;}
    case HitKind::Scroll:pointer_scroll(hit.value);break;
    case HitKind::Close:pointer_key(Key::Back);break;
    case HitKind::DetailClose:close_card_inspect();break;
    case HitKind::ReferenceNode:map_preview.selected=hit.value;break;
    case HitKind::WorldNode:
        if(!map_legend&&map_routes_active())begin_map_travel(unsigned(hit.value));break;
    case HitKind::MapDrawingTool:map_drawing_tool(unsigned(hit.value));break;
    case HitKind::AncientDialogueNext:next_ancient_dialogue();break;
    case HitKind::EnemySelect:if(hit.value>=0&&hit.value<game.enemy_count&&game.enemies[hit.value].hp>0)target=hit.value;break;
    case HitKind::WorldChoice:toggle_world_choice(hit.value);break;
    case HitKind::WorldPreviewCard:{WorldSelectionCard model{};if(world_selection_active()&&hit.value>=0&&world_selection_preview_card(unsigned(hit.value),model)&&model.hover){
        if(model.deck>=0&&!model.upgraded&&model.facts.enchantment==game.event_cards.deck[model.deck].enchantment)open_card_detail(model.card,model.deck);
        else open_card_offer(model.card,model.facts,model.deck>=0?game.defect.genetic_deck[model.deck]:0,model.deck>=0?game.necro.scythe_deck[model.deck]:0);
    }break;}
    case HitKind::CrystalCell:
        if(game.phase==Phase::OriginalEventChoice&&hit.value>=0&&hit.value<CrystalCells){crystal_click(hit.value);}break;
    case HitKind::ReferenceAction:
        pointer_cancel();
        if(hit.value==0)map_preview.begin_edit();
        else if(hit.value==1)map_preview.change_act();
        else if(hit.value==2){map_preview.settings.increased_elites=!map_preview.settings.increased_elites;map_preview.generate();}
        break;
    case HitKind::SeedCharacter:reference_text(hit.value);break;
    case HitKind::HistoryCard:case HitKind::HistoryRelic:history_pointer(hit);break;
    case HitKind::UiAction:ui_action((UiAction)hit.value);break;
    case HitKind::LibraryItem:select_library_item(hit.value);break;
    case HitKind::LibraryControl:library_control(hit.value);break;
    case HitKind::RunModifier:run_modifier_select(unsigned(hit.value));break;
    case HitKind::InputBinding:begin_key_rebind(unsigned(hit.value));break;
    case HitKind::CombatChoice:toggle_choice(hit.value);break;
    case HitKind::StatusActor:open_statuses(hit.value);break;
    case HitKind::BattleStatus:open_status_icon(hit.value);break;
    case HitKind::PowerInstance:open_status_icon(hit.value&255);break;
    case HitKind::CombatPeek:toggle_combat_peek();break;
    case HitKind::AuxiliaryControl:auxiliary_control(hit.value);break;
    case HitKind::GameOverDetail:if(game_over_available()&&game_over_summary)game_over_focus=int8_t(hit.value);break;
    case HitKind::Orb:break; // Original orb focus retains its anchored hover tip.
    case HitKind::StatusItem:status_selected=hit.value;status_scroll=0;break;
    default:break;
    }
}
} // namespace spire
