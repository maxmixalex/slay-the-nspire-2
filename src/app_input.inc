#include "hand_focus.inc"
#include "combat_peek.inc"
#include "auxiliary_input.inc"
static void invalidate_hand_tiles();
static void invalidate_hud_motion();
#include "end_turn_hold.inc"
#include "ui_preferences.inc"
#include "ui_progress.inc"
#include "profiles_input.inc"
#include "ui_progress_navigation.inc"
#include "frontend_input.inc"
#include "world_map_travel.inc"
#include "world_map_act_intro.inc"
#include "world_map_drawing_input.inc"
#include "world_map_window.inc"
bool App::begin_startup_logo(){
    if(!title||!startup_logo.begin(map_clock_ms,content.has_startup_logo_assets(),skip_intro_logo))return false;
    pointer_cancel();physical_keys_cancel();hit_count=0;return true;
}
bool App::init(const uint8_t*p,size_t n,Callbacks cb){invalidate_hand_tiles();invalidate_hud_motion();reset_card_grids();card_library={};collection={};library_search_editing=false;reset_in_place(bestiary);hud_focus=-1;hud_focus_reticle={};reset_in_place(hud_relic_motion);card_inspect={};relic_inspect={};potion_belt_motion={};deck_count_pulse={};for(auto&reticle:target_reticles)reticle={};reward_presentation.reset();combat_feedback.reset();combat_status.reset_visuals();combat_sparks.reset();combat_cards.reset();combat_preparation.reset();merchant_preparation.reset();character_select_presentation.reset();character_departure={};character_random=false;room_presentation.reset();turn_banner.reset();reset_treasure_presentation();reset_map_reticles();for(auto&button:button_motions)button=ButtonMotion{};proceed_motion={};end_turn_glow={};end_turn_hold={};command_held=255;command_hold=false;map_travel={};map_visuals.reset();reset_map_window();reset_map_act_intro();profiles_open=profile_pending=false;for(auto&motion:profile_motion)motion={};for(auto&label:profile_labels)label={};reset_in_place(auxiliary);callbacks=cb;restore_preferences();restore_progress();if(!content.open(p,n)||!content.current_profile())return false;refresh_save();return true;}
bool App::init(ResourceSource source,Callbacks cb){invalidate_hand_tiles();invalidate_hud_motion();reset_card_grids();card_library={};collection={};library_search_editing=false;reset_in_place(bestiary);hud_focus=-1;hud_focus_reticle={};reset_in_place(hud_relic_motion);card_inspect={};relic_inspect={};potion_belt_motion={};deck_count_pulse={};for(auto&reticle:target_reticles)reticle={};reward_presentation.reset();combat_feedback.reset();combat_status.reset_visuals();combat_sparks.reset();combat_cards.reset();combat_preparation.reset();merchant_preparation.reset();character_select_presentation.reset();character_departure={};character_random=false;room_presentation.reset();turn_banner.reset();reset_treasure_presentation();reset_map_reticles();for(auto&button:button_motions)button=ButtonMotion{};proceed_motion={};end_turn_glow={};end_turn_hold={};command_held=255;command_hold=false;map_travel={};map_visuals.reset();reset_map_window();reset_map_act_intro();profiles_open=profile_pending=false;for(auto&motion:profile_motion)motion={};for(auto&label:profile_labels)label={};reset_in_place(auxiliary);callbacks=cb;restore_preferences();restore_progress();if(!content.open(source)||!content.current_profile())return false;refresh_save();return true;}
void App::message(const char*s){notice=s;notice_ticks=60;}
void App::animate(int actor,int id,int delay){if(actor<0||actor>MaxEnemies|| (actor&&game.enemy_world.active))return;motion[actor]={id,animation+(uint32_t)mx(0,delay)};}
// A retained room keeps its Proceed tween through a card-selection overlay.
// Its own room/phase identity resets it when actual travel replaces the node.
void App::reset_presentation(bool keep_map_visuals,unsigned retained_effects){content.cancel_preload();reset_creature_scaled_poses(*this);rich_label_clocks.reset();overflow_label_clocks.reset();main_menu_backdrop={};turn_banner_suppress_intro=combat_phase(game.phase)&&game.turn>0;screen_motion.clear_target(screen_motion.clock);combat_doom.reset();combat_speech_key=nullptr;title_presentation={};potion_popup_exit={};deck_selection.reset();combat_selection.reset();native_confirm_motion={};ftue.clear();history_tooltip={};game_over_presentation.reset();auxiliary.screen=AuxiliaryScreen::None;auxiliary.editing=auxiliary.closing=auxiliary.category_open=false;choice_peeking=false;choice_peek_from=0;choice_peek_at=map_clock_ms;choice_peek_revision=game.revision;reset_in_place(crystal_presentation);if(!keep_map_visuals)ancient_banner={};if(!keep_map_visuals)reset_in_place(ancient_dialogue);reset_in_place(bestiary);catalog_choice_revision=UINT32_MAX;catalog_choice_phase=255;++pile_sort_epoch;reset_card_grids();hud_focus=-1;hud_focus_reticle={};if(!(retained_effects&KeepHudRelicMotion))reset_in_place(hud_relic_motion);card_inspect={};relic_inspect.reset();reward_presentation.reset(retained_effects&255u);combat_feedback.reset();combat_status.reset_visuals();combat_sparks.reset();combat_cards.reset();combat_preparation.reset();merchant_preparation.reset();character_select_presentation.reset();character_departure={};character_random=false;room_presentation.reset();turn_banner.reset();turn_banner_seed=0;turn_banner_floor=turn_banner_combat=turn_banner_encounter=-1;turn_banner_number=0;keyword_hover={};reset_map_window();if(!keep_map_visuals){map_travel={};map_visuals.reset();reset_map_act_intro();}else if(map_travel.pending)cancel_map_travel();map_visuals.bind(game,map_clock_ms);reset_treasure_presentation();reset_map_reticles();for(auto&button:button_motions)button=ButtonMotion{};if(!keep_map_visuals)proceed_motion={};end_turn_glow={};end_turn_hold={};command_held=255;command_hold=false;for(auto&m:enemy_presentation)reset_in_place(m);creature_layout={};osty_presentation={};reset_in_place(orb_presentation);for(auto&bar:health_bars)bar.reset();health_bars_active=false;enemy_visual_clock=combat_linger_until=animation;shop_open=event_reader=grimoire_select=game_over_summary=history_from_game_over=false;game_over_focus=-1;event_reader_scroll=0;event_body_scroll.reset();event_body_key=0;event_body_focus=false;detail_owned=-1;relic_flash_ticks=0;relic_flash_mask=0;seen_relic_revision=game.relic_revision;seen_world_relic_revision=game.world_relics.flash_revision;world_relic_flash=NoWorldRelic;detail_has_facts=false;detail_genetic=detail_scythe=0;map_legend=false;map_legend_item=0;status_actor=-1;status_selected=0;deck_candidate=-1;pointer_cancel();choice_selected_count=0;choice_revision=game.revision;detail_card=detail_item=detail_instance=detail_deck_index=-1;detail_cost_override=-2;reward_cards=false;potion_targeting=false;potion_popup_slot=potion_removed_slot=-1;potion_removed=Potion::Empty;potion_belt_motion={};deck_count_pulse={};for(auto&reticle:target_reticles)reticle={};potion_page=potion_menu_item=0;cursor=target=panel=scroll=map_scroll=busy=card_fly=0;osty_motion.clip=-1;last_osty_hp=game.expansion.osty_hp;last_osty_attacks=game.necro.osty_attacks_turn;for(int i=0;i<=MaxEnemies;i++){motion[i].clip=-1;last_hit[i]=hit_start[i]=0;}for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}

