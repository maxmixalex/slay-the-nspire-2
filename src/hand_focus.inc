// Focus is presentation state, separate from the retained keyboard cursor.
// Returning from an overlay or a completed action must not leave a raised card.
bool App::hand_can_focus()const{
    return game.phase==Phase::Combat&&combat_hand().count&&!title&&!paused&&!help&&
        !panel&&!library&&!character_select&&!map_preview.open&&!event_reader&&
        detail_card<0&&detail_item<0&&deck_candidate<0&&status_actor<0;
}
struct HandInputFocus {
    App& app;Key key;uint32_t revision;bool active,previous;
    HandInputFocus(App&a,Key k):app(a),key(k),revision(a.game.revision),
        active(a.hand_can_focus()),previous(a.hand_focus){}
    ~HandInputFocus(){
        if(!active||!app.hand_can_focus()||revision!=app.game.revision){app.hand_focus=false;return;}
        if(key==Key::Left||key==Key::Right)app.hand_focus=true;
        else if(key==Key::Up||key==Key::Down||key==Key::Confirm)app.hand_focus=previous;
        else app.hand_focus=false;
    }
};
