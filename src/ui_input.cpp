#include "app.hpp"
#include "status.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "ui_core_observer.hpp"
namespace spire {
#include "world_choice_input.inc"
unsigned App::library_items(CardKind* out)const{
    unsigned count=0;CardKind ids[CardKinds];
    const CardColor filters[]={CardColor::Token,CardColor::Ironclad,CardColor::Silent,CardColor::Colorless,
        CardColor::Defect,CardColor::Regent,CardColor::Necrobinder};
    for(unsigned i=0;i<CardKinds;i++){
        CardColor color=card_color(i);
        if(library_filter>0&&library_filter<7&&color!=filters[library_filter])continue;
        ids[count++]=(CardKind)i;
    }
    if(library_sort)for(unsigned i=1;i<count;i++){
        CardKind id=ids[i];unsigned j=i;
        auto order=[&](CardKind k){return library_sort==1?cost({k,(uint8_t)(library_upgraded&&card_upgradeable({k,0}))}):(int)definitions[k].type;};
        while(j&&order(ids[j-1])>order(id)){ids[j]=ids[j-1];--j;}ids[j]=id;
    }
    if(out)for(unsigned i=0;i<count;i++)out[i]=ids[i];
    return count;
}
void App::sync_selection(){
    if(choice_revision!=game.revision){choice_selected_count=0;choice_revision=game.revision;}
}
void App::toggle_choice(int id){
    sync_selection();if(!choice_phase(game.phase)||id<0||id>=MaxCards+3)return;
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
    if(game.phase==Phase::ExtendedChoice)done=choose_extended(game,choice_selected,choice_selected_count);
    else if(game.phase==Phase::ReturnChoice)done=choose_return_to_draw(game,choice_selected[0]);
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
    else if(key==Key::Detail)open_choice_detail(source.ids[cursor]);
    else return false; // Pause, save/load and browsing remain available.
    return true;
}
void App::select_library_item(int index){
    int count=library_tab==0?(int)library_items():library_tab==1?WorldRelicKinds:PotionKinds;
    if(!library||index<0||index>=count)return;
    library_selected=index;
    if(library_tab==0){CardKind ids[CardKinds];library_items(ids);open_card_detail({ids[index],(uint8_t)(library_upgraded&&card_upgradeable({ids[index],0}))});}
    else{detail_owned=-1;detail_item=library_tab==1?index:WorldRelicKinds+index;detail_scroll=0;pointer_cancel();}
}
void App::ui_action(UiAction action){
    if(map_transition_blocks_pointer())return;
    UiCoreObserver observer(*this);
    pointer_cancel();
    // Opaque frontend modals own all actions until explicitly dismissed.
    if(frontend_confirm&&action!=UiAction::FrontendAccept&&action!=UiAction::FrontendCancel&&action!=UiAction::Back)return;
    if(settings_open&&action!=UiAction::SettingsFastMode&&action!=UiAction::SettingsControls&&action!=UiAction::SettingsPractice&&action!=UiAction::SettingsReference&&action!=UiAction::SettingsBack&&action!=UiAction::Back)return;
    switch(action){
    case UiAction::FrontendAccept:accept_frontend_confirm();break;
    case UiAction::FrontendCancel:frontend_confirm=frontend_choice=0;break;
    case UiAction::SettingsFastMode:if(settings_open)fast_mode=!fast_mode;break;
    case UiAction::SettingsControls:if(settings_open)help=true;break;
    case UiAction::SettingsPractice:if(settings_open&&title){settings_open=false;practice_entry=true;character_select=true;}break;
    case UiAction::SettingsReference:if(settings_open&&title){settings_open=false;map_preview.open=true;if(!map_preview.generated)map_preview.generate();}break;
    case UiAction::SettingsBack:settings_open=false;break;
    case UiAction::LabCard:{
        const bool overflow=game.hand.count>=MaxHand;
        if(library&&!title&&!paused&&!help&&!panel&&detail_card>=0&&lab_card(game,card_from_token(detail_card))){
            detail_card=-1;library=false;reset_presentation();
            message(overflow?"手牌已满，卡牌已加入弃牌堆":"卡牌已加入手牌");
        }break;}
    case UiAction::LabPotion:
        if(library&&!title&&!paused&&!help&&!panel&&detail_item>=WorldRelicKinds&&lab_potion(game,(Potion)(detail_item-WorldRelicKinds+1))){
            detail_item=-1;library=false;reset_presentation();
            message("已获得药水");
        }break;
    case UiAction::OpenShop:
        if(!title&&!paused&&!panel&&game.phase==Phase::Shop){shop_open=true;cursor=0;}break;
    case UiAction::CloseShop:
        if(game.phase==Phase::Shop){shop_open=false;cursor=0;}break;
    case UiAction::LeaveShop:
        if(!shop_open&&game.phase==Phase::Shop){leave_shop(game);reset_presentation(true);}break;
    case UiAction::GameOverSummary:
        if(game_over_available()&&!game_over_summary){game_over_summary=true;pointer_cancel();}break;
    case UiAction::GameOverMainMenu:
        if(game_over_available()&&game_over_summary){if(finish_saved_run())return_to_title();else message("无法删除存档，请重试");}break;
    case UiAction::LeaveCamp:
        camp_input(Key::EndTurn);break;
    case UiAction::TreasureOpen:case UiAction::TreasureTake:case UiAction::TreasureLeave:
        treasure_action(action);break;
    case UiAction::PreviousCampPage:camp_page(-1);break;
    case UiAction::NextCampPage:camp_page(1);break;
    case UiAction::ReadEvent:
        if(!title&&!paused&&!panel&&!library&&game.phase==Phase::Event){event_reader=true;event_reader_scroll=0;}break;
    case UiAction::CloseEventReader:event_reader=false;break;
    case UiAction::StartCharacter:
        if(character_select){
            forget_run_checkpoint();
            new_run(game,0x20261003u+run_number++*7919u,practice_entry,selected_character);
            if(!practice_entry){if(*selected_seed&&!world_set_seed(game,selected_seed)){message("种子无效");break;}if(!world_begin_run(game,selected_region,selected_ascension)){message("无法开始远征");break;}}
            character_select=run_settings=false;title=false;reset_presentation();
            if(game.world.active){map_visuals.bind(game,map_clock_ms);if(const auto*m=world_current_map(game))map_visuals.initial_marker(m->root,map_clock_ms);}
        }break;
    case UiAction::RunSettings:if(character_select&&!practice_entry)run_settings=!run_settings;break;
    case UiAction::RunAscensionDown:if(character_select&&selected_ascension)selected_ascension--;break;
    case UiAction::RunAscensionUp:if(character_select&&selected_ascension<10)selected_ascension++;break;
    case UiAction::RunRegion:if(character_select)selected_region=selected_region==reference::ActKind::Overgrowth?reference::ActKind::Underdocks:reference::ActKind::Overgrowth;break;
    case UiAction::RunSeed:
        if(character_select){map_preview.open=true;run_seed_editing=true;map_preview.begin_edit();unsigned n=0;do{map_preview.draft[n]=selected_seed[n];}while(selected_seed[n++]);}break;
    case UiAction::RunSeedClear:if(character_select)selected_seed[0]=0;break;
    case UiAction::ToggleMapLegend:
        if(!title&&!paused&&!help&&!library&&game.world.active&&(panel==3||(!panel&&game.phase==Phase::Map)))map_legend=!map_legend;break;
    case UiAction::MapLegendUnknown:case UiAction::MapLegendShop:case UiAction::MapLegendTreasure:
    case UiAction::MapLegendRest:case UiAction::MapLegendMonster:case UiAction::MapLegendElite:
        if(map_legend&&game.world.active&&(panel==3||(!panel&&game.phase==Phase::Map)))map_legend_item=uint8_t(int(action)-int(UiAction::MapLegendUnknown));break;
    case UiAction::LibraryCards:case UiAction::LibraryRelics:case UiAction::LibraryPotions:
        if(library){library_tab=(int)action-(int)UiAction::LibraryCards;library_page=library_selected=0;}break;
    case UiAction::PreviousPage:case UiAction::NextPage:
        if(library){int per=library_tab==0?8:18,count=library_tab==0?(int)library_items():library_tab==1?WorldRelicKinds:PotionKinds;
            int last=(count-1)/per;library_page+=action==UiAction::PreviousPage?-1:1;
            if(library_page<0)library_page=0;if(library_page>last)library_page=last;
            library_selected=library_page*per;
        }break;
    case UiAction::ToggleUpgrades:if(library&&library_tab==0)library_upgraded=!library_upgraded;break;
    case UiAction::ToggleCardUpgrade:
        if(detail_card>=0){Card c=card_from_token(detail_card);if(c.upgraded||card_upgradeable(c)){c.upgraded=!c.upgraded;detail_card=card_token(c);}}break;
    case UiAction::DiscardPotion:
        if(panel==4&&!original_event_blocks_potions(game)&&discard_potion(game,scroll))message("已丢弃药水");break;
    case UiAction::TargetFirst:if(panel==4&&game.enemy_count&&game.enemies[0].hp>0)target=0;break;
    case UiAction::TargetSecond:if(panel==4&&game.enemy_count>1&&game.enemies[1].hp>0)target=1;break;
    case UiAction::RewardCards:
        if(game.phase==Phase::Reward&&!(game.reward_claimed&2)){reward_cards=true;cursor=0;}break;
    case UiAction::RewardGold:
        if(game.phase==Phase::Reward){claim_reward_gold(game);}break;
    case UiAction::RewardPotion:
        if(game.phase==Phase::Reward&&!claim_reward_potion(game))message("药水栏已满，可先打开药水栏丢弃一瓶");break;
    case UiAction::LeaveRewards:
        if(game.phase==Phase::Reward){leave_rewards(game);reward_cards=grimoire_select=false;cursor=0;}break;
    case UiAction::RewardGrimoire:
        if(!title&&!paused&&!help&&!panel&&game.phase==Phase::Reward&&game.necro.grimoire_pending){grimoire_select=true;reward_cards=false;cursor=0;}break;
    case UiAction::SkipGrimoire:
        if(!title&&!paused&&!help&&!panel&&grimoire_select&&game.phase==Phase::Reward&&necro_grimoire_choice(game,-1)){
            deck_candidate=-1;if(!game.necro.grimoire_pending){grimoire_select=false;cursor=3;}
        }break;
    case UiAction::ShowStars:case UiAction::ShowOrbs:case UiAction::ShowOsty:{
        open_statuses(0);if(status_actor<0)break;
        const int wanted=action==UiAction::ShowStars?44:action==UiAction::ShowOsty?45:46;
        int ids[StatusKinds],n=collect_statuses(game,0,ids);for(int i=0;i<n;i++)if(ids[i]==wanted){status_selected=i;break;}
        break;}
    case UiAction::MoreStatus:
        if(status_actor>=0)status_scroll=status_scroll>=status_scroll_limit?0:status_scroll+5;break;
    case UiAction::SelectIronclad:case UiAction::SelectSilent:case UiAction::SelectDefect:case UiAction::SelectRegent:case UiAction::SelectNecrobinder:
        if(character_select)selected_character=action==UiAction::SelectIronclad?Character::Ironclad:action==UiAction::SelectSilent?Character::Silent:
            action==UiAction::SelectDefect?Character::Defect:action==UiAction::SelectRegent?Character::Regent:Character::Necrobinder;break;
    case UiAction::FilterAll:case UiAction::FilterIronclad:case UiAction::FilterSilent:case UiAction::FilterColorless:
        if(library&&library_tab==0){library_filter=(int)action-(int)UiAction::FilterAll;library_page=library_selected=0;}break;
    case UiAction::FilterDefect:case UiAction::FilterRegent:case UiAction::FilterNecrobinder:
        if(library&&library_tab==0){library_filter=4+(int)action-(int)UiAction::FilterDefect;library_page=library_selected=0;}break;
    case UiAction::CycleSort:if(library&&library_tab==0){library_sort=(library_sort+1)%3;library_page=library_selected=0;}break;
    case UiAction::PreviousChoicePage:case UiAction::NextChoicePage:
        if(!title&&!paused&&!help&&!panel&&choice_phase(game.phase)){
            int page=cursor/10+(action==UiAction::PreviousChoicePage?-1:1);
            int last=(choice_pile(game).count-1)/10;
            if(page<0)page=0;if(page>last)page=last;cursor=page*10;
        }break;
    case UiAction::ConfirmChoice:confirm_choice();break;
    case UiAction::ConfirmWorldChoice:confirm_world_choice();break;
    case UiAction::LeaveWorldRewards:{
        const bool rewards=game.phase==Phase::WorldRelicChoice?game.world_relics.choice==WorldRelicChoice::OfferReward:game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Rewards;
        if(!rewards||world_choice_min())break;
        const bool terminal=game.world.active&&game.phase==Phase::WorldRelicChoice&&game.world_relics.return_phase==uint8_t(Phase::Reward);
        choice_selected_count=0;confirm_world_choice();
        // A terminal combat set's Skip is the room's Proceed action. Nested
        // event/relic sets resume their parent and are never closed here.
        if(terminal&&game.phase==Phase::Reward&&!world_relic_pending(game)&&ui_reward_actions(game,nullptr)==1){leave_rewards(game);reset_presentation(true);}
        break;}
    case UiAction::ClearWorldChoice:choice_selected_count=0;break;
    case UiAction::PreviousWorldPage:case UiAction::NextWorldPage:{
        const unsigned per=world_choice_page_size();
        unsigned count=world_choice_options(nullptr,0);int page=cursor/int(per)+(action==UiAction::PreviousWorldPage?-1:1);
        if(page<0)page=0;if(page>int((count?count-1:0)/per))page=int((count?count-1:0)/per);cursor=page*int(per);break;}
    case UiAction::RewardRelic:if(game.phase==Phase::Reward)world_claim_reward_relic(game);break;
    case UiAction::SacrificeReward:
        if(world_take_card_reward_alternative(game,1)){reward_cards=false;choice_selected_count=0;choice_revision=game.revision;cursor=0;pointer_cancel();}break;
    case UiAction::RerollReward:
        if(world_take_card_reward_alternative(game,2)){choice_selected_count=0;choice_revision=game.revision;cursor=0;pointer_cancel();}break;
    case UiAction::CrystalSmall:original_event_crystal_tool(game,false);break;
    case UiAction::CrystalLarge:original_event_crystal_tool(game,true);break;
    case UiAction::ClearChoice:choice_selected_count=0;break;
    case UiAction::ConfirmDeckChoice:confirm_deck_choice();break;
    case UiAction::CancelDeckChoice:deck_candidate=-1;break;
    case UiAction::PreviousDeckPage:case UiAction::NextDeckPage:
        if(!title&&!paused&&!panel&&deck_candidate<0&&(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform||
            (grimoire_select&&game.phase==Phase::Reward&&game.necro.grimoire_pending))){
            int page=cursor/10+(action==UiAction::PreviousDeckPage?-1:1);
            if(page<0)page=0;int last=(game.deck_count-1)/10;if(page>last)page=last;cursor=page*10;
        }break;
    case UiAction::Back:pointer_key(Key::Back);break;
    }
}
bool App::screen_input(Key key){
    if(key==Key::Collection){library=!library;library_page=library_selected=0;pointer_cancel();return true;}
    if(library){
        int per=library_tab==0?8:18,count=library_tab==0?(int)library_items():library_tab==1?WorldRelicKinds:PotionKinds,columns=library_tab==0?4:6;
        if(key==Key::Back){library=false;return true;}
        if(key==Key::Quit){cancel_map_travel();quit=true;return true;}
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
        if(run_settings){
            if(key==Key::Back||key==Key::EndTurn)run_settings=false;
            else if(key==Key::Left)ui_action(UiAction::RunAscensionDown);
            else if(key==Key::Right)ui_action(UiAction::RunAscensionUp);
            else if(key==Key::Up||key==Key::Down)ui_action(UiAction::RunRegion);
            else if(key==Key::Confirm)ui_action(UiAction::RunSeed);
            return true;
        }
        if(key==Key::Confirm)ui_action(UiAction::StartCharacter);
        else if(key==Key::EndTurn&&!practice_entry)run_settings=true;
        else if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down)
            selected_character=(Character)(((int)selected_character+(key==Key::Left||key==Key::Up?CharacterKinds-1:1))%CharacterKinds);
        else if(key==Key::Back)character_select=false;
        else if(key==Key::Quit){cancel_map_travel();quit=true;}
        return true;
    }
    if(!title&&!paused&&!help&&!panel&&game.phase==Phase::Reward&&!reward_cards&&!grimoire_select){
        UiAction actions[6];const int count=ui_reward_actions(game,actions);
        if(key==Key::Left||key==Key::Up){cursor=(cursor+count-1)%count;return true;}
        if(key==Key::Right||key==Key::Down){cursor=(cursor+1)%count;return true;}
        if(key==Key::Confirm){ui_action(actions[cursor%count]);return true;}
    }
    return false;
}
}

