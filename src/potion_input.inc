// NPotionHolder / NPotionPopup in the pinned assembly: opening a bottle is
// separate from Use; thrown bottles are consumed only after a valid target.
void App::open_potion(int slot){
    if(title||paused||help||library||character_select||slot<0||slot>=int(potion_slot_count(game)))return;
    if(potion_at(game,unsigned(slot))==Potion::Empty)return;
    if(panel==4&&scroll==slot&&!potion_targeting){close_potion();return;}
    if(panel==4&&!potion_targeting)begin_potion_popup_exit(potion_at(game,unsigned(scroll)));
    pointer_cancel();hud_focus=-1;panel=4;scroll=slot;potion_page=uint8_t(slot/hud_potion_layout::Visible);
    potion_targeting=false;potion_menu_item=0;potion_popup_slot=slot;potion_popup_start=map_clock_ms;
    if(!potion_usable()&&can_discard_potion(game,slot))potion_menu_item=1;
}
static bool thrown_bottle(Potion bottle){
    const auto*d=potion_definition(bottle);
    return d&&(d->targeted||bottle==Potion::Explosive||bottle==Potion::PotionOfBinding||bottle==Potion::ShacklingPotion||bottle==Potion::FoulPotion);
}
void App::begin_potion_popup_exit(Potion bottle){
    if(panel!=4||potion_targeting||potion_popup_slot<0||bottle==Potion::Empty)return;
    const unsigned age=map_clock_ms-potion_popup_start;
    potion_popup_exit.start=map_clock_ms;potion_popup_exit.x=int16_t(hud_potion_layout::center_x(unsigned(scroll))-potion_popup_layout::W/2);
    potion_popup_exit.y=int16_t(33-5*CombatCardMotion::sine(1024+int(age>=150?150:age)*1024/150)/1024);
    potion_popup_exit.alpha=uint8_t(age>=100?255:(1024-CombatCardMotion::sine(1024+int(age)*2048/100))*255/2048);
    potion_popup_exit.thrown=thrown_bottle(bottle);potion_popup_exit.active=true;
}
void App::close_potion(Potion bottle){
    if(bottle==Potion::Empty&&scroll>=0&&scroll<int(potion_slot_count(game)))bottle=potion_at(game,unsigned(scroll));
    begin_potion_popup_exit(bottle);
    if(panel==4)panel=0;
    potion_targeting=false;potion_popup_slot=-1;potion_menu_item=0;pointer_cancel();
}
bool App::potion_needs_target()const{
    const Potion bottle=potion_at(game,unsigned(scroll));const auto*d=potion_definition(bottle);
    return d&&(d->targeted||(bottle==Potion::FoulPotion&&game.phase!=Phase::Combat));
}
bool App::potion_is_thrown()const{
    const Potion bottle=potion_at(game,unsigned(scroll));
    // NPotionPopup includes AnyEnemy, AllEnemies and TargetedNoCreature.
    return thrown_bottle(bottle);
}
bool App::potion_usable()const{
    if(panel!=4||combat_actions_locked())return false;
    const Potion bottle=potion_at(game,unsigned(scroll));const auto*d=potion_definition(bottle);
    if(!d)return false;
    if(bottle==Potion::FoulPotion&&game.phase==Phase::Shop&&shop_open)return false;
    if(d->targeted){for(int i=0;i<game.enemy_count;++i)if(can_use_potion(game,scroll,i))return true;return false;}
    return can_use_potion(game,scroll,target);
}
void App::remove_potion_visual(Potion bottle,int slot,bool discarded){
    potion_removed=bottle;potion_removed_slot=slot;potion_removed_start=map_clock_ms;potion_removed_discard=discarded;
}
bool App::finish_potion_use(int selected_target){
    if(panel!=4||!potion_usable())return false;
    const int slot=scroll;const Potion bottle=potion_at(game,unsigned(slot));
    if(!can_use_potion(game,slot,selected_target))return false;
    begin_combat_feedback();
    if(!use_potion(game,slot,selected_target)){combat_feedback.reset();combat_status.reset();return false;}
    remove_potion_visual(bottle,slot,false);close_potion(bottle);
    choice_selected_count=0;choice_revision=game.revision;
    if(target>=game.enemy_count||!enemy_allow_target(game,target))for(int i=0;i<game.enemy_count;++i)if(enemy_allow_target(game,i)){target=i;break;}
    return true;
}
void App::start_potion_use(){
    if(!potion_usable())return;
    if(potion_needs_target()){
        begin_potion_popup_exit(potion_at(game,unsigned(scroll)));
        pointer_cancel();potion_targeting=true;
        if(game.phase==Phase::Combat&&!can_use_potion(game,scroll,target))
            for(int i=0;i<game.enemy_count;++i)if(can_use_potion(game,scroll,i)){target=i;break;}
    }else finish_potion_use(target);
}
bool App::potion_input(Key key){
    if(hud_focus>=0&&panel!=4){
        if(title||paused||help||library||character_select||settings_open||frontend_confirm||status_actor>=0||event_reader){hud_focus=-1;return false;}
        if(key==Key::Save||key==Key::Load||key==Key::Quit)return false;
        const int slots=int(potion_slot_count(game));
        const auto return_to_screen=[&](){
            hud_focus=-1;
            if(panel==1){pile_control_focus=-1;focus_card_grid();}
            else if(!panel&&combat_phase(game.phase)){hand_focus=true;cursor=mn(mx(0,cursor),mx(0,int(combat_hand().count)-1));}
        };
        if(hud_focus>=HudRelicFocusFirst){
            const int count=int(ui_owned_relic_count(game));if(!count){return_to_screen();return false;}
            int index=mn(hud_focus-HudRelicFocusFirst,count-1);
            if(key==Key::Left||key==Key::Right){index=(index+count+(key==Key::Left?-1:1))%count;hud_focus=HudRelicFocusFirst+index;return true;}
            if(key==Key::Up||key==Key::Potions){hud_focus=2;potion_page=0;return true;}
            if(key==Key::Confirm||key==Key::Detail){open_owned_relic(index);return true;}
            if(key==Key::Down||key==Key::Back||key==Key::Relics){return_to_screen();return true;}
            hud_focus=-1;return false;
        }
        hud_focus=mn(hud_focus,slots+5);
        if(key==Key::Left||key==Key::Right){
            const auto room=hud_room_info(game);const int step=key==Key::Left?-1:1;
            for(int next=hud_focus+step;next>=0&&next<=slots+5;next+=step){
                if(next==slots+2&&!room.room_enabled)continue;if(next==slots+4&&!room.boss_enabled)continue;
                hud_focus=next;break;
            }
            if(hud_focus>=2&&hud_focus<slots+2)potion_page=uint8_t((hud_focus-2)/hud_potion_layout::Visible);
            return true;
        }
        if(key==Key::Up)return true;
        if(key==Key::Down){
            if((!panel||panel==3)&&ui_owned_relic_count(game))hud_focus=HudRelicFocusFirst;
            else return_to_screen();return true;
        }
        if(key==Key::Confirm){if(hud_focus>=2&&hud_focus<slots+2)open_potion(hud_focus-2);return true;}
        if(key==Key::Detail)return true; // NPotionHolder exposes its hover tip, not a separate inspect screen.
        hud_focus=-1;
        if(key==Key::Back||key==Key::Potions)return true;
        return false;
    }
    if(panel!=4)return false;
    if(key==Key::Save||key==Key::Load||key==Key::Quit)return false;
    if(key==Key::Map||key==Key::Inspect||key==Key::Relics||key==Key::Log){close_potion();return false;}
    if(key==Key::Back||key==Key::Potions){close_potion();return true;}
    if(potion_targeting){
        if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){
            target=spatial_enemy_target(*this,target,key,true);
        }else if(key==Key::Confirm)finish_potion_use(target);
        return true;
    }
    if(key==Key::Left||key==Key::Right){
        const int count=int(potion_slot_count(game)),direction=key==Key::Left?-1:1;
        for(int i=1;i<=count;++i){const int next=(scroll+count+i*direction)%count;
            if(potion_at(game,unsigned(next))!=Potion::Empty){if(next!=scroll)open_potion(next);break;}}
    }else if(key==Key::Up||key==Key::Down)potion_menu_item^=1;
    else if(key==Key::Confirm){if(potion_menu_item)ui_action(UiAction::DiscardPotion);else start_potion_use();}
    else if(key==Key::Detail)return true;
    return true;
}

void App::potion_obtain_failed(){
    if(has_world_relic(game,WorldRelicSozu))return;
    for(unsigned i=0;i<potion_slot_count(game);++i)if(potion_at(game,i)==Potion::Empty)return;
    potion_belt_motion.fail(map_clock_ms);
}
