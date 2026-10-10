#include "app.hpp"
#include "status.hpp"
namespace spire {
unsigned App::library_items(uint8_t* out)const{
    unsigned count=0;uint8_t ids[CardKinds];
    for(unsigned i=0;i<CardKinds;i++){
        CardColor color=card_color(i);
        if(library_filter&&color!=(library_filter==1?CardColor::Ironclad:library_filter==2?CardColor::Silent:CardColor::Colorless))continue;
        ids[count++]=(uint8_t)i;
    }
    if(library_sort)for(unsigned i=1;i<count;i++){
        uint8_t id=ids[i];unsigned j=i;
        auto order=[&](unsigned k){return library_sort==1?cost({(uint8_t)k,(uint8_t)library_upgraded}):(int)definitions[k].type;};
        while(j&&order(ids[j-1])>order(id)){ids[j]=ids[j-1];--j;}ids[j]=id;
    }
    if(out)for(unsigned i=0;i<count;i++)out[i]=ids[i];
    return count;
}
void App::sync_selection(){
    if(choice_revision!=game.revision){choice_selected_count=0;choice_revision=game.revision;}
}
void App::toggle_choice(int id){
    sync_selection();if(!choice_phase(game.phase)||id<0||id>=game.card_count)return;
    const auto& source=choice_pile(game);
    bool in_hand=false;for(unsigned i=0;i<source.count;i++)if(source.ids[i]==id)in_hand=true;
    if(!in_hand)return;
    for(unsigned i=0;i<choice_selected_count;i++)if(choice_selected[i]==id){
        for(unsigned j=i+1;j<choice_selected_count;j++)choice_selected[j-1]=choice_selected[j];--choice_selected_count;return;
    }
    unsigned needed=choice_max(game);
    if(needed==1)choice_selected_count=0;
    if(choice_selected_count<needed)choice_selected[choice_selected_count++]=(uint16_t)id;
}
void App::confirm_choice(){
    sync_selection();if(!choice_phase(game.phase))return;
    unsigned needed=choice_max(game);
    if(choice_selected_count<choice_min(game)||choice_selected_count>needed)return;
    bool done=false;
    if(game.phase==Phase::ReturnChoice)done=choose_return_to_draw(game,choice_selected[0]);
    else if(game.phase==Phase::ExhaustManyChoice)done=choose_exhaust_many(game,choice_selected,choice_selected_count);
    else if(game.phase==Phase::DiscardChoice)done=choose_discard(game,choice_selected,needed);
    else for(int i=0;i<game.hand.count;i++)if(game.hand.ids[i]==choice_selected[0]){done=choose_exhaust(game,i);break;}
    if(done){choice_selected_count=0;choice_revision=game.revision;cursor=0;busy=0;pointer_cancel();}
}
bool App::selection_input(Key key){
    if(title||paused||help||panel||!choice_phase(game.phase))return false;
    sync_selection();const auto& source=choice_pile(game);int count=source.count;if(!count)return false;
    if(cursor<0)cursor=0;if(cursor>=count)cursor=count-1;
    if(key==Key::Left)cursor=(cursor+count-1)%count;
    else if(key==Key::Right)cursor=(cursor+1)%count;
    else if(key==Key::Up)cursor=cursor>=5?cursor-5:cursor;
    else if(key==Key::Down)cursor=cursor+5<count?cursor+5:cursor;
    else if(key==Key::Confirm)toggle_choice(source.ids[cursor]);
    else if(key==Key::EndTurn)confirm_choice();
    else if(key==Key::Detail)open_card_detail(game.cards[source.ids[cursor]]);
    else return false; // Pause, save/load and browsing remain available.
    return true;
}
void App::select_library_item(int index){
    int count=library_tab==0?(int)library_items():library_tab==1?RelicKinds:PotionKinds;
    if(!library||index<0||index>=count)return;
    library_selected=index;
    if(library_tab==0){uint8_t ids[CardKinds];library_items(ids);open_card_detail({ids[index],(uint8_t)library_upgraded});}
    else{detail_item=library_tab==1?index:RelicKinds+index;detail_scroll=0;pointer_cancel();}
}
void App::ui_action(UiAction action){
    pointer_cancel();
    switch(action){
    case UiAction::StartCharacter:
        if(character_select){new_run(game,0x20261003u+run_number++*7919u,practice_entry,selected_character);character_select=false;title=false;reset_presentation();}break;
    case UiAction::LibraryCards:case UiAction::LibraryRelics:case UiAction::LibraryPotions:
        if(library){library_tab=(int)action-(int)UiAction::LibraryCards;library_page=library_selected=0;}break;
    case UiAction::PreviousPage:case UiAction::NextPage:
        if(library){int per=library_tab==0?8:18,count=library_tab==0?(int)library_items():library_tab==1?RelicKinds:PotionKinds;
            int last=(count-1)/per;library_page+=action==UiAction::PreviousPage?-1:1;
            if(library_page<0)library_page=0;if(library_page>last)library_page=last;
            library_selected=library_page*per;
        }break;
    case UiAction::ToggleUpgrades:if(library&&library_tab==0)library_upgraded=!library_upgraded;break;
    case UiAction::ToggleCardUpgrade:if(detail_card>=0)detail_card^=256;break;
    case UiAction::DiscardPotion:
        if(panel==4&&discard_potion(game,scroll))message("已丢弃药水");break;
    case UiAction::TargetFirst:if(panel==4&&game.enemy_count&&game.enemies[0].hp>0)target=0;break;
    case UiAction::TargetSecond:if(panel==4&&game.enemy_count>1&&game.enemies[1].hp>0)target=1;break;
    case UiAction::RewardCards:
        if(game.phase==Phase::Reward&&!(game.reward_claimed&2)){reward_cards=true;cursor=0;}break;
    case UiAction::RewardGold:
        if(game.phase==Phase::Reward){claim_reward_gold(game);}break;
    case UiAction::RewardPotion:
        if(game.phase==Phase::Reward&&!claim_reward_potion(game))message("药水栏已满，可先打开药水栏丢弃一瓶");break;
    case UiAction::LeaveRewards:
        if(game.phase==Phase::Reward){leave_rewards(game);reward_cards=false;cursor=0;}break;
    case UiAction::SelectIronclad:case UiAction::SelectSilent:
        if(character_select)selected_character=action==UiAction::SelectIronclad?Character::Ironclad:Character::Silent;break;
    case UiAction::FilterAll:case UiAction::FilterIronclad:case UiAction::FilterSilent:case UiAction::FilterColorless:
        if(library&&library_tab==0){library_filter=(int)action-(int)UiAction::FilterAll;library_page=library_selected=0;}break;
    case UiAction::CycleSort:if(library&&library_tab==0){library_sort=(library_sort+1)%3;library_page=library_selected=0;}break;
    case UiAction::PreviousChoicePage:case UiAction::NextChoicePage:
        if(!title&&!paused&&!help&&!panel&&choice_phase(game.phase)){
            int page=cursor/10+(action==UiAction::PreviousChoicePage?-1:1);
            int last=(choice_pile(game).count-1)/10;
            if(page<0)page=0;if(page>last)page=last;cursor=page*10;
        }break;
    case UiAction::ConfirmChoice:confirm_choice();break;
    case UiAction::ClearChoice:choice_selected_count=0;break;
    case UiAction::ConfirmDeckChoice:confirm_deck_choice();break;
    case UiAction::CancelDeckChoice:deck_candidate=-1;break;
    case UiAction::PreviousDeckPage:case UiAction::NextDeckPage:
        if(!title&&!paused&&!panel&&deck_candidate<0&&(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform)){
            int page=cursor/10+(action==UiAction::PreviousDeckPage?-1:1);
            if(page<0)page=0;int last=(game.deck_count-1)/10;if(page>last)page=last;cursor=page*10;
        }break;
    case UiAction::Back:pointer_key(Key::Back);break;
    }
}
bool App::screen_input(Key key){
    if(key==Key::Collection){library=!library;library_page=library_selected=0;pointer_cancel();return true;}
    if(library){
        int per=library_tab==0?8:18,count=library_tab==0?(int)library_items():library_tab==1?RelicKinds:PotionKinds,columns=library_tab==0?4:6;
        if(key==Key::Back){library=false;return true;}
        if(key==Key::Quit){quit=true;return true;}
        if(key==Key::Inspect){library_tab=(library_tab+1)%3;library_page=library_selected=0;return true;}
        if(key==Key::Up)library_selected-=columns;
        if(key==Key::Down)library_selected+=columns;
        if(key==Key::Left)--library_selected;
        if(key==Key::Right)++library_selected;
        if(library_selected<0)library_selected=0;if(library_selected>=count)library_selected=count-1;
        library_page=library_selected/per;
        if(key==Key::Confirm||key==Key::Detail)select_library_item(library_selected);
        return true;
    }
    if(character_select){
        if(key==Key::Confirm)ui_action(UiAction::StartCharacter);
        else if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down)selected_character=selected_character==Character::Ironclad?Character::Silent:Character::Ironclad;
        else if(key==Key::Back)character_select=false;
        else if(key==Key::Quit)quit=true;
        return true;
    }
    if(!title&&!paused&&!help&&!panel&&game.phase==Phase::Reward&&!reward_cards){
        if(key==Key::Left||key==Key::Up){cursor=(cursor+3)%4;return true;}
        if(key==Key::Right||key==Key::Down){cursor=(cursor+1)%4;return true;}
        if(key==Key::Confirm){const UiAction actions[]={UiAction::RewardGold,UiAction::RewardCards,UiAction::RewardPotion,UiAction::LeaveRewards};ui_action(actions[cursor%4]);return true;}
    }
    return false;
}
}

