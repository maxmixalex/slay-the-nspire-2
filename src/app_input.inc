#include "hand_focus.inc"
static void invalidate_hand_tiles();
#include "frontend_input.inc"
#include "world_map_travel.inc"
#include "world_map_act_intro.inc"
#include "world_map_window.inc"
bool App::init(const uint8_t*p,size_t n,Callbacks cb){invalidate_hand_tiles();combat_preparation.reset();character_select_presentation.reset();turn_banner.reset();reset_treasure_presentation();reset_map_reticles();for(auto&button:button_motions)button=ButtonMotion{};end_turn_glow={};map_travel={};map_visuals.reset();reset_map_window();reset_map_act_intro();callbacks=cb;if(!content.open(p,n)||!content.current_profile())return false;refresh_save();return true;}
bool App::init(ResourceSource source,Callbacks cb){invalidate_hand_tiles();combat_preparation.reset();character_select_presentation.reset();turn_banner.reset();reset_treasure_presentation();reset_map_reticles();for(auto&button:button_motions)button=ButtonMotion{};end_turn_glow={};map_travel={};map_visuals.reset();reset_map_window();reset_map_act_intro();callbacks=cb;if(!content.open(source)||!content.current_profile())return false;refresh_save();return true;}
void App::message(const char*s){notice=s;notice_ticks=60;}
void App::animate(int actor,int id,int delay){if(actor<0||actor>MaxEnemies|| (actor&&game.enemy_world.active))return;motion[actor]={id,animation+(uint32_t)mx(0,delay)};}
void App::reset_presentation(bool keep_map_visuals){combat_preparation.reset();character_select_presentation.reset();turn_banner.reset();turn_banner_seed=0;turn_banner_floor=turn_banner_combat=turn_banner_encounter=-1;turn_banner_number=0;keyword_hover={};reset_map_window();if(!keep_map_visuals){map_travel={};map_visuals.reset();reset_map_act_intro();}else if(map_travel.pending)cancel_map_travel();map_visuals.bind(game,map_clock_ms);reset_treasure_presentation();reset_map_reticles();for(auto&button:button_motions)button=ButtonMotion{};end_turn_glow={};for(auto&m:enemy_presentation)m=EnemyPresentation{};for(auto&bar:health_bars)bar.reset();health_bars_active=false;enemy_visual_clock=combat_linger_until=animation;shop_open=event_reader=grimoire_select=game_over_summary=false;event_reader_scroll=0;detail_owned=-1;relic_flash_ticks=0;relic_flash_mask=0;seen_relic_revision=game.relic_revision;seen_world_relic_revision=game.world_relics.flash_revision;world_relic_flash=NoWorldRelic;detail_has_facts=false;detail_genetic=detail_scythe=0;map_legend=false;map_legend_item=0;status_actor=-1;status_selected=0;deck_candidate=-1;pointer_cancel();choice_selected_count=0;choice_revision=game.revision;detail_card=detail_item=detail_instance=detail_deck_index=-1;detail_cost_override=-2;reward_cards=false;cursor=target=panel=scroll=map_scroll=busy=card_fly=0;osty_motion.clip=-1;last_osty_hp=game.expansion.osty_hp;last_osty_attacks=game.necro.osty_attacks_turn;for(int i=0;i<=MaxEnemies;i++){motion[i].clip=-1;last_hit[i]=hit_start[i]=0;}for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
void App::sync_character_feedback(){
    int hp=game.expansion.osty_hp,attacks=game.necro.osty_attacks_turn,clip=-1;
    if(combat_phase(game.phase)){
        if(hp<=0&&last_osty_hp>0)clip=AnimOstyDie;
        else if(hp>0&&last_osty_hp<=0)clip=AnimOstyRevive;
        else if(hp<last_osty_hp)clip=AnimOstyHurt;
        else if(hp>0&&attacks>last_osty_attacks)clip=AnimOstyAttack;
    }
    if(clip>=0)osty_motion={clip,animation};last_osty_hp=hp;last_osty_attacks=attacks;
}
void App::advance(uint32_t ms){if(ms>250)ms=250;sync_combat_preparation();map_clock_ms+=ms;if(combat_preparation.active){step_combat_preparation();return;}sync_treasure_presentation();tick_remainder+=ms*(fast_mode?2:1);while(tick_remainder>=33){tick_remainder-=33;tick();}sync_treasure_presentation();update_map_travel();sync_map_window();}
void App::draw_frame(){render();}
void App::tick(){if(combat_preparation.active){step_combat_preparation();return;}animation++;sync_map_reticles();if(relic_flash_ticks>0)--relic_flash_ticks;if(notice_ticks>0)--notice_ticks;if(feedback>0)--feedback;if(busy>0)--busy;if(card_fly>0)--card_fly;}
#include "card_description.inc"
static int choice_count(const App&a){
    switch(a.game.phase){case Phase::Map:return a.game.world.active?reference::Columns:3;case Phase::Shop:return ShopLeave+1;case Phase::Event:return a.game.original_events.active?original_event_option_count(a.game):a.game.event_page>=2?1:2;
    case Phase::Reward:return a.game.world.active?int(a.game.world.reward_count)+1:4;case Phase::Camp:return world_rest_option_count(a.game);case Phase::Upgrade:case Phase::Remove:case Phase::Transform:return a.game.deck_count;
    case Phase::Treasure:return a.game.treasure_state?2:1;case Phase::ReturnChoice:return a.game.discard.count;case Phase::ExtendedChoice:return a.game.expansion.choices.count;default:return a.game.hand.count;}
}
static void next_target(App&a){for(int i=1;i<=a.game.enemy_count;i++){int t=(a.target+i)%a.game.enemy_count;if(a.game.enemies[t].hp){a.target=t;break;}}}
#include "world_map_input.inc"
void App::input(Key k){
    UiCoreObserver observer(*this);
    sync_combat_preparation();
    if(combat_preparation.active&&!paused&&!frontend_confirm&&k!=Key::Back&&k!=Key::Save&&k!=Key::Load&&k!=Key::Quit)return;
    MapReticleInputSync map_focus{*this};
    sync_map_window();
    // Persistence/host exit remain available during presentation. Ordinary
    // hotkeys follow NInputManager; pointer dispatch follows MouseFilter.
    if((pointer.dispatching?map_transition_blocks_pointer():map_transition_blocks_keyboard())&&
       k!=Key::Save&&k!=Key::Load&&k!=Key::Quit)return;
    if(!pointer.dispatching&&(k==Key::Left||k==Key::Right||k==Key::Up||k==Key::Down))map_directional=true;
    if(!title&&!paused&&int32_t(combat_linger_until-animation)>0&&k!=Key::Back&&k!=Key::Save&&k!=Key::Load&&k!=Key::Quit)return;
    HandInputFocus focus(*this,k);
    if(!pointer.dispatching){pointer_cancel();pointer.visible=false;}
    if(frontend_input(k))return;
    if(map_input(k))return;
    if(room_input(k))return;
    if(keyword_detail_input(*this,k))return;
    if(status_input(k))return;
    if(reference_input(k))return;
    if(detail_item>=0){
        if(k==Key::EndTurn&&library&&!title&&!paused&&!help){ui_action(UiAction::LabPotion);return;}
        if(k==Key::Back||k==Key::Confirm||k==Key::Detail){detail_item=-1;detail_owned=-1;}
        else if(k==Key::Up)detail_scroll=mx(0,detail_scroll-1);
        else if(k==Key::Down)++detail_scroll;
        return;
    }
    if(detail_card>=0){
        if(k==Key::EndTurn&&library&&!title&&!paused&&!help){ui_action(UiAction::LabCard);return;}
        if(k==Key::Back||k==Key::Confirm||k==Key::Detail)detail_card=-1;
        else if(k==Key::Up)detail_scroll=mx(0,detail_scroll-1);
        else if(k==Key::Down)++detail_scroll;
        else if(k==Key::Left||k==Key::Right)ui_action(UiAction::ToggleCardUpgrade);
        return;
    }
    if(game_over_input(k))return;
    if(treasure_input(k)||camp_input(k))return;
    if(screen_input(k))return;
    if(world_choice_input(k))return;
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
            else if(cursor<ShopRemove)detail_item=WorldRelicKinds+game.shop_potions[cursor-ShopPotionStart]-1;
            return;
        }
    }
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel&&combat_phase(game.phase)&&game.hand.count){
        open_choice_detail(game.hand.ids[mn(mx(0,cursor),game.hand.count-1)]);return;
    }

    if(k==Key::Quit){cancel_map_travel();quit=true;return;}
    if(k==Key::Back){
        if(reward_cards&&!panel){reward_cards=false;cursor=1;return;}
        if(panel==3&&map_window.is_open)close_map_window();
        else if(panel){panel=0;scroll=0;if(!map_window.is_open)map_scroll=0;}
        else if(game.phase==Phase::Remove){game.phase=Phase::Shop;shop_open=true;cursor=ShopRemove;}
        else if(game.phase==Phase::Upgrade){if(game.world.active)world_rest_cancel(game);else game.phase=Phase::Camp;cursor=1;}
        else{paused=true;menu=0;}return;
    }
    if(k==Key::Save){message(save_current_run()?"已保存远征":"保存失败");return;}
    if(k==Key::Load){if(load_saved_run())message("已载入");else message("没有有效的存档");return;}
    if(k==Key::Log){panel=panel==2?0:2;scroll=0;return;}
    if(k==Key::Inspect){panel=panel==1?0:1;scroll=inspect_cursor=0;return;}
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
    if(k==Key::Relics){panel=panel==5?0:5;scroll=0;return;}
    if(k==Key::Potions){panel=panel==4?0:4;scroll=0;return;}
    if(panel){
        if(panel==5){
            if(k==Key::Left)scroll=mx(0,scroll-1);
            if(k==Key::Right)scroll=mn(mx(0,int(ui_owned_relic_count(game))-1),scroll+1);
            if(k==Key::Up)scroll=mx(0,scroll-6);
            if(k==Key::Down)scroll=mn(mx(0,int(ui_owned_relic_count(game))-1),scroll+6);
            if(k==Key::Confirm||k==Key::Detail)open_owned_relic(scroll);
            return;
        }
        if(panel==4){
            if(k==Key::Left||k==Key::Right)scroll=(scroll+int(potion_slot_count(game))+(k==Key::Left?-1:1))%int(potion_slot_count(game));if(k==Key::Up||k==Key::Down)next_target(*this);
            if(k==Key::Confirm){if(use_potion(game,scroll,target)){panel=0;busy=choice_phase(game.phase)?0:8;next_target(*this);cursor=0;choice_selected_count=0;choice_revision=game.revision;message("药水已使用");}else message("此时无法使用，或药水槽为空");}return;
        }
        if(panel==3){if(k==Key::Up)map_scroll=mn(reference::MaxRows,map_scroll+1);if(k==Key::Down)map_scroll=mx(-reference::MaxRows,map_scroll-1);return;}
        if(panel==1){
            if(k==Key::Up)inspect_cursor=mx(0,inspect_cursor-1);
            if(k==Key::Down)++inspect_cursor;
            scroll=(inspect_cursor/10)*10;
            if(k==Key::Confirm||k==Key::Detail){
                render(); // Keyboard batches may arrive before the next timed frame.
                const HitRegion* pick=nullptr;int n=0;
                for(int i=0;i<hit_count;i++)if((hits[i].kind==HitKind::PileCard||hits[i].kind==HitKind::InstanceCard||hits[i].kind==HitKind::DeckCard)){if(n++==inspect_cursor%10){pick=&hits[i];break;}}
                if(pick){if(pick->kind==HitKind::InstanceCard)open_choice_detail(pick->value);else if(pick->kind==HitKind::DeckCard)open_card_detail(game.deck[pick->value],pick->value);else open_card_detail(card_from_token(pick->value));}
                return;
            }
        }else{if(k==Key::Up)scroll=mx(0,scroll-1);if(k==Key::Down)scroll++;}
        if(panel==1&&(k==Key::Left||k==Key::Right)){pile_tab=(pile_tab+(k==Key::Left?4:1))%5;scroll=inspect_cursor=0;}return;
    }
    if(game.phase==Phase::Complete||game.phase==Phase::Dead||game.phase==Phase::Error){if(k==Key::Confirm){title=true;menu=0;}return;}
    int count=choice_count(*this);if(k==Key::Left)cursor=count?(cursor+count-1)%count:0;if(k==Key::Right)cursor=count?(cursor+1)%count:0;
    if(k==Key::Up||k==Key::Down){
        if(game.phase==Phase::Event){scroll=mx(0,scroll+(k==Key::Up?-1:1));}
        else if(game.phase==Phase::Map){map_scroll=mx(-reference::MaxRows,mn(reference::MaxRows,map_scroll+(k==Key::Up?1:-1)));}
        else if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform)cursor=(cursor+(k==Key::Up?count-1:1))%mx(1,count);
        else if(game.phase==Phase::Shop){
            const int below[15]={5,6,7,8,13,14,14,10,11,12,14,14,14,14,0};
            const int above[15]={14,14,14,14,14,0,1,2,3,4,7,8,9,4,5};
            cursor=(k==Key::Down?below:above)[cursor];
        }
        else if(game.phase==Phase::Camp)cursor=(cursor+(k==Key::Up?count-1:1))%mx(1,count);
        else if(!library&&combat_phase(game.phase))next_target(*this);
    }
    if(busy&&(k==Key::Confirm||k==Key::EndTurn))return;
    Phase before=game.phase;bool acted=false;int hp_before[MaxEnemies+1]={game.hp};
    for(int i=0;i<game.enemy_count;i++)hp_before[i+1]=game.enemies[i].hp;
    if(k==Key::EndTurn&&game.phase==Phase::Camp&&world_rest_can_proceed(game))acted=world_rest_proceed(game);
    if(k==Key::EndTurn&&game.phase==Phase::Combat){
        uint8_t moves[MaxEnemies]{};for(int i=0;i<game.enemy_count;i++)moves[i]=game.enemies[i].move;acted=end_turn(game);
        if(acted){if(!game.enemy_world.active){for(int i=0;i<game.enemy_count;i++)if(hp_before[i+1])animate(i+1,moves[i]==2?AnimEnemyBuff:AnimEnemyAttack,i*12);animate(0,AnimHeroHurt,7);busy=18+10*(game.enemy_count-1);}else if(game.hp<hp_before[0])animate(0,AnimHeroHurt,TurnBannerPresentation::EnemyLeadTicks+TurnBannerPresentation::IntentLeadTicks+7);feedback=10;}
    }
    if(k==Key::Confirm){
        switch(game.phase){
        case Phase::Combat:{
            Card card{};if(cursor<game.hand.count)card=game.cards[game.hand.ids[cursor]];
            acted=play(game,cursor,target);
            if(!acted)message("资源不足，或尚未满足出牌条件");else{animate(0,card.kind==Shiv&&game.character==Character::Silent?AnimSilentShiv:definitions[card.kind].type==CardType::Attack?AnimHeroAttack:AnimHeroCast);fly_card=card.kind;card_fly=12;busy=12;feedback=8;}break;}
        case Phase::ExhaustChoice:acted=choose_exhaust(game,cursor);break;
        case Phase::Map:if(map_scroll){map_scroll=0;message("已返回可选楼层，再次确认进入");}else{acted=game.world.active?begin_map_column(cursor):choose_node(game,cursor);if(!acted)message("这条路线不相连，请选择亮起的节点");}break;
        case Phase::Shop:{const bool leaving=cursor==ShopLeave;acted=leaving?leave_shop(game):buy(game,cursor);if(!acted)message("金币不足、已售出，或物品槽已满");else if(leaving)proceed_map_window();else if(game.phase==Phase::Shop)message("购买成功");break;}
        case Phase::Remove:acted=remove_card(game,cursor);if(acted)message("已移除所选卡牌");break;
        case Phase::Event:acted=event_choice(game,cursor);scroll=0;break;
        case Phase::Transform:acted=transform_card(game,cursor);if(!acted)message("请选择另一张尚未变化的牌");break;
        case Phase::Treasure:
            if(!game.treasure_state)acted=open_treasure(game);
            else if(cursor==0&&game.treasure_state==1){acted=take_treasure(game);if(acted)cursor=1;}
            else if(cursor==1)acted=leave_treasure(game);
            break;
        case Phase::Reward:acted=choose_reward(game,cursor==int(game.world.active?game.world.reward_count:3)?-1:cursor);if(acted){reward_cards=false;cursor=1;}break;
        case Phase::Camp:acted=game.world.active?world_rest_choose(game,cursor):camp(game,cursor==1);if(!acted)message("没有可以升级的牌");break;
        case Phase::Upgrade:acted=upgrade_card(game,cursor);if(!acted)message("这张牌已经升级");break;
        default:break;
        }
    }
    if(acted){
        for(int i=0;i<=game.enemy_count;i++){int hp=i?game.enemies[i-1].hp:game.hp;int hit=hp_before[i]-hp;if(hit>0){last_hit[i]=hit;hit_start[i]=(int)animation+6;if(i)animate(i,AnimEnemyHurt,6);}}
        if(game.phase!=before){shop_open=false;event_reader=false;reward_cards=false;cursor=0;scroll=map_scroll=0;if(game.phase==Phase::Map){for(int i=0;i<(game.world.active?reference::Columns:3);i++)if(game.world.active?([&](){uint8_t ids[reference::Columns];unsigned n=world_available_points(game,ids,reference::Columns);for(unsigned j=0;j<n;j++)if(game.world.maps[game.world.act_index].points[ids[j]].col==i)return true;return false;})():reachable(game,game.next_row*3+i)){cursor=i;break;}}}
        cursor=mn(cursor,mx(0,choice_count(*this)-1));if(target>=game.enemy_count||game.enemies[target].hp==0){target=0;for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
        // Finish clamping the retained event cursor before Open chooses the
        // map's reachable column. Repeated Proceed has no new 1->2 edge.
        if(before==Phase::Event&&game.world.active&&game.world.room_open==2&&world_can_travel(game))proceed_map_window();
    }
}