namespace spire {
static bool deck_phase(Phase phase){return phase==Phase::Upgrade||phase==Phase::Remove||phase==Phase::Transform;}
bool App::deck_candidate_valid()const{
    const bool grimoire=grimoire_select&&game.phase==Phase::Reward&&game.necro.grimoire_pending;
    if(deck_candidate<0||deck_candidate>=game.deck_count||deck_choice_revision!=game.revision||deck_choice_phase!=game.phase||(!deck_phase(game.phase)&&!grimoire))return false;
    Card c=game.deck[deck_candidate];
    if(game.phase==Phase::Upgrade&&!card_upgradeable(c))return false;
    if(game.phase==Phase::Transform&&(deck_candidate==game.event_first||!card_transformable(c)))return false;
    if((grimoire||game.phase==Phase::Remove)&&(!card_removable(c)||game.deck_count<=1))return false;
    return true;
}
void App::confirm_deck_choice(){
    if(!deck_candidate_valid()){deck_candidate=-1;return;}
    int index=deck_candidate;Phase phase=game.phase;deck_candidate=-1;pointer_cancel();
    if(grimoire_select&&phase==Phase::Reward){
        if(necro_grimoire_choice(game,index)){cursor=index<game.deck_count?index:game.deck_count-1;if(!game.necro.grimoire_pending){grimoire_select=false;cursor=3;}}
        else message("此时无法移除这张牌");return;
    }
    bool ok=phase==Phase::Upgrade?upgrade_card(game,index):phase==Phase::Remove?remove_card(game,index):transform_card(game,index);
    if(ok){reset_presentation(true);if(game.phase==Phase::Shop){cursor=ShopRemove;shop_open=true;}}
    else message("此时无法完成这项操作");
}
bool App::deck_input(Key key){
    if(grimoire_select&&(game.phase!=Phase::Reward||!game.necro.grimoire_pending))grimoire_select=false;
    if(title||paused||help||panel||library||(!deck_phase(game.phase)&&!grimoire_select))return false;
    if(deck_candidate>=0){
        if(!deck_candidate_valid()){deck_candidate=-1;return true;}
        if(key==Key::Back){deck_candidate=-1;return true;}
        if(key==Key::Confirm){confirm_deck_choice();return true;}
        // The irreversible-action confirmation consumes navigation/game actions.
        // Save and load remain available; loading resets its captured revision.
        return key!=Key::Save&&key!=Key::Load&&key!=Key::Quit;
    }
    if(grimoire_select&&key==Key::Back){grimoire_select=false;cursor=3;return true;}
    if(grimoire_select&&key==Key::EndTurn){ui_action(UiAction::SkipGrimoire);return true;}
    if(key==Key::Back&&game.event_deck_action!=EventDeckAction::None){
        cancel_event_selection(game);reset_presentation(true);return true;
    }
    int count=game.deck_count;if(!count)return true;
    if(cursor<0)cursor=0;if(cursor>=count)cursor=count-1;
    if(key==Key::Left)cursor=cursor>0?cursor-1:0;
    else if(key==Key::Right)cursor=cursor+1<count?cursor+1:count-1;
    else if(key==Key::Up)cursor=cursor>=5?cursor-5:cursor;
    else if(key==Key::Down)cursor=cursor+5<count?cursor+5:count-1;
    else if(key==Key::Detail)open_card_detail(game.deck[cursor],cursor);
    else if(key==Key::Confirm){
        if(game.phase==Phase::Upgrade&&!card_upgradeable(game.deck[cursor])){message("这张牌不能继续升级");return true;}
        if(game.phase==Phase::Transform&&(cursor==game.event_first||!card_transformable(game.deck[cursor]))){message("请选择另一张可变化的牌");return true;}
        if((grimoire_select||game.phase==Phase::Remove)&&(!card_removable(game.deck[cursor])||game.deck_count<=1)){message("这张牌不能移除");return true;}
        deck_candidate=cursor;deck_choice_revision=game.revision;deck_choice_phase=game.phase;pointer_cancel();
    }else return false;
    return true;
}
}

