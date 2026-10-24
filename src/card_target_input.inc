// The original directional path selects a card, restricts focus to its valid
// targets, then confirms the play. Pointer drag/release is a separate path.
bool App::card_target_input(Key key){
    if(pointer.dispatching)return false;
    const bool available=hand_can_focus()&&!combat_actions_locked();
    if(keyboard_card>=0&&(!available||keyboard_card_revision!=game.revision||keyboard_card>=game.card_count||game.zones[keyboard_card]!=Zone::Hand))keyboard_card=-1;
    if(!available)return false;
    if(keyboard_card>=0){
        int index=-1;for(unsigned i=0;i<game.hand.count;++i)if(game.hand.ids[i]==keyboard_card){index=int(i);break;}
        if(index<0){keyboard_card=-1;return false;}
        cursor=index;
        if(key==Key::Back){keyboard_card=-1;hand_focus=true;return true;}
        if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){
            if(card_targets_enemy(game,game.cards[keyboard_card],keyboard_card)){
                const int direction=key==Key::Left||key==Key::Up?-1:1;
                for(int step=1;step<=game.enemy_count;++step){const int next=(target+direction*step+game.enemy_count)%game.enemy_count;
                    if(game.enemies[next].hp>0){target=next;break;}}
            }return true;
        }
        if(key==Key::Confirm){keyboard_card=-1;return false;}
        keyboard_card=-1;return false;
    }
    if(key!=Key::Confirm||cursor<0||cursor>=game.hand.count)return false;
    const int instance=game.hand.ids[cursor];
    if(!can_play_card(game,instance)){message("资源不足，或尚未满足出牌条件");return true;}
    keyboard_card=instance;keyboard_card_revision=game.revision;
    // NControllerCardPlay keeps the last valid target, falling back to the
    // first hittable creature. Looking/selection never advances game state.
    if(target<0||target>=game.enemy_count||game.enemies[target].hp<=0){
        target=0;for(int i=0;i<game.enemy_count;++i)if(game.enemies[i].hp>0){target=i;break;}
    }
    hand_focus=false;return true;
}
