// Focus is presentation state, separate from the retained keyboard cursor.
// Returning from an overlay or a completed action must not leave a raised card.
bool App::hand_can_focus()const{
    return game.phase==Phase::Combat&&combat_hand().count&&!title&&!paused&&!help&&
        !panel&&!library&&!character_select&&!map_preview.open&&!event_reader&&!settings_open&&!frontend_confirm&&
        detail_card<0&&detail_item<0&&deck_candidate<0&&status_actor<0&&hud_focus<0;
}
int App::selected_hand_card()const{
    if(!hand_can_focus())return -1;
    int id=keyboard_card>=0?keyboard_card:pointer.armed_card;
    if(id<0&&pointer.held&&pointer.dragging&&pointer.captured.kind==HitKind::Hand)id=pointer.captured.value;
    return id>=0&&id<game.card_count&&game.zones[id]==Zone::Hand?id:-1;
}
struct HandInputFocus {
    App& app;Key key;uint32_t revision;bool active,previous,card_selected;
    HandInputFocus(App&a,Key k):app(a),key(k),revision(a.game.revision),
        active(a.hand_can_focus()),previous(a.hand_focus),card_selected(a.keyboard_card>=0){}
    ~HandInputFocus(){
        if(!active||!app.hand_can_focus()||revision!=app.game.revision){app.hand_focus=false;return;}
        if(app.keyboard_card>=0){app.hand_focus=false;return;}
        if(card_selected&&key==Key::Back){app.hand_focus=true;return;}
        if(key==Key::Left||key==Key::Right)app.hand_focus=true;
        else if(key==Key::Up||key==Key::Down||key==Key::Confirm)app.hand_focus=previous;
        else app.hand_focus=false;
    }
};

// Enemy-specific preview values belong to the card currently being aimed.
// A retained navigation target must not make every idle card show Vulnerable.
int App::card_preview_target(int instance)const{
    if(instance<0||instance>=game.card_count||target<0||target>=game.enemy_count||game.enemies[target].hp<=0||
       selected_hand_card()!=instance||!card_targets_enemy(game,game.cards[instance],instance))return -1;
    if(keyboard_card==instance)return target;
    if(pointer.visible){const auto*hit=hit_at(pointer.x,pointer.y);
        if(hit&&(hit->kind==HitKind::Enemy||hit->kind==HitKind::EnemyIntent)&&hit->value==target)return target;}
    return -1;
}
void App::combat_play_denied(int instance){
    const char*key="UNPLAYABLE";
    if(instance>=0&&instance<game.card_count){
        if(cost(game,instance)>game.energy)key="NOT_ENOUGH_ENERGY";
        else if(card_star_cost(game,game.cards[instance],instance)>game.expansion.stars)key="NOT_ENOUGH_STARS";

    }
    combat_speech_key=key;combat_speech_at=map_clock_ms;
}