namespace spire {
static bool deck_phase(Phase phase){return phase==Phase::Upgrade||phase==Phase::Remove||phase==Phase::Transform;}
bool App::deck_candidate_valid()const{
    if(deck_candidate<0||deck_candidate>=game.deck_count||deck_choice_revision!=game.revision||deck_choice_phase!=game.phase||!deck_phase(game.phase))return false;
    if(game.phase==Phase::Upgrade&&game.deck[deck_candidate].upgraded)return false;
    if(game.phase==Phase::Transform&&deck_candidate==game.event_first)return false;
    return true;
}
void App::confirm_deck_choice(){
    if(!deck_candidate_valid()){deck_candidate=-1;return;}
    int index=deck_candidate;Phase phase=game.phase;deck_candidate=-1;pointer_cancel();
    bool ok=phase==Phase::Upgrade?upgrade_card(game,index):phase==Phase::Remove?remove_card(game,index):transform_card(game,index);
    if(ok){reset_presentation();if(game.phase==Phase::Shop)cursor=ShopRemove;}
    else message("此时无法完成这项操作");
}
bool App::deck_input(Key key){
    if(title||paused||help||panel||library||!deck_phase(game.phase))return false;
    if(deck_candidate>=0){
        if(!deck_candidate_valid()){deck_candidate=-1;return true;}
        if(key==Key::Back){deck_candidate=-1;return true;}
        if(key==Key::Confirm){confirm_deck_choice();return true;}
        // The irreversible-action confirmation consumes navigation/game actions.
        // Save and load remain available; loading resets its captured revision.
        return key!=Key::Save&&key!=Key::Load&&key!=Key::Quit;
    }
    int count=game.deck_count;if(!count)return true;
    if(cursor<0)cursor=0;if(cursor>=count)cursor=count-1;
    if(key==Key::Left)cursor=cursor>0?cursor-1:0;
    else if(key==Key::Right)cursor=cursor+1<count?cursor+1:count-1;
    else if(key==Key::Up)cursor=cursor>=5?cursor-5:cursor;
    else if(key==Key::Down)cursor=cursor+5<count?cursor+5:count-1;
    else if(key==Key::Detail)open_card_detail(game.deck[cursor]);
    else if(key==Key::Confirm){
        if(game.phase==Phase::Upgrade&&game.deck[cursor].upgraded){message("这张牌已经升级");return true;}
        if(game.phase==Phase::Transform&&cursor==game.event_first){message("请选择另一张牌");return true;}
        deck_candidate=cursor;deck_choice_revision=game.revision;deck_choice_phase=game.phase;pointer_cancel();
    }else return false;
    return true;
}
}

namespace spire {
void App::open_statuses(int actor){
    if(title||paused||help||panel||library||character_select||map_preview.open||!combat_phase(game.phase))return;
    if(actor<0||actor>game.enemy_count)return;
    status_actor=actor;status_selected=0;pointer_cancel();
}
bool App::status_input(Key key){
    if(status_actor<0){if(key!=Key::Statuses)return false;open_statuses(0);return true;}
    if(key==Key::Back||key==Key::Statuses){status_actor=-1;return true;}
    if(key==Key::Quit){quit=true;return true;}
    if(key==Key::Left||key==Key::Right){
        int count=game.enemy_count+1;
        status_actor=(status_actor+(key==Key::Left?count-1:1))%count;status_selected=0;
    }
    int count=collect_statuses(game,status_actor,nullptr);
    if(key==Key::Up&&status_selected>0)--status_selected;
    if(key==Key::Down&&status_selected+1<count)++status_selected;
    return true;
}
}
