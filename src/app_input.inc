static void invalidate_hand_tiles();
bool App::init(const uint8_t*p,size_t n,Callbacks cb){invalidate_hand_tiles();callbacks=cb;return content.open(p,n)&&content.current_profile();}
void App::message(const char*s){notice=s;notice_ticks=60;}
void App::animate(int actor,int id,int delay){if(actor<0||actor>2)return;motion[actor]={id,animation+(uint32_t)mx(0,delay)};}
void App::reset_presentation(){detail_owned=-1;relic_flash_ticks=0;relic_flash_mask=0;seen_relic_revision=game.relic_revision;status_actor=-1;status_selected=0;deck_candidate=-1;pointer_cancel();choice_selected_count=0;choice_revision=game.revision;detail_card=detail_item=-1;reward_cards=false;cursor=target=panel=scroll=map_scroll=busy=card_fly=0;for(int i=0;i<3;i++){motion[i].clip=-1;last_hit[i]=hit_start[i]=0;}for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
void App::advance(uint32_t ms){if(ms>250)ms=250;tick_remainder+=ms;while(tick_remainder>=33){tick_remainder-=33;tick();}}
void App::draw_frame(){render();}
void App::tick(){animation++;if(relic_flash_ticks>0)--relic_flash_ticks;if(notice_ticks>0)--notice_ticks;if(feedback>0)--feedback;if(busy>0)--busy;if(card_fly>0)--card_fly;}
void App::card_description(Card c,int t,char*out,size_t cap)const{
    if(!cap)return;out[0]=0;if(c.kind>=CardKinds)return;TextBuffer b{out,cap,0};const char*p=content.card_string(c.kind,c.upgraded?2:1);const auto&d=definitions[c.kind];
    while(*p){
        if(*p=='\x1e'){++p;if(library||!combat_phase(game.phase)){while(*p&&*p!='\x1f')++p;}continue;}
        if(*p=='\x1f'){++p;continue;}
        if(p[0]=='{'&&p[1]&&p[2]=='}'){
            int value=0;switch(p[1]){case 's':value=d.creates+(c.upgraded?d.creates_up:0);break;case 'r':value=(c.kind==BouncingFlask||c.kind==SwordBoomerang)?(c.upgraded?4:3):1;break;case 'h':value=d.hp_loss;break;case 'e':value=d.energy_gain+(c.upgraded?d.energy_up:0);break;case 'd':value=(!library&&combat_phase(game.phase))?damage(game,c,t):d.damage+(c.upgraded?d.damage_up:0);break;case 'b':value=(!library&&combat_phase(game.phase))?card_block(game,c):d.block+(c.upgraded?d.block_up:0);break;case 'n':value=d.draw+(c.upgraded?d.draw_up:0);break;case 'p':value=d.power+(c.upgraded?d.power_up:0);break;default:b.ch(*p++);continue;}
            b.number(value);p+=3;
        }else b.ch(*p++);}
}
static int choice_count(const App&a){
    switch(a.game.phase){case Phase::Map:return 3;case Phase::Shop:return ShopLeave+1;case Phase::Event:return a.game.event_page>=2?1:2;
    case Phase::Reward:return 4;case Phase::Camp:return 2;case Phase::Upgrade:case Phase::Remove:case Phase::Transform:return a.game.deck_count;
    case Phase::Treasure:return a.game.treasure_state?2:1;case Phase::ReturnChoice:return a.game.discard.count;default:return a.game.hand.count;}
}
static void next_target(App&a){for(int i=1;i<=a.game.enemy_count;i++){int t=(a.target+i)%a.game.enemy_count;if(a.game.enemies[t].hp){a.target=t;break;}}}
void App::input(Key k){
    if(!pointer.dispatching){pointer_cancel();pointer.visible=false;}
    if(status_input(k))return;
    if(reference_input(k))return;
    if(detail_item>=0){
        if(k==Key::Back||k==Key::Confirm||k==Key::Detail){detail_item=-1;detail_owned=-1;}
        else if(k==Key::Up)detail_scroll=mx(0,detail_scroll-1);
        else if(k==Key::Down)++detail_scroll;
        return;
    }
    if(detail_card>=0){
        if(k==Key::Back||k==Key::Confirm||k==Key::Detail)detail_card=-1;
        else if(k==Key::Up)detail_scroll=mx(0,detail_scroll-1);
        else if(k==Key::Down)++detail_scroll;
        else if(k==Key::Left||k==Key::Right)detail_card^=256;
        return;
    }
    if(screen_input(k))return;
    if(selection_input(k))return;
    if(deck_input(k))return;
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel){
        if(game.phase==Phase::Reward&&reward_cards&&cursor<3){open_card_detail({game.reward[cursor],0});return;}
        if(game.phase==Phase::Treasure&&game.treasure_state&&game.treasure_relic){
            const auto*d=relic_definition(game.treasure_relic);
            if(d){detail_item=(int)(d-relic_definitions);detail_owned=-1;detail_scroll=0;}return;
        }
        if(game.phase==Phase::Shop){
            detail_owned=-1;detail_scroll=0;
            if(cursor<ShopCardCount)open_card_detail({game.shop_cards[cursor],0});
            else if(cursor<ShopPotionStart){for(int i=0;i<RelicKinds;i++)if(relic_definitions[i].id==game.shop_relics[cursor-ShopRelicStart])detail_item=i;}
            else if(cursor<ShopRemove)detail_item=RelicKinds+game.shop_potions[cursor-ShopPotionStart]-1;
            return;
        }
    }
    if(k==Key::Detail&&!title&&!paused&&!help&&!panel&&combat_phase(game.phase)&&game.hand.count){
        open_card_detail(game.cards[game.hand.ids[mn(cursor,game.hand.count-1)]]);return;
    }

    if(k==Key::Quit){quit=true;return;}
    if(help){if(k==Key::Back||k==Key::Confirm)help=false;return;}
    if(title){
        if(k==Key::Up||k==Key::Left)menu=(menu+6)%7;if(k==Key::Down||k==Key::Right)menu=(menu+1)%7;
        if(k==Key::Confirm){
            if(menu==0){practice_entry=false;character_select=true;}
            else if(menu==1){if(callbacks.load&&callbacks.load(game)){title=false;reset_presentation();message("已恢复存档");}else message("没有有效的存档");}
            else if(menu==2){library=true;library_page=library_selected=0;}
            else if(menu==3){practice_entry=true;character_select=true;}
            else if(menu==4){map_preview.open=true;if(!map_preview.generated)map_preview.generate();}
            else if(menu==5)help=true;else quit=true;
        }return;
    }
    if(paused){
        if(k==Key::Back){paused=false;return;}if(k==Key::Up||k==Key::Left)menu=(menu+5)%6;if(k==Key::Down||k==Key::Right)menu=(menu+1)%6;
        if(k==Key::Confirm){switch(menu){case 0:paused=false;break;
            case 1:has_save=callbacks.save&&callbacks.save(game);message(has_save?"已保存远征":"保存失败");paused=false;break;
            case 2:if(callbacks.load&&callbacks.load(game)){reset_presentation();message("已载入");}else message("没有有效的存档");paused=false;break;
            case 3:help=true;break;case 4:paused=false;title=true;menu=0;break;case 5:quit=true;break;}}return;
    }
    if(k==Key::Back){
        if(reward_cards&&!panel){reward_cards=false;cursor=1;return;}
        if(panel){panel=0;scroll=map_scroll=0;}
        else if(game.phase==Phase::Remove){game.phase=Phase::Shop;cursor=ShopRemove;}
        else if(game.phase==Phase::Upgrade){game.phase=Phase::Camp;cursor=1;}
        else{paused=true;menu=0;}return;
    }
    if(k==Key::Save){has_save=callbacks.save&&callbacks.save(game);message(has_save?"已保存远征":"保存失败");return;}
    if(k==Key::Load){if(callbacks.load&&callbacks.load(game)){reset_presentation();message("已载入");}else message("没有有效的存档");return;}
    if(k==Key::Log){panel=panel==2?0:2;scroll=0;return;}
    if(k==Key::Inspect){panel=panel==1?0:1;scroll=inspect_cursor=0;return;}
    if(k==Key::Map){if(game.practice){message("组合练习没有远征路线");return;}panel=panel==3?0:3;map_scroll=0;return;}
    if(k==Key::Relics){panel=panel==5?0:5;scroll=0;return;}
    if(k==Key::Potions){panel=panel==4?0:4;scroll=0;return;}
    if(panel){
        if(panel==5){
            if(k==Key::Left)scroll=mx(0,scroll-1);
            if(k==Key::Right)scroll=mn(mx(0,game.relic_count-1),scroll+1);
            if(k==Key::Up)scroll=mx(0,scroll-6);
            if(k==Key::Down)scroll=mn(mx(0,game.relic_count-1),scroll+6);
            if(k==Key::Confirm||k==Key::Detail)open_owned_relic(scroll);
            return;
        }
        if(panel==4){
            if(k==Key::Left||k==Key::Right)scroll=1-scroll;if(k==Key::Up||k==Key::Down)next_target(*this);
            if(k==Key::Confirm){if(use_potion(game,scroll,target)){panel=0;busy=8;next_target(*this);if(game.phase==Phase::Reward)cursor=0;message("药水已使用");}else message("此时无法使用，或药水槽为空");}return;
        }
        if(panel==3){if(k==Key::Up)map_scroll=mn(MapRows,map_scroll+1);if(k==Key::Down)map_scroll=mx(-MapRows,map_scroll-1);return;}
        if(panel==1){
            if(k==Key::Up)inspect_cursor=mx(0,inspect_cursor-1);
            if(k==Key::Down)++inspect_cursor;
            scroll=(inspect_cursor/10)*10;
            if(k==Key::Confirm||k==Key::Detail){
                render(); // Keyboard batches may arrive before the next timed frame.
                const HitRegion* pick=nullptr;int n=0;
                for(int i=0;i<hit_count;i++)if(hits[i].kind==HitKind::PileCard){if(n++==inspect_cursor%10){pick=&hits[i];break;}}
                if(pick)open_card_detail({(uint8_t)(pick->value&255),(uint8_t)(pick->value>>8)});
                return;
            }
        }else{if(k==Key::Up)scroll=mx(0,scroll-1);if(k==Key::Down)scroll++;}
        if(panel==1&&(k==Key::Left||k==Key::Right)){pile_tab=(pile_tab+(k==Key::Left?4:1))%5;scroll=inspect_cursor=0;}return;
    }
    if(game.phase==Phase::Complete||game.phase==Phase::Dead||game.phase==Phase::Error){if(k==Key::Confirm){title=true;menu=0;}return;}
    int count=choice_count(*this);if(k==Key::Left)cursor=count?(cursor+count-1)%count:0;if(k==Key::Right)cursor=count?(cursor+1)%count:0;
    if(k==Key::Up||k==Key::Down){
        if(game.phase==Phase::Event){scroll=mx(0,scroll+(k==Key::Up?-1:1));}
        else if(game.phase==Phase::Map){map_scroll=mx(-MapRows,mn(MapRows,map_scroll+(k==Key::Up?1:-1)));}
        else if(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform)cursor=(cursor+(k==Key::Up?count-1:1))%mx(1,count);
        else if(game.phase==Phase::Shop){
            const int below[15]={5,6,7,8,9,13,13,10,11,12,14,14,14,0,0};
            const int above[15]={13,13,14,14,14,0,1,2,3,4,7,8,9,5,10};
            cursor=(k==Key::Down?below:above)[cursor];
        }
        else if(game.phase==Phase::Camp)cursor=1-cursor;
        else if(!library&&combat_phase(game.phase))next_target(*this);
    }
    if(busy&&(k==Key::Confirm||k==Key::EndTurn))return;
    Phase before=game.phase;bool acted=false;int hp_before[3]={game.hp,game.enemies[0].hp,game.enemies[1].hp};
    if(k==Key::EndTurn&&game.phase==Phase::Combat){
        uint8_t moves[2]={game.enemies[0].move,game.enemies[1].move};acted=end_turn(game);
        if(acted){for(int i=0;i<game.enemy_count;i++)if(hp_before[i+1])animate(i+1,moves[i]==2?AnimEnemyBuff:AnimEnemyAttack,i*12);animate(0,AnimHeroHurt,7);busy=18+10*(game.enemy_count-1);feedback=10;}
    }
    if(k==Key::Confirm){
        switch(game.phase){
        case Phase::Combat:{
            Card card{};if(cursor<game.hand.count)card=game.cards[game.hand.ids[cursor]];
            acted=play(game,cursor,target);
            if(!acted)message("能量不足，或没有可打出的牌");else{animate(0,card.kind==Shiv?AnimSilentShiv:definitions[card.kind].type==CardType::Attack?AnimHeroAttack:AnimHeroCast);fly_card=card.kind;card_fly=12;busy=12;feedback=8;}break;}
        case Phase::ExhaustChoice:acted=choose_exhaust(game,cursor);break;
        case Phase::Map:if(map_scroll){map_scroll=0;message("已返回可选楼层，再次确认进入");}else{acted=choose_node(game,cursor);if(!acted)message("这条路线不相连，请选择亮起的节点");}break;
        case Phase::Shop:acted=cursor==ShopLeave?leave_shop(game):buy(game,cursor);if(!acted)message("金币不足、已售出，或物品槽已满");else if(game.phase==Phase::Shop)message("购买成功");break;
        case Phase::Remove:acted=remove_card(game,cursor);if(acted)message("已移除所选卡牌");break;
        case Phase::Event:acted=event_choice(game,cursor);scroll=0;break;
        case Phase::Transform:acted=transform_card(game,cursor);if(!acted)message("请选择另一张尚未变化的牌");break;
        case Phase::Treasure:
            if(!game.treasure_state)acted=open_treasure(game);
            else if(cursor==0&&game.treasure_state==1){acted=take_treasure(game);if(acted)cursor=1;}
            else if(cursor==1)acted=leave_treasure(game);
            break;
        case Phase::Reward:acted=choose_reward(game,cursor==3?-1:cursor);if(acted){reward_cards=false;cursor=1;}break;
        case Phase::Camp:acted=camp(game,cursor==1);if(!acted)message("没有可以升级的牌");break;
        case Phase::Upgrade:acted=upgrade_card(game,cursor);if(!acted)message("这张牌已经升级");break;
        default:break;
        }
    }
    if(acted){
        for(int i=0;i<3;i++){int hp=i?game.enemies[i-1].hp:game.hp;int hit=hp_before[i]-hp;if(hit>0){last_hit[i]=hit;hit_start[i]=(int)animation+6;if(i)animate(i,AnimEnemyHurt,6);}}
        if(game.phase!=before){reward_cards=false;cursor=0;scroll=map_scroll=0;if(game.phase==Phase::Map){for(int i=0;i<3;i++)if(reachable(game,game.next_row*3+i)){cursor=i;break;}}}
        cursor=mn(cursor,mx(0,choice_count(*this)-1));if(target>=game.enemy_count||game.enemies[target].hp==0){target=0;for(int i=0;i<game.enemy_count;i++)if(game.enemies[i].hp){target=i;break;}}
    }
}
