#include "app.hpp"
#include "status.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "world_card_factory.hpp"
#include "ui_core_observer.hpp"
#include "library_search.hpp"
#include "settings_layout.hpp"
#include "menu_overlay_layout.hpp"
namespace spire {
static bool room_shop_context(const App&a){return a.game.phase==Phase::Shop||(a.game.phase==Phase::Event&&a.game.original_events.active&&a.game.original_events.id==OriginalEventId::FakeMerchant&&!a.game.original_events.finished);}
static bool room_shop_available(const App&a){return room_shop_context(a)&&!a.title&&!a.paused&&!a.help&&!a.panel&&!a.library&&!a.character_select&&!a.map_preview.open&&!a.event_reader&&!a.frontend_confirm&&!a.settings_open&&!a.map_window.is_open&&a.detail_card<0&&a.detail_item<0&&a.status_actor<0&&a.deck_candidate<0;}
#include "world_choice_input.inc"
#include "pile_input.inc"
#include "card_grid_input.inc"
uint32_t App::next_run_seed(bool* randomized){
    const uint32_t serial=run_number++;
    const uint32_t entropy=callbacks.seed_entropy?callbacks.seed_entropy():0;
    if(randomized)*randomized=entropy!=0;
    if(!entropy)return 0x20261003u+serial*7919u;
    // Host entropy separates process launches; the counter also separates
    // two starts that receive the same coarse hardware-clock sample.
    uint32_t value=entropy^(serial*0x9e3779b9u)^map_clock_ms^0xa341316cu;
    value^=value>>16;value*=0x7feb352du;value^=value>>15;
    value*=0x846ca68bu;value^=value>>16;return value?value:1u;
}
#include "card_library_input.inc"
#include "collection_input.inc"
#include "run_modes_input.inc"
void App::sync_selection(){
    if(choice_revision!=game.revision){choice_selected_count=0;choice_revision=game.revision;}
    if(!game.world.active||game.practice||!combat_choice_ready()||(catalog_choice_revision==game.revision&&catalog_choice_phase==unsigned(game.phase)))return;
    catalog_choice_revision=game.revision;catalog_choice_phase=unsigned(game.phase);
    // RelicSelectCmd and Reward.MarkContentAsSeen mark the offered content
    // when its screen is created. Reopening a pending saved choice also
    // observes it; hovering and timed renders do not rescan the reward set.
    if(world_choice_is_potion_reward()){discover_potion(world_choice_potion(0));return;}
    if(game.phase==Phase::OriginalEventChoice){const auto&e=game.original_events;
        if(e.selection==OriginalEventSelection::OfferCard)for(unsigned i=0;i<e.offer_count;++i)discover_card(card_from_token(e.offers[i]).kind);
        else if(e.selection==OriginalEventSelection::Relic)for(unsigned i=0;i<e.offer_count;++i)discover_relic(RelicId(e.offers[i]));
        else if(e.selection==OriginalEventSelection::Rewards)for(unsigned i=0;i<e.reward_count;++i){const auto&r=e.rewards[i];if(r.kind==2)discover_potion(Potion(r.model));else if(r.kind==3)discover_relic(RelicId(r.model));}
    }else if(game.phase==Phase::WorldRelicChoice){const auto&w=game.world_relics;
        if(w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::CombatOffer)for(unsigned i=0;i<w.offer_count;++i)discover_card(card_from_token(w.offers[i]).kind);
        else if(w.choice==WorldRelicChoice::OfferReward)for(unsigned i=0;i<w.offer_count;++i)if(w.offers[i]<w.reward_count){const auto&r=w.rewards[w.offers[i]];if(r.kind==2)discover_relic(RelicId(r.value));else if(r.kind==3)discover_potion(Potion(r.value));}
    }
}
#include "combat_selection_input.inc"
#include "item_inspect_input.inc"
void App::select_library_item(int index){
    if(!library)return;
    if(library_tab){prepare_collection();if(index>=0&&index<collection.count){library_selected=index;if(library_tab==1)open_relic_inspect(RelicId(collection.ids[index]));}return;}
    CardKind ids[CardKinds];const unsigned count=library_items(ids);if(index<0||unsigned(index)>=count)return;
    library_selected=index;card_library.control=-1;library_search_editing=false;
    if(library_card_seen(ids[index]))open_card_detail({ids[index],uint8_t(library_upgraded&&card_upgradeable({ids[index],0}))});
}
void App::ui_action(UiAction action){
    if(character_departure.active())return;
    sync_ftues();if(ftue_before_action(action))return;
    if(detail_item>=0&&detail_item<WorldRelicKinds){
        if(action==UiAction::InspectPreviousRelic)move_relic_inspect(-1);
        else if(action==UiAction::InspectNextRelic)move_relic_inspect(1);
        else if(action==UiAction::CloseRelicInspect||action==UiAction::Back)close_relic_inspect();
        return;
    }
    if(auxiliary.screen!=AuxiliaryScreen::None){if(action==UiAction::Back)close_auxiliary();return;}
    if(map_transition_blocks_pointer())return;
    UiCoreObserver observer(*this);
    pointer_cancel();
    // Opaque frontend modals own all actions until explicitly dismissed.
    if(frontend_confirm&&action!=UiAction::FrontendAccept&&action!=UiAction::FrontendCancel&&action!=UiAction::Back)return;
    if(settings_open&&!frontend_confirm){settings_action(action);return;}
    if(run_modes_action(action))return;
    if(choice_peeking||!combat_choice_ready())switch(action){
    case UiAction::ConfirmChoice:case UiAction::ClearChoice:case UiAction::PreviousChoicePage:case UiAction::NextChoicePage:
    case UiAction::ConfirmWorldChoice:case UiAction::ClearWorldChoice:case UiAction::PreviousWorldPage:case UiAction::NextWorldPage:
    case UiAction::LeaveWorldRewards:case UiAction::SacrificeReward:case UiAction::RerollReward:
    case UiAction::CrystalSmall:case UiAction::CrystalLarge:return;
    default:break;
    }
    switch(action){
    case UiAction::OpenCompendium:open_compendium();break;
    case UiAction::OpenProfiles:open_profiles();break;
    case UiAction::SelectProfile1:case UiAction::SelectProfile2:case UiAction::SelectProfile3:
        request_profile_change(unsigned(action)-unsigned(UiAction::SelectProfile1)+1);break;
    case UiAction::DeleteProfile1:case UiAction::DeleteProfile2:case UiAction::DeleteProfile3:
        if(profiles_open&&!profile_pending){const unsigned target=unsigned(action)-unsigned(UiAction::DeleteProfile1);
            if(profile_summaries[target].exists){profile_target=uint8_t(target+1);frontend_confirm=5;frontend_choice=0;pointer_cancel();}}break;
    case UiAction::OpenBestiary:if(compendium_open)open_bestiary();break;
    case UiAction::OpenStatistics:if(compendium_open)open_statistics();break;
    case UiAction::OpenHistory:if(compendium_open)open_history();break;
    case UiAction::HistoryPrevious:history_select(1);break;
    case UiAction::HistoryNext:history_select(-1);break;
    case UiAction::FrontendAccept:accept_frontend_confirm();break;
    case UiAction::FrontendCancel:frontend_confirm=frontend_choice=0;break;
    case UiAction::SettingsFastMode:if(settings_open)set_fast_mode(!fast_mode);break;
    case UiAction::SettingsControls:if(settings_open)help=true;break;
#if SPIRE_DEBUG_COMMANDS
    case UiAction::SettingsPractice:if(settings_open&&title){settings_open=false;practice_entry=true;character_select=true;}break;
    case UiAction::SettingsReference:if(settings_open&&title){settings_open=false;map_preview.open=true;if(!map_preview.generated)map_preview.generate();}break;
#else
    case UiAction::SettingsPractice:case UiAction::SettingsReference:break;
#endif
    case UiAction::SettingsBack:settings_open=false;break;
    case UiAction::SettingsGeneralTab:case UiAction::SettingsInputTab:
        if(settings_open){settings_tab=action==UiAction::SettingsInputTab;settings_item=0;pointer_cancel();}break;
    case UiAction::SettingsLanguage:case UiAction::SettingsLanguageEnglish:case UiAction::SettingsLanguageChinese:
    case UiAction::SettingsLongPress:case UiAction::SettingsLanguageClose:case UiAction::SettingsCardIndices:case UiAction::SettingsTextEffects:case UiAction::SettingsRunTimer:
    case UiAction::SettingsKeyboardOnly:case UiAction::SettingsPhobia:case UiAction::SettingsShakeDown:case UiAction::SettingsShakeUp:
    case UiAction::SettingsReset:case UiAction::SettingsScrollUp:case UiAction::SettingsScrollDown:
    case UiAction::SettingsFeedback:case UiAction::SettingsCredits:break;
#if SPIRE_DEBUG_COMMANDS
    case UiAction::LabCard:{
        const bool overflow=game.hand.count>=MaxHand;
        if(library&&!title&&!paused&&!help&&!panel&&!combat_actions_locked()&&detail_card>=0&&lab_card(game,card_from_token(detail_card))){
            detail_card=-1;library=false;reset_presentation();
            message(overflow?"手牌已满，卡牌已加入弃牌堆":"卡牌已加入手牌");
        }break;}
    case UiAction::LabPotion:
        if(library&&!title&&!paused&&!help&&!panel&&!combat_actions_locked()&&detail_item>=WorldRelicKinds&&lab_potion(game,(Potion)(detail_item-WorldRelicKinds+1))){
            detail_item=-1;library=false;reset_presentation();
            message("已获得药水");
        }break;
#else
    case UiAction::LabCard:case UiAction::LabPotion:break;
#endif
    case UiAction::OpenShop:
        if(room_shop_available(*this)){shop_open=true;room_presentation.merchant.set_open(true,map_clock_ms);room_presentation.merchant.speak(4,map_clock_ms);
            const bool fake=game.phase==Phase::Event;cursor=merchant_navigation::first(merchant_navigation::stock(game,fake),fake?6:ShopLeave);}break;
    case UiAction::CloseShop:
        if(room_shop_available(*this)){shop_open=false;room_presentation.merchant.set_open(false,map_clock_ms);cursor=0;}break;
    case UiAction::LeaveShop:
        if(!shop_open&&room_shop_available(*this)){
            if(game.phase==Phase::Shop){if(leave_shop(game))proceed_map_window();}
            else if(event_choice(game,6)&&original_event_leave(game)){cursor=0;proceed_map_window();}
        }break;
    case UiAction::GameOverSummary:
        show_game_over_summary();break;
    case UiAction::GameOverMainMenu:
        if(game_over_available()&&game_over_summary&&game_over_button_ready()){if(finish_saved_run())return_to_title();else message("无法删除存档，请重试");}break;
    case UiAction::GameOverViewRun:open_game_over_history();break;
    case UiAction::LeaveCamp:
        camp_input(Key::EndTurn);break;
    case UiAction::TreasureOpen:case UiAction::TreasureTake:case UiAction::TreasureLeave:
        treasure_action(action);break;
    case UiAction::PreviousCampPage:camp_page(-1);break;
    case UiAction::NextCampPage:camp_page(1);break;
    case UiAction::ReadEvent:
        if(event_body_active()){event_body_focus=!event_body_focus;break;}
        if(ancient_dialogue_ready()){event_body_focus=!event_body_focus;break;}
        if(!title&&!paused&&!panel&&!library&&(game.phase==Phase::Event||
           (game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Crystal))){event_reader=true;event_reader_scroll=0;}break;
    case UiAction::CloseEventReader:event_reader=false;break;
    case UiAction::StartCharacter:if(character_select)start_selected_run({},true);break;
    case UiAction::SelectRandomCharacter:if(character_select)character_random=true;break;
    case UiAction::RunSettings:break; // Retired port-only overlay; keep enum ABI.
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
        if(library||compendium_open)open_library(int(action)-int(UiAction::LibraryCards));break;
    case UiAction::PreviousPage:case UiAction::NextPage:break; // Retired pagination ABI.
    case UiAction::ToggleUpgrades:if(library&&!library_tab)library_control(LibraryViewUpgrades);break;
    case UiAction::InspectPreviousCard:move_card_inspect(-1);break;
    case UiAction::InspectNextCard:move_card_inspect(1);break;
    case UiAction::ToggleCardUpgrade:
        if(detail_card>=0){Card c=card_from_token(detail_card);if(c.upgraded||card_upgradeable(c)){c.upgraded=!c.upgraded;detail_card=card_token(c);card_inspect.view_all_upgraded=false;}}break;
    case UiAction::DiscardPotion:
        if(panel==4&&!combat_actions_locked()&&can_discard_potion(game,scroll)){
            const Potion bottle=potion_at(game,scroll);const int slot=scroll;
            if(discard_potion(game,slot)){remove_potion_visual(bottle,slot,true);close_potion(bottle);}
        }break;
    case UiAction::TargetFirst:if(panel==4&&game.enemy_count&&game.enemies[0].hp>0)target=0;break;
    case UiAction::TargetSecond:if(panel==4&&game.enemy_count>1&&game.enemies[1].hp>0)target=1;break;
    case UiAction::RewardCards:
        if(game.phase==Phase::Reward&&!(game.reward_claimed&2)){reward_cards=true;sync_card_reward_presentation();}break;
    case UiAction::RewardGold:
        if(game.phase==Phase::Reward){claim_reward_gold(game);}break;
    case UiAction::RewardPotion:
        if(game.phase==Phase::Reward&&!claim_reward_potion(game))potion_obtain_failed();break;
    case UiAction::LeaveRewards:
        if(game.phase==Phase::Reward){const bool left=leave_rewards(game);reward_cards=grimoire_select=false;cursor=0;if(left)proceed_map_window();}break;
    case UiAction::RewardGrimoire:
        if(!title&&!paused&&!help&&!panel&&game.phase==Phase::Reward&&game.necro.grimoire_pending){grimoire_select=true;reward_cards=false;cursor=0;}break;
    case UiAction::SkipGrimoire:
        if(!title&&!paused&&!help&&!panel&&grimoire_select&&game.phase==Phase::Reward&&necro_grimoire_choice(game,-1)){
            deck_candidate=-1;if(!game.necro.grimoire_pending){grimoire_select=false;cursor=3;}
        }break;
    case UiAction::ShowStars:return;
    case UiAction::ShowOrbs:case UiAction::ShowOsty:{
        open_statuses(0);if(status_actor<0)break;
        const int wanted=action==UiAction::ShowStars?44:action==UiAction::ShowOsty?45:46;
        int ids[StatusKinds],n=combat_statuses(0,ids);for(int i=0;i<n;i++)if(ids[i]==wanted){status_selected=i;break;}
        break;}
    case UiAction::MoreStatus:
        if(status_actor>=0)status_scroll=status_scroll>=status_scroll_limit?0:status_scroll+5;break;
    case UiAction::SelectIronclad:case UiAction::SelectSilent:case UiAction::SelectDefect:case UiAction::SelectRegent:case UiAction::SelectNecrobinder:
        if(character_select){character_random=false;selected_character=action==UiAction::SelectIronclad?Character::Ironclad:action==UiAction::SelectSilent?Character::Silent:
            action==UiAction::SelectDefect?Character::Defect:action==UiAction::SelectRegent?Character::Regent:Character::Necrobinder;}break;
    case UiAction::FilterAll:case UiAction::FilterIronclad:case UiAction::FilterSilent:case UiAction::FilterColorless:
    case UiAction::FilterDefect:case UiAction::FilterRegent:case UiAction::FilterNecrobinder:
    case UiAction::CycleSort:break; // Replaced by original sidebar controls.
    case UiAction::PreviousChoicePage:case UiAction::NextChoicePage:
        if(!title&&!paused&&!help&&!panel&&choice_phase(game.phase)){
            int page=cursor/10+(action==UiAction::PreviousChoicePage?-1:1);
            int last=(choice_pile(game).count-1)/10;
            if(page<0)page=0;if(page>last)page=last;cursor=page*10;
        }break;
    case UiAction::ConfirmChoice:if(combat_selection_confirmable())confirm_choice();break;
    case UiAction::SkipCombatChoice:skip_combat_selection();break;
    case UiAction::ConfirmWorldChoice:confirm_world_choice();break;
    case UiAction::LeaveWorldRewards:{
        const bool rewards=game.phase==Phase::WorldRelicChoice?game.world_relics.choice==WorldRelicChoice::OfferReward:game.phase==Phase::OriginalEventChoice&&game.original_events.selection==OriginalEventSelection::Rewards;
        if(!rewards||world_choice_min())break;
        const bool terminal=game.world.active&&game.phase==Phase::WorldRelicChoice&&game.world_relics.return_phase==uint8_t(Phase::Reward);
        if(world_retained_reward_group(game)){
            // The original terminal list is still claimable after closing
            // its map. Preserve its exact active group and offers here.
            if(world_finish_room(game))proceed_map_window();
            break;
        }
        choice_selected_count=0;confirm_world_choice();
        // A terminal combat set's Skip is the room's Proceed action. Nested
        // event/relic sets resume their parent and are never closed here.
        if(terminal&&game.phase==Phase::Reward&&!world_relic_pending(game)&&ui_reward_actions(game,nullptr)==1&&leave_rewards(game))proceed_map_window();
        break;}
    case UiAction::ClearWorldChoice:choice_selected_count=0;break;
    case UiAction::PreviousWorldPage:case UiAction::NextWorldPage:{
        if(reward_list_interactive()){reward_presentation.scroll_list(action==UiAction::PreviousWorldPage?-10:10);break;}
        if(card_reward_active())break;
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
    case UiAction::ClearChoice:if(!choice_peeking)choice_selected_count=0;break;
    case UiAction::ConfirmDeckChoice:confirm_deck_choice();break;
    case UiAction::CancelDeckChoice:if(deck_candidate>=0)cancel_deck_choice();break;
    case UiAction::DeckViewUpgrades:
        if(deck_selection_active()&&game.phase==Phase::Upgrade&&pointer.visible){sync_deck_selection();deck_selection.show_upgrades=!deck_selection.show_upgrades;}
        else if(world_selection_active()&&world_selection_upgrades()&&pointer.visible){sync_world_selection();deck_selection.show_upgrades=!deck_selection.show_upgrades;}break;
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
    if(key==Key::Collection){open_compendium();return true;}
    if(library){
        if(key==Key::Back){open_compendium();return true;}
        if(key==Key::Map&&game.world.active&&!title&&!paused){library=compendium_open=false;if(!map_window.visible)open_map_window(true);return true;}
        if(key==Key::Quit){cancel_map_travel();quit=true;return true;}
        library_input(key);return true;
    }
    if(character_select){
        run_settings=false;
        if(key==Key::Confirm)ui_action(UiAction::StartCharacter);
        else if(key==Key::Inspect&&!practice_entry)ui_action(UiAction::RunAscensionDown);
        else if(key==Key::ExhaustPile&&!practice_entry)ui_action(UiAction::RunAscensionUp);
        else if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){
            unsigned current=CharacterKinds;
            if(!character_random)for(unsigned i=0;i<CharacterKinds;++i)if(RunModeCharacters[i]==selected_character){current=i;break;}
            const unsigned next=(current+(key==Key::Left||key==Key::Up?CharacterKinds:1))%(CharacterKinds+1);
            character_random=next==CharacterKinds;if(!character_random)selected_character=RunModeCharacters[next];
        }
        else if(key==Key::Back){if(practice_entry)character_select=false;else open_run_menu(RunMenu::Singleplayer);}
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
bool App::deck_selection_active()const{
    return !title&&!paused&&!help&&!panel&&!library&&!character_select&&!settings_open&&!frontend_confirm&&!map_preview.open&&
        !map_window.is_open&&!event_reader&&!ftue.active()&&status_actor<0&&detail_card<0&&detail_item<0&&
        (deck_phase(game.phase)||(grimoire_select&&game.phase==Phase::Reward&&game.necro.grimoire_pending));
}
bool App::deck_selection_cancelable()const{
    return grimoire_select||game.phase==Phase::Remove||game.phase==Phase::Upgrade||game.event_deck_action!=EventDeckAction::None;
}
unsigned App::deck_choice_items(uint16_t*out,unsigned capacity)const{
    unsigned count=0;const bool remove=game.phase==Phase::Remove||grimoire_select;
    // FromDeckForRemoval preserves deck order within its curse-first sort.
    // Store only indices: all clicks, facts and commands address the actual deck.
    for(unsigned pass=0;pass<(remove?2u:1u);++pass)for(unsigned i=0;i<game.deck_count;++i){
        const Card card=game.deck[i];
        const bool eligible=game.phase==Phase::Upgrade?card_upgradeable(card):game.phase==Phase::Transform?
            int(i)!=game.event_first&&card_transformable(game,card,int(i)):card_removable(game,card,int(i))&&game.deck_count>1;
        if(!eligible)continue;
        if(remove&&((card_type(game,card,-1,int(i))==CardType::Curse)!=(pass==0)))continue;
        if(out&&count<capacity)out[count]=uint16_t(i);++count;
    }
    return count;
}
void App::sync_deck_selection(){
    auto&view=deck_selection;
    if(!view.bound||view.revision!=game.revision||view.phase!=unsigned(game.phase)){
        view.reset();view.bound=true;view.revision=game.revision;view.phase=uint8_t(game.phase);view.opened=map_clock_ms;
        if(game.phase==Phase::Remove&&game.event_deck_action==EventDeckAction::None&&
           (!game.world.active||game.world.room_type==unsigned(WorldRoom::Shop))){
            // The source deck overlay covers the already-open inventory.
            // Loading the picker or canceling it must not replay OpenShop.
            shop_open=true;auto&m=room_presentation.merchant;
            m.open=m.initialized=true;m.start=map_clock_ms-1000;m.from_y=MerchantInventoryLayout::RugY;m.from_alpha=204;m.sold_mask=game.shop_sold;
        }
    }
    // NDeckUpgradeSelectScreen clears, rather than merely hides, the tickbox
    // whenever directional navigation becomes active.
    if(!pointer.visible&&!paused&&!settings_open&&!frontend_confirm&&!help&&!ftue.active())view.show_upgrades=false;
    uint16_t indices[MaxDeck];const unsigned count=deck_choice_items(indices,MaxDeck);
    bool found=false;for(unsigned i=0;i<count;++i)if(indices[i]==cursor){found=true;break;}
    if(!found)cursor=count?indices[0]:0;
}
void App::cancel_deck_choice(){
    if(!deck_selection_active())return;
    if(deck_candidate>=0){deck_candidate=-1;pointer_cancel();focus_card_grid();return;}
    if(!deck_selection_active()||!deck_selection_cancelable())return;
    if(grimoire_select){grimoire_select=false;cursor=3;}
    else if(game.event_deck_action!=EventDeckAction::None){cancel_event_selection(game);reset_presentation(true);return;}
    else if(game.phase==Phase::Remove){game.phase=Phase::Shop;shop_open=true;cursor=ShopRemove;}
    else if(game.phase==Phase::Upgrade){if(game.world.active)world_rest_cancel(game);else game.phase=Phase::Camp;cursor=1;}
    deck_selection.reset();pointer_cancel();
}
bool App::deck_candidate_valid()const{
    const bool grimoire=grimoire_select&&game.phase==Phase::Reward&&game.necro.grimoire_pending;
    if(deck_candidate<0||deck_candidate>=game.deck_count||deck_choice_revision!=game.revision||deck_choice_phase!=game.phase||(!deck_phase(game.phase)&&!grimoire))return false;
    Card c=game.deck[deck_candidate];
    if(game.phase==Phase::Upgrade&&!card_upgradeable(c))return false;
    if(game.phase==Phase::Transform&&(deck_candidate==game.event_first||!card_transformable(game,c,deck_candidate)))return false;
    if((grimoire||game.phase==Phase::Remove)&&(!card_removable(game,c,deck_candidate)||game.deck_count<=1))return false;
    return true;
}
void App::confirm_deck_choice(){
    if(!deck_selection_active())return;
    if(!deck_candidate_valid()){deck_candidate=-1;return;}
    int index=deck_candidate;Phase phase=game.phase;deck_candidate=-1;pointer_cancel();
    const auto rest_selection=room_presentation.rest.selection;
    RewardPresentation::Upgrades upgraded;
    RewardPresentation::RemovedCard removed;
    if(phase==Phase::Remove||grimoire_select){
        removed.card=game.deck[index];if(const auto*facts=event_card_facts(game,removed.card,-1,index))removed.facts=*facts;
        removed.energy=int16_t(card_offer_cost(removed.card,&removed.facts));removed.stars=int16_t(card_star_cost(game,removed.card));
        card_description(removed.card,-1,removed.description,sizeof removed.description,-1,index,&removed.facts);
        removed.start=map_clock_ms;removed.active=true;
    }
    if(grimoire_select&&phase==Phase::Reward){
        if(necro_grimoire_choice(game,index)){reward_presentation.removed=removed;cursor=index<game.deck_count?index:game.deck_count-1;if(!game.necro.grimoire_pending){grimoire_select=false;cursor=3;}}
        else message("此时无法移除这张牌");return;
    }
    bool ok=phase==Phase::Upgrade?upgrade_card(game,index):phase==Phase::Remove?remove_card(game,index):transform_card(game,index);
    if(ok){
      if(phase==Phase::Upgrade)upgraded.add(game.deck[index],game.event_cards.deck[index],index,map_clock_ms);
      reset_presentation(true);if(removed.active)reward_presentation.removed=removed;if(upgraded.count)reward_presentation.upgrades=upgraded;
      if(rest_selection.pending&&game.world.active&&game.world.room_type==unsigned(WorldRoom::Rest)){
        room_presentation.rest.restore_selection(rest_selection,map_clock_ms);sync_camp_presentation();
      }
      if(game.phase==Phase::Shop){cursor=ShopRemove;shop_open=true;
        // The removal picker covers the open rug. Returning to that rug is
        // not another Open animation; Used starts when removal succeeds.
        auto&m=room_presentation.merchant;m.open=m.initialized=true;m.start=map_clock_ms-1000;m.from_y=MerchantInventoryLayout::RugY;m.from_alpha=204;m.remove_start=map_clock_ms;m.sold_mask=game.shop_sold;m.speak(0,map_clock_ms);
    }}
    else message("此时无法完成这项操作");
}
bool App::deck_input(Key key){
    if(grimoire_select&&(game.phase!=Phase::Reward||!game.necro.grimoire_pending))grimoire_select=false;
    if(title||paused||help||panel||library||(!deck_phase(game.phase)&&!grimoire_select))return false;
    if(deck_candidate>=0){
        if(!deck_candidate_valid()){deck_candidate=-1;return true;}
        if(key==Key::Back){cancel_deck_choice();return true;}
        if(key==Key::Confirm){confirm_deck_choice();return true;}
        // The irreversible-action confirmation consumes navigation/game actions.
        // Save and load remain available; loading resets its captured revision.
        return key!=Key::Save&&key!=Key::Load&&key!=Key::Quit;
    }
    sync_deck_selection();
    if(key==Key::Back){cancel_deck_choice();return true;}
    if(grimoire_select&&key==Key::EndTurn){ui_action(UiAction::SkipGrimoire);return true;}
    int count=game.deck_count;if(!count)return true;
    if(cursor<0)cursor=0;if(cursor>=count)cursor=count-1;
    if(key==Key::Left)cursor=cursor>0?cursor-1:0;
    else if(key==Key::Right)cursor=cursor+1<count?cursor+1:count-1;
    else if(key==Key::Up)cursor=cursor>=5?cursor-5:cursor;
    else if(key==Key::Down)cursor=cursor+5<count?cursor+5:count-1;
    else if(key==Key::Detail)open_card_detail(game.deck[cursor],cursor);
    else if(key==Key::Confirm){
        if(game.phase==Phase::Upgrade&&!card_upgradeable(game.deck[cursor])){message("这张牌不能继续升级");return true;}
        if(game.phase==Phase::Transform&&(cursor==game.event_first||!card_transformable(game,game.deck[cursor],cursor))){message("请选择另一张可变化的牌");return true;}
        if((grimoire_select||game.phase==Phase::Remove)&&(!card_removable(game,game.deck[cursor],cursor)||game.deck_count<=1)){message("这张牌不能移除");return true;}
        deck_candidate=cursor;deck_choice_revision=game.revision;deck_choice_phase=game.phase;deck_selection.preview_opened=map_clock_ms;pointer_cancel();
    }else return false;
    return true;
}
}

namespace spire {
void App::open_statuses(int actor){
    if(title||paused||help||panel||library||character_select||detail_card>=0||detail_item>=0||map_preview.open||(!combat_phase(game.phase)&&!choice_peeking))return;
    if(actor<0||actor>game.enemy_count)return;
    status_actor=actor;status_selected=status_scroll=status_scroll_limit=0;pointer_cancel();
}
void App::open_status_icon(int encoded){
    if(encoded<0)return;const int actor=encoded/StatusKinds,id=encoded%StatusKinds;
    open_statuses(actor);if(status_actor!=actor)return;
    int ids[StatusKinds],count=combat_statuses(actor,ids);
    for(int i=0;i<count;i++)if(ids[i]==id){status_selected=i;return;}
}
bool App::status_input(Key key){
    if(status_actor<0){if(key!=Key::Statuses)return false;open_statuses(0);return true;}
    if(key==Key::Back||key==Key::Statuses){status_actor=-1;return true;}
    if(key==Key::Quit){cancel_map_travel();quit=true;return true;}
    if(key==Key::Left||key==Key::Right){
        int count=game.enemy_count+1;
        status_actor=(status_actor+(key==Key::Left?count-1:1))%count;status_selected=status_scroll=status_scroll_limit=0;
    }
    int count=combat_statuses(status_actor,nullptr);
    if(key==Key::Up&&status_selected>0){--status_selected;status_scroll=0;}
    if(key==Key::Down&&status_selected+1<count){++status_selected;status_scroll=0;}
    if(key==Key::Confirm)ui_action(UiAction::MoreStatus);
    return true;
}
}

namespace spire {
void App::open_owned_relic(int position){
    if(title||paused||help||library||character_select||map_preview.open||status_actor>=0||
       detail_card>=0||detail_item>=0||(panel&&panel!=3)||position<0||position>=int(ui_owned_relic_count(game)))return;
    open_relic_inspect(ui_owned_relic_id(game,position),position);
}
void App::sync_relic_feedback(){
    if(seen_world_relic_revision!=game.world_relics.flash_revision){seen_world_relic_revision=game.world_relics.flash_revision;world_relic_flash=game.world_relics.last_flash;relic_flash_ticks=30;}
    if(seen_relic_revision==game.relic_revision)return;
    seen_relic_revision=game.relic_revision;relic_flash_mask=game.relic_flash;relic_flash_ticks=30;
}
}

namespace spire {
bool App::event_body_active()const{
 return !title&&!paused&&!help&&!panel&&!library&&!settings_open&&!frontend_confirm&&!character_select&&
  !map_preview.open&&!map_window.is_open&&detail_card<0&&detail_item<0&&status_actor<0&&deck_candidate<0&&
  game.phase==Phase::Event&&game.original_events.active&&!room_presentation.event.ancient&&
  !(game.original_events.id==OriginalEventId::FakeMerchant&&!game.original_events.finished)&&
  !(game.original_events.id==OriginalEventId::CrystalSphere&&game.original_events.finished&&game.original_events.crystal_item_count);
}
bool App::room_input(Key key){
    if(event_body_active()&&event_body_focus){
        if(key==Key::Back||key==Key::Detail||key==Key::Confirm){event_body_focus=false;pointer_cancel();return true;}
        if(key==Key::Up||key==Key::Down||key==Key::Left||key==Key::Right){event_body_scroll.wheel(key==Key::Up?-1:key==Key::Left?-5:key==Key::Down?1:5);return true;}
    }
    if(event_reader){
        if(key==Key::Back||key==Key::Detail||key==Key::Confirm){event_reader=false;pointer_cancel();}
        else if(key==Key::Up||key==Key::Left){event_reader_scroll-=key==Key::Left?10:1;if(event_reader_scroll<0)event_reader_scroll=0;}
        else if(key==Key::Down||key==Key::Right){event_reader_scroll+=key==Key::Right?10:1;if(event_reader_scroll>2048)event_reader_scroll=2048;}
        else if(key==Key::Quit){cancel_map_travel();quit=true;}
        return true;
    }
    if(title||paused||help||panel||library||character_select||map_preview.open||detail_card>=0||detail_item>=0||deck_candidate>=0)return false;
    const bool fake=game.phase==Phase::Event&&game.original_events.active&&game.original_events.id==OriginalEventId::FakeMerchant&&!game.original_events.finished;
    if(game.phase==Phase::Event&&!fake){
        if(ancient_dialogue_input(key))return true;
        if(game.original_events.id==OriginalEventId::CrystalSphere&&game.original_events.finished&&game.original_events.crystal_item_count)
            return key==Key::Detail; // Its own Proceed has no default-event option tween.
        if(key==Key::Detail){ui_action(UiAction::ReadEvent);return true;}
        if(key==Key::Confirm&&game.original_events.active){const auto&m=room_presentation.event;
            if(!m.bound||m.id!=unsigned(game.original_events.id)||(!m.ancient&&!m.option_ready(unsigned(cursor),map_clock_ms))||(m.ancient&&!pointer.dispatching&&map_clock_ms-m.start<800))return true;
        }
        return false;
    }
    if(!room_shop_context(*this))return false;
    if(shop_open){
        if(key==Key::Back||(key==Key::Confirm&&cursor==(fake?6:ShopLeave))){ui_action(UiAction::CloseShop);return true;}
        if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){
            const auto direction=key==Key::Left?merchant_navigation::Left:key==Key::Right?merchant_navigation::Right:
                key==Key::Up?merchant_navigation::Up:merchant_navigation::Down;
            cursor=merchant_navigation::move(cursor,direction,merchant_navigation::stock(game,fake),fake);return true;
        }
        return false;
    }
    if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){cursor=cursor?0:1;return true;}
    if(key==Key::Confirm){ui_action(cursor?UiAction::LeaveShop:UiAction::OpenShop);return true;}
    if(key==Key::Detail){ui_action(UiAction::OpenShop);return true;}
    return false;
}
}
