// NCombatRoom owns the same screen-X focus graph for cards and thrown
// potions. Restricted navigation retains Up/Down and wraps Left/Right.
static int spatial_enemy_target(const App&a,int current,Key key,bool potion=false){
    if(key!=Key::Left&&key!=Key::Right)return current;
    struct Target {int id,x;} ordered[MaxEnemies];unsigned count=0;
    for(int i=0;i<a.game.enemy_count;++i)if(potion?can_use_potion(a.game,a.scroll,i):a.game.enemies[i].hp>0){
        const Target next{i,a.combat_enemy_pose(i).x};unsigned at=count++;
        while(at&&ordered[at-1].x>next.x){ordered[at]=ordered[at-1];--at;}ordered[at]=next;
    }
    if(!count)return current;
    unsigned at=0;while(at<count&&ordered[at].id!=current)++at;
    if(at==count)return ordered[key==Key::Left?count-1:0].id;
    return ordered[(at+(key==Key::Left?count-1:1))%count].id;
}
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
        if(key==Key::Back){sync_combat_cards();keyboard_card=-1;hand_focus=true;return true;}
        if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){
            if((key==Key::Left||key==Key::Right)&&card_targets_enemy(game,game.cards[keyboard_card],keyboard_card)){
                target=spatial_enemy_target(*this,target,key);
            }return true;
        }
        // Preserve the selected pose until begin_combat_feedback captures it.
        if(key==Key::Confirm)return false;
        keyboard_card=-1;return false;
    }
    if(key!=Key::Confirm||cursor<0||cursor>=game.hand.count)return false;
    const int instance=game.hand.ids[cursor];
    if(!can_play_card(game,instance)){if(!ftue_before_play(cursor))combat_play_denied(instance);return true;}
    sync_combat_cards();
    keyboard_card=instance;keyboard_card_revision=game.revision;
    // NControllerCardPlay keeps the last valid target, falling back to the
    // first hittable creature. Looking/selection never advances game state.
    if(target<0||target>=game.enemy_count||game.enemies[target].hp<=0){
        target=0;for(int i=0;i<game.enemy_count;++i)if(game.enemies[i].hp>0){target=i;break;}
    }
    hand_focus=false;return true;
}