void App::sync_character_feedback(){
    int hp=combat_health(MaxEnemies+1).hp,attacks=combat_status_fact(CombatStatus::OstyAttacks),clip=-1;
    if(combat_phase(game.phase)){
        if(hp<=0&&last_osty_hp>0)clip=AnimOstyDie;
        else if(hp>0&&last_osty_hp<=0)clip=AnimOstyRevive;
        else if(hp<last_osty_hp)clip=AnimOstyHurt;
        else if(hp>0&&attacks>last_osty_attacks)clip=AnimOstyAttack;
    }
    if(clip>=0)osty_motion={clip,animation};last_osty_hp=hp;last_osty_attacks=attacks;
}
void App::advance(uint32_t ms){
    if(startup_logo.active()){map_clock_ms+=ms>250?250:ms;startup_logo.advance(map_clock_ms);return;}
    if(long_press||end_turn_hold.owner||end_turn_hold.timer||
       (end_turn_hold.fired&&end_turn_hold.fade_age<EndTurnHold::FadeDuration))
        advance_end_turn_hold(ms>250?250:ms);
    if(debug_open){map_clock_ms+=ms>250?250:ms;return;}
    if(bestiary.open){if(ms>250)ms=250;map_clock_ms+=ms;bestiary.advance(ms);return;}
    sync_game_over_presentation();progress_time(ms);
    if(ms>250)ms=250;sync_combat_preparation();map_clock_ms+=ms;advance_auxiliary(ms);
    if(combat_preparation.active&&character_departure.phase==2)character_departure.started+=ms;
    if(const State*stock=title_merchant_stock(*this)){
        content.set_image_cache_profile(true);
        if(!merchant_preparation.planned)prepare_merchant_inventory(stock);
        merchant_preparation.shown=true;
    }
    // Continue has already restored the stock before its original800ms
    // character wipe. Use those ordinary frames for bounded preparation;
    // neither the wipe clock nor inventory input waits for this queue.
    if(character_departure.phase==1&&character_departure.continuing&&game.phase==Phase::Shop){
        content.set_image_cache_profile(true);
        if(!merchant_preparation.planned)prepare_merchant_inventory();
        merchant_preparation.shown=true;
    }
    advance_character_departure();
    if(character_departure.phase==1){step_merchant_preparation();return;}
    if(library&&card_library.query_pending&&map_clock_ms-card_library.query_changed>=250)commit_library_query();
    progress_page_scroll.step(ms);event_body_scroll.step(ms);
    if(reward_presentation.window_mode)reward_presentation.list_scroll.step(ms);
    for(auto&grid:card_grids)grid.advance(ms);room_presentation.merchant.step(map_clock_ms,ms);
    step_merchant_preparation();
    if(combat_preparation.active){
        // NTransition.RoomFadeIn begins only after the entered room is ready.
        // The bounded decoder may need several host updates; those updates
        // must not spend the fade-in while the room is still hidden.
        if(map_travel.fading_in)map_travel.fade_start+=ms;
        step_combat_preparation();return;
    }
    // Original Fast selects shorter command waits; it does not accelerate
    // banners, creature animations or health-bar tweens through Engine time.
    sync_treasure_presentation();tick_remainder+=ms;
    while(tick_remainder>=33){tick_remainder-=33;tick();}
    sync_treasure_presentation();sync_camp_presentation();update_map_travel();sync_map_window();map_drawing_advance(ms);sync_progress();
}
void App::draw_frame(){render();}
void App::tick(){
    if(startup_logo.active()){map_clock_ms+=33;startup_logo.advance(map_clock_ms);return;}
    if(debug_open)return;
    if(combat_preparation.active){step_combat_preparation();return;}
    if(ftue.id==FtueId::Shuffle)return;
    if(ftue.shuffle_waiting&&int32_t(animation+1-ftue.shuffle_at)>=0&&!ftue.active()){
        if(open_ftue(FtueId::Shuffle))return;ftue.shuffle_waiting=false;
    }
    animation++;sync_map_reticles();if(relic_flash_ticks>0)--relic_flash_ticks;if(notice_ticks>0)--notice_ticks;if(feedback>0)--feedback;if(busy>0)--busy;if(card_fly>0)--card_fly;}