namespace spire {
void App::open_statuses(int actor){
    if(title||paused||help||panel||library||character_select||detail_card>=0||detail_item>=0||map_preview.open||!combat_phase(game.phase))return;
    if(actor<0||actor>game.enemy_count)return;
    status_actor=actor;status_selected=status_scroll=status_scroll_limit=0;pointer_cancel();
}
bool App::status_input(Key key){
    if(status_actor<0){if(key!=Key::Statuses)return false;open_statuses(0);return true;}
    if(key==Key::Back||key==Key::Statuses){status_actor=-1;return true;}
    if(key==Key::Quit){cancel_map_travel();quit=true;return true;}
    if(key==Key::Left||key==Key::Right){
        int count=game.enemy_count+1;
        status_actor=(status_actor+(key==Key::Left?count-1:1))%count;status_selected=status_scroll=status_scroll_limit=0;
    }
    int count=collect_statuses(game,status_actor,nullptr);
    if(key==Key::Up&&status_selected>0){--status_selected;status_scroll=0;}
    if(key==Key::Down&&status_selected+1<count){++status_selected;status_scroll=0;}
    if(key==Key::Confirm)ui_action(UiAction::MoreStatus);
    return true;
}
}

namespace spire {
void App::open_owned_relic(int position){
    if(title||paused||help||library||character_select||map_preview.open||status_actor>=0||
       detail_card>=0||detail_item>=0||(panel&&panel!=5)||position<0||position>=int(ui_owned_relic_count(game)))return;
    detail_owned=position;detail_item=ui_owned_relic_id(game,position);detail_scroll=0;pointer_cancel();
}
void App::sync_relic_feedback(){
    if(seen_world_relic_revision!=game.world_relics.flash_revision){seen_world_relic_revision=game.world_relics.flash_revision;world_relic_flash=game.world_relics.last_flash;relic_flash_ticks=30;}
    if(seen_relic_revision==game.relic_revision)return;
    seen_relic_revision=game.relic_revision;relic_flash_mask=game.relic_flash;relic_flash_ticks=30;
}
}