#include "card_description.inc"
static int choice_count(const App&a){
    switch(a.game.phase){case Phase::Map:return a.game.world.active?reference::Columns:3;case Phase::Shop:return ShopLeave+1;case Phase::Event:return a.game.original_events.active?original_event_option_count(a.game):a.game.event_page>=2?1:2;
    case Phase::Reward:return a.game.world.active?int(a.game.world.reward_count)+1:4;case Phase::Camp:return world_rest_option_count(a.game);case Phase::Upgrade:case Phase::Remove:case Phase::Transform:return a.game.deck_count;
    case Phase::Treasure:return a.game.treasure_state?2:1;case Phase::ReturnChoice:return a.game.discard.count;case Phase::ExtendedChoice:return a.game.expansion.choices.count;default:return a.combat_hand().count;}
}
static void next_target(App&a){for(int i=1;i<=a.game.enemy_count;i++){int t=(a.target+i)%a.game.enemy_count;if(a.game.enemies[t].hp){a.target=t;break;}}}
#include "card_target_input.inc"
#include "potion_input.inc"
#include "world_map_input.inc"
#include "input_bindings.inc"
void App::input(Key k){
    if(startup_logo.active()){
        if(k==Key::Confirm||k==Key::Back)startup_logo.cancel(map_clock_ms);
        if(k==Key::Quit)quit=true;return;
    }
    // The original console has priority over every modal, including tutorial
    // popups, credits, relic inspection and the main menu.
    if(debug_input(k))return;
    if(character_departure.active()){if(k==Key::Quit)quit=true;return;}
    if(k!=Key::EndTurn&&!end_turn_hold.dispatching)end_turn_hold.unfocus();
    sync_ftues();if(ftue_input(k))return;
    if(detail_item>=0&&!settings_open&&!frontend_confirm){
        if(!pointer.dispatching){pointer_cancel(true);pointer.visible=false;pointer.motion_time=map_clock_ms;}
        relic_inspect_input(k);return;
    }
    if(auxiliary.screen!=AuxiliaryScreen::None){if(!pointer.dispatching){pointer_cancel(true);pointer.visible=false;pointer.motion_time=map_clock_ms;}auxiliary_input(k);return;}
    if(bestiary.open){if(!pointer.dispatching){pointer_cancel(true);pointer.visible=false;pointer.motion_time=map_clock_ms;}bestiary_input(k);return;}
    UiCoreObserver observer(*this);
    sync_combat_preparation();
    if(combat_preparation.active&&!paused&&!frontend_confirm&&k!=Key::Back&&k!=Key::Save&&k!=Key::Load&&k!=Key::Quit)return;
    MapReticleInputSync map_focus{*this};
    sync_map_window();
    // Persistence/host exit remain available during presentation. Ordinary
    // hotkeys follow NInputManager; pointer dispatch follows MouseFilter.
    if((pointer.dispatching?map_transition_blocks_pointer():map_transition_blocks_keyboard())&&
       k!=Key::Save&&k!=Key::Load&&k!=Key::Quit)return;
    if(!pointer.dispatching&&map_drawing_key(k))return;
    if(!pointer.dispatching&&(k==Key::Left||k==Key::Right||k==Key::Up||k==Key::Down))map_directional=true;
    if(!title&&!paused&&int32_t(combat_linger_until-animation)>0&&k!=Key::Back&&k!=Key::Save&&k!=Key::Load&&k!=Key::Quit)return;
    HandInputFocus focus(*this,k);
    if(!pointer.dispatching){pointer_cancel(true);pointer.visible=false;pointer.motion_time=map_clock_ms;}
    if(frontend_input(k))return;
    if(detail_item<0&&detail_card<0&&potion_input(k))return;
    if(map_input(k))return;
    if(room_input(k))return;
    if(keyword_detail_input(*this,k))return;
    if(status_input(k))return;
    if(reference_input(k))return;
    if(relic_inspect_input(k))return;
    if(detail_card>=0){
        if(k==Key::EndTurn&&library&&!title&&!paused&&!help){ui_action(UiAction::LabCard);return;}
        if(k==Key::Back||k==Key::Detail)close_card_inspect();
        else if(k==Key::Confirm)ui_action(UiAction::ToggleCardUpgrade);
        else if(k==Key::Up||k==Key::Down)keyword_hover_scroll(*this,k==Key::Up?-1:1);
        else if(k==Key::Left||k==Key::Right)move_card_inspect(k==Key::Left?-1:1);
        return;
    }
    if(combat_peek_input(k))return;
    if(card_grid_input(k))return;
    if(game_over_input(k))return;
    if(treasure_input(k)||camp_input(k))return;
    if(screen_input(k))return;
    // A canonical pending choice may be ahead of its visible command cue.
    // Keep inspection panels and persistence available; choice navigation,
    // details and confirmation become active together with the overlay.
    if(!panel&&!combat_choice_ready()&&(k==Key::Left||k==Key::Right||k==Key::Up||k==Key::Down||
       k==Key::Confirm||k==Key::EndTurn||k==Key::Detail))return;
    if(world_choice_input(k))return;
    if(card_reward_interactive()&&game.phase==Phase::Reward){
        sync_card_reward_presentation();
        if(k==Key::ExhaustPile){card_reward_alternative(false);return;}
        const unsigned count=game.world.active?game.world.reward_count:3;
        if(k==Key::Left||k==Key::Right){if(count)cursor=(cursor+int(count)+(k==Key::Left?-1:1))%int(count);return;}
        if(k==Key::Up||k==Key::Down)return;
        if(k==Key::Confirm&&!card_reward_input_ready())return;
    }
    if(selection_input(k))return;
    if(deck_input(k))return;
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel){
        if(game.phase==Phase::Reward&&reward_cards&&cursor<int(game.world.active?game.world.reward_count:3)){if(game.world.active)open_card_offer(game.world.reward_cards[cursor],game.world.reward_facts[cursor]);else open_card_detail({game.reward[cursor],0});return;}
        if(game.phase==Phase::Treasure&&game.treasure_state&&(game.world.active?game.world.treasure_relic!=NoWorldRelic:game.treasure_relic!=0)){
            if(game.world.active){detail_item=game.world.treasure_relic;detail_owned=-1;detail_scroll=0;return;}
            const auto*d=relic_definition(game.treasure_relic);
            if(d){detail_item=(int)(d-relic_definitions);detail_owned=-1;detail_scroll=0;}return;
        }
        if(game.phase==Phase::Shop){
            detail_owned=-1;detail_scroll=0;
            if(cursor<ShopCardCount){if(game.world.active)open_card_offer(game.world.shop_cards[cursor],game.world.shop_facts[cursor]);else open_card_detail({game.shop_cards[cursor],0});}
            else if(cursor<ShopPotionStart){if(game.world.active)detail_item=game.world.shop_relics[cursor-ShopRelicStart];else for(int i=0;i<RelicKinds;i++)if(relic_definitions[i].id==game.shop_relics[cursor-ShopRelicStart])detail_item=i;}
            // Potions retain their source hover/focus; there is no potion inspector.
            return;
        }
    }
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel&&combat_phase(game.phase)){
        const auto hand=combat_hand();
        if(hand.count)open_choice_detail(hand.ids[mn(mx(0,cursor),hand.count-1)]);return;
    }

    if(card_target_input(k))return;
    if(k==Key::Quit){cancel_map_travel();quit=true;return;}
    if(k==Key::Back){
        if(reward_cards&&!panel){reward_cards=false;cursor=1;return;}
        if(panel==3&&map_window.is_open)close_map_window();
        else if(panel){panel=0;scroll=0;if(!map_window.is_open)map_scroll=0;}
        else if(game.phase==Phase::Remove){game.phase=Phase::Shop;shop_open=true;cursor=ShopRemove;}
        else if(game.phase==Phase::Upgrade){if(game.world.active)world_rest_cancel(game);else game.phase=Phase::Camp;cursor=1;}
        else{paused=true;menu=0;}return;
    }
    if(k==Key::Save){message(save_current_run()?"已保存远征":"Could not save the run.");return;}
    if(k==Key::Load){if(load_saved_run())message("已载入");else message("No valid saved run was found.");return;}
    if(k==Key::Log){panel=panel==2?0:2;scroll=0;return;}
    if(k==Key::Inspect){panel=panel==1&&!pile_tab?0:1;pile_tab=0;scroll=inspect_cursor=0;pile_control_focus=-1;card_grids[unsigned(CardGridSurface::Pile)].reset();return;}
    if(k==Key::DrawPile||k==Key::DiscardPile||k==Key::ExhaustPile){
        if(combat_phase(game.phase)||choice_peeking){
            const int selected=k==Key::DrawPile?1:k==Key::DiscardPile?2:3;
            panel=panel==1&&pile_tab==selected?0:1;pile_tab=selected;scroll=inspect_cursor=0;pile_control_focus=-1;card_grids[unsigned(CardGridSurface::Pile)].reset();
        }return;
    }
    if(k==Key::Map){
        if(game.practice){message("组合练习没有远征路线");return;}
        if(game.world.active&&game.phase==Phase::Map)return; // MapRoom disables TopBar.Map.
        if(game.world.active&&game.phase!=Phase::Map){
            // The original top bar toggles Visible, which remains true during
            // the close tail; repeated clicks cannot restart or reopen it.
            if(panel&&panel!=3){panel=0;scroll=0;if(map_window.is_open)panel=3;else if(!map_window.visible)open_map_window(true);}
            else if(map_window.visible)close_map_window();else open_map_window(true);
        }else{panel=panel==3?0:3;map_scroll=0;}
        return;
    }
    if(k==Key::Relics){
        if(game_over_summary)return; // The source summary has no inventory grid.
        panel=map_window.is_open?3:0;hud_focus=ui_owned_relic_count(game)?HudRelicFocusFirst:-1;keyboard_card=-1;hand_focus=false;return;
    }
    if(k==Key::Potions){hud_focus=2;potion_page=0;keyboard_card=-1;hand_focus=false;return;}
    if(panel){
        if(panel==3){if(k==Key::Up)map_scroll=mn(reference::MaxRows,map_scroll+1);if(k==Key::Down)map_scroll=mx(-reference::MaxRows,map_scroll-1);return;}
        if(panel==1){
            const int count=int(pile_items());
            if(pile_control_focus>=0&&!pile_tab){
                if(k==Key::Left)pile_control_focus=mx(0,pile_control_focus-1);
                else if(k==Key::Right)pile_control_focus=mn(3,pile_control_focus+1);
                else if(k==Key::Down){inspect_cursor=mn(pile_control_focus,mx(0,count-1));pile_control_focus=-1;focus_card_grid();}
                else if(k==Key::Confirm)sort_pile(unsigned(pile_control_focus));
                else if(k==Key::EndTurn)pile_upgraded=!pile_upgraded;
                return;
            }
            if(k==Key::Up){if(inspect_cursor<5&&!pile_tab){pile_control_focus=mn(3,inspect_cursor);return;}inspect_cursor=mx(0,inspect_cursor-5);}
            if(k==Key::Down)inspect_cursor=mn(mx(0,count-1),inspect_cursor+5);
            if(k==Key::Left)inspect_cursor=mx(0,inspect_cursor-1);
            if(k==Key::Right)inspect_cursor=mn(mx(0,count-1),inspect_cursor+1);
            if(k==Key::EndTurn&&!pile_tab){pile_upgraded=!pile_upgraded;return;}
            if(k==Key::Confirm||k==Key::Detail){
                int ids[MaxCards];const unsigned count=pile_items(ids);
                if(unsigned(inspect_cursor)<count){const int id=ids[inspect_cursor];
                    if(pile_tab)open_choice_detail(id);
                    else{Card card=game.deck[id];if(pile_upgraded&&card_upgradeable(card))card.upgraded=1;open_card_detail(card,id);}}
                return;
            }
        }else{if(k==Key::Up)scroll=mx(0,scroll-1);if(k==Key::Down)scroll++;}
        return;
    }
    if(game.phase==Phase::Complete||game.phase==Phase::Dead||game.phase==Phase::Error){if(k==Key::Confirm){title=true;menu=0;}return;}
    int count=choice_count(*this);if(k==Key::Left)cursor=count?(cursor+count-1)%count:0;if(k==Key::Right)cursor=count?(cursor+1)%count:0;
    if(k==Key::Up||k==Key::Down){
        if(game.phase==Phase::Event){
            if(game.original_events.active&&!ui_ancient_event(game.original_events.id)&&game.original_events.id!=OriginalEventId::FakeMerchant)
                cursor=(cursor+(k==Key::Up?count-1:1))%mx(1,count);
            else scroll=mx(0,scroll+(k==Key::Up?-1:1));
        }
        else if(game.phase==Phase::Map){map_scroll=mx(-reference::MaxRows,mn(reference::MaxRows,map_scroll+(k==Key::Up?1:-1)));}
        else if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform)cursor=(cursor+(k==Key::Up?count-1:1))%mx(1,count);
        else if(game.phase==Phase::Camp)cursor=(cursor+(k==Key::Up?count-1:1))%mx(1,count);
        else if(!library&&combat_phase(game.phase))next_target(*this);
    }
    // Pending card/relic choices were handled above and may continue their
    // suspended effect. Ordinary gameplay waits for the visible side to end.
    if(combat_actions_locked()&&(k==Key::Confirm||k==Key::EndTurn))return;
    Phase before=game.phase;bool acted=false;int hp_before[MaxEnemies+1]={game.hp};
    for(int i=0;i<game.enemy_count;i++)hp_before[i+1]=game.enemies[i].hp;
    if(k==Key::EndTurn&&game.phase==Phase::Camp&&world_rest_can_proceed(game))acted=world_rest_proceed(game);
    if(k==Key::EndTurn&&game.phase==Phase::Combat){
        if(long_press&&!end_turn_hold.dispatching&&!ftue.skip_end_turn_check)return;
        if(!end_turn_hold.dispatching&&ftue_before_end_turn())return;
        uint8_t moves[MaxEnemies]{};for(int i=0;i<game.enemy_count;i++)moves[i]=game.enemies[i].move;begin_combat_feedback();acted=end_turn(game);if(!acted)combat_feedback.reset();
        if(acted){if(!game.enemy_world.active){for(int i=0;i<game.enemy_count;i++)if(hp_before[i+1])animate(i+1,moves[i]==2?AnimEnemyBuff:AnimEnemyAttack,i*12);animate(0,AnimHeroHurt,7);busy=18+10*(game.enemy_count-1);}feedback=10;}
    }
    if(k==Key::Confirm){
        switch(game.phase){
        case Phase::Combat:{
            if(ftue_before_play(cursor))return;
            Card card{};if(cursor<game.hand.count)card=game.cards[game.hand.ids[cursor]];
            begin_combat_feedback(cursor>=0&&cursor<game.hand.count?game.hand.ids[cursor]:-1);
            keyboard_card=-1;pointer.armed_card=-1;
            acted=play(game,cursor,target);
            if(!acted){combat_feedback.reset();combat_play_denied(cursor>=0&&cursor<game.hand.count?game.hand.ids[cursor]:-1);}
            else{ftue_manual_played();animate(0,card.kind==Shiv&&game.character==Character::Silent?AnimSilentShiv:definitions[card.kind].type==CardType::Attack?AnimHeroAttack:AnimHeroCast);
                if(!game.enemy_world.active){fly_card=card.kind;card_fly=12;busy=12;}feedback=8;}break;}
        case Phase::ExhaustChoice:acted=choose_exhaust(game,cursor);break;
        case Phase::Map:if(map_scroll){map_scroll=0;message("已返回可选楼层，再次确认进入");}else{acted=game.world.active?begin_map_column(cursor):choose_node(game,cursor);if(!acted)message("这条路线不相连，请选择亮起的节点");}break;
        case Phase::Shop:{
            const bool leaving=cursor==ShopLeave;const int item=cursor;
            const int price=item>=0&&item<ShopLeave?(game.world.active?world_shop_price(game,item):item==ShopRemove?remove_price(game):game.shop_prices[item]):0;
            const bool stocked=item>=0&&item<ShopLeave&&!(game.shop_sold&(1u<<item)),affordable=game.gold>=price;
            acted=leaving?leave_shop(game):buy(game,item);
            if(leaving){if(acted)proceed_map_window();}
            else if(!acted&&stocked){
                const bool potion=item>=ShopPotionStart&&item<ShopRemove;
                int reason=!affordable?1:potion&&has_world_relic(game,WorldRelicSozu)?3:2;
                room_presentation.merchant.failure(item,reason,map_clock_ms);
                if(potion&&affordable)potion_obtain_failed();
            }else if(acted&&game.phase==Phase::Shop){
                room_presentation.merchant.speak(0,map_clock_ms);
                cursor=merchant_navigation::after_purchase(merchant_navigation::stock(game,false),unsigned(item),false);
            }
            break;}
        case Phase::Remove:acted=remove_card(game,cursor);if(acted)message("已移除所选卡牌");break;
        case Phase::Event:{const bool fake=game.original_events.active&&game.original_events.id==OriginalEventId::FakeMerchant&&!game.original_events.finished;
            const int item=cursor;const auto event=game.original_events.id;const unsigned page=game.original_events.page;
            acted=event_choice(game,item);scroll=0;
            if(acted&&game.original_events.active){
                if(event==OriginalEventId::DenseVegetation&&!page&&item==1)screen_effect(ScreenShakePresentation::Rumble,ScreenShakePresentation::Medium,800,0,fast_mode?700:1500);
                else if(event==OriginalEventId::PunchOff&&!page&&!item)screen_effect(ScreenShakePresentation::Trauma,ScreenShakePresentation::Strong);
            }
            if(fake&&item<6){if(acted){room_presentation.merchant.speak(0,map_clock_ms);
                    cursor=merchant_navigation::after_purchase(merchant_navigation::stock(game,true),unsigned(item),true);
                }else room_presentation.merchant.failure(item,1,map_clock_ms);}
            break;}
        case Phase::Transform:acted=transform_card(game,cursor);if(!acted)message("请选择另一张尚未变化的牌");break;
        case Phase::Treasure:
            if(!game.treasure_state)acted=open_treasure(game);
            else if(cursor==0&&game.treasure_state==1){acted=take_treasure(game);if(acted)cursor=1;}
            else if(cursor==1)acted=leave_treasure(game);
            break;
        case Phase::Reward:if(reward_cards&&card_reward_input_ready())acted=choose_reward(game,cursor);if(acted){reward_cards=false;cursor=1;}break;
        case Phase::Camp:acted=game.world.active?world_rest_choose(game,cursor):camp(game,cursor==1);if(!acted)message("没有可以升级的牌");break;
        case Phase::Upgrade:acted=upgrade_card(game,cursor);if(!acted)message("这张牌已经升级");break;
        default:break;
        }
    }
    if(acted){
        if(!combat_feedback.active)for(int i=0;i<=game.enemy_count;i++){int hp=i?game.enemies[i-1].hp:game.hp;int hit=hp_before[i]-hp;if(hit>0){last_hit[i]=hit;hit_start[i]=(int)animation+6;if(i)animate(i,AnimEnemyHurt,6);}}
        if(game.phase!=before){shop_open=false;event_reader=false;reward_cards=false;cursor=0;scroll=map_scroll=0;if(game.phase==Phase::Map){for(int i=0;i<(game.world.active?reference::Columns:3);i++)if(game.world.active?([&](){uint8_t ids[reference::Columns];unsigned n=world_available_points(game,ids,reference::Columns);for(unsigned j=0;j<n;j++)if(game.world.maps[game.world.act_index].points[ids[j]].col==i)return true;return false;})():reachable(game,game.next_row*3+i)){cursor=i;break;}}}
        cursor=mn(cursor,mx(0,choice_count(*this)-1));if(target>=game.enemy_count||game.enemies[target].hp==0){target=0;for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
        // Finish clamping the retained event cursor before Open chooses the
        // map's reachable column. Repeated Proceed has no new 1->2 edge.
        if(before==Phase::Event&&game.world.active&&game.world.room_open==2&&world_can_travel(game))proceed_map_window();
    }
}