namespace spire {
bool App::room_input(Key key){
    if(event_reader){
        if(key==Key::Back||key==Key::Detail||key==Key::Confirm){event_reader=false;pointer_cancel();}
        else if(key==Key::Up||key==Key::Left){event_reader_scroll-=key==Key::Left?10:1;if(event_reader_scroll<0)event_reader_scroll=0;}
        else if(key==Key::Down||key==Key::Right){event_reader_scroll+=key==Key::Right?10:1;if(event_reader_scroll>2048)event_reader_scroll=2048;}
        else if(key==Key::Quit){cancel_map_travel();quit=true;}
        return true;
    }
    if(title||paused||help||panel||library||character_select||map_preview.open||detail_card>=0||detail_item>=0||deck_candidate>=0)return false;
    if(game.phase==Phase::Event&&key==Key::Detail){ui_action(UiAction::ReadEvent);return true;}
    if(game.phase!=Phase::Shop)return false;
    if(shop_open){
        if(key==Key::Back||(key==Key::Confirm&&cursor==ShopLeave)){ui_action(UiAction::CloseShop);return true;}
        return false;
    }
    if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){cursor=cursor?0:1;return true;}
    if(key==Key::Confirm){ui_action(cursor?UiAction::LeaveShop:UiAction::OpenShop);return true;}
    if(key==Key::Detail){ui_action(UiAction::OpenShop);return true;}
    return false;
}
}
