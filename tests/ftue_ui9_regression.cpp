// Keep the scene-specific text fitter private to the production renderer.
#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "resource_transfer.hpp"
#include "ui_core_observer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static App a;
static State snapshot_state,oracle_state;
static unsigned checks;
static bool actual;
static const char*captures;
static const char*scenario="setup";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FTUE UI [%s]:%d %s (modal=%u phase=%u clock=%u)\n",scenario,__LINE__,#x,unsigned(a.ftue.id),unsigned(a.game.phase),a.map_clock_ms);std::exit(1);}}while(0)
#include "ftue_fixture.hpp"
static void snapshot(){std::memcpy(&snapshot_state,&a.game,sizeof(State));}
static void unchanged(){CHECK(!std::memcmp(&snapshot_state,&a.game,sizeof(State)));}
static void advance(unsigned ms){while(ms){const unsigned step=ms>33?33:ms;a.advance(step);ms-=step;}a.render();}
static void prepare(){
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
 a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;
 a.reset_presentation();a.progress.tutorial_flags=FtueFresh|FtueSeenMask;a.ftue.manual_cards=0;
 a.pointer.visible=false;a.busy=0;
}
static void only(FtueId id){a.progress.tutorial_flags=uint16_t(FtueFresh|(FtueSeenMask&~ftue_bit(id)));}
static void capture(const char*name){
 CHECK(a.content.healthy());if(!captures)return;
 FILE*f=std::fopen((std::string(captures)+"/"+name+".ppm").c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(unsigned y=0;y<240;++y){unsigned char row[960];for(unsigned x=0;x<320;++x){const unsigned p=a.pixels[y*320+x];row[x*3]=(p>>11)*255/31;row[x*3+1]=((p>>5)&63)*255/63;row[x*3+2]=(p&31)*255/31;}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void modal_hits(){CHECK(a.ftue.active());CHECK(a.hit_count);for(unsigned i=0;i<a.hit_count;++i)CHECK(a.hits[i].kind==HitKind::FtueControl);}
static HitRegion button(int value){for(unsigned i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::FtueControl&&a.hits[i].value==value)return a.hits[i];CHECK(false);return {};}
static void click(int value){a.render();const auto h=button(value);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);CHECK(a.ftue.active());a.pointer_button(false);}
static void practice(Character character=Character::Ironclad){
 new_run(a.game,0x718541,true,character);prepare();snapshot();const unsigned before=a.animation;a.render();
 for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}
 CHECK(!a.combat_preparation.active&&a.content.healthy());CHECK(a.animation==before);unchanged();
}
static void embark(){
 scenario="fresh Embark asks before seed or State changes";practice();only(FtueId::AcceptTutorials);a.title=a.character_select=true;
 const unsigned serial=a.run_number;snapshot();CHECK(!a.start_selected_run({}));unchanged();CHECK(a.run_number==serial);a.render();modal_hits();
 a.input(Key::Back);unchanged();CHECK(a.ftue.id==FtueId::AcceptTutorials);advance(330);capture("ftue-accept");
 click(0);CHECK(!a.ftue.active()&&!a.character_select&&!a.title);CHECK(a.game.world.active);CHECK(!(a.progress.tutorial_flags&FtueEnabled));CHECK(a.seen_ftue(FtueId::CombatRules));
 scenario="Yes preserves enabled state and begins one run";practice();only(FtueId::AcceptTutorials);a.title=a.character_select=true;
 CHECK(!a.start_selected_run({}));click(1);CHECK(a.progress.tutorial_flags&FtueEnabled);CHECK(a.game.world.active&&!a.title);
 scenario="legacy metadata never inserts a new startup or Embark screen";practice();a.progress.tutorial_flags=0;a.title=a.character_select=true;
 CHECK(a.start_selected_run({}));CHECK(!a.ftue.active()&&!a.progress.tutorial_flags);
}
static void combat_pages(){
 scenario="three pages, previous, blocked hotkeys and exact completion";practice();only(FtueId::CombatRules);snapshot();CHECK(a.open_ftue(FtueId::CombatRules));
 a.render();modal_hits();CHECK(!(a.progress.tutorial_flags&ftue_bit(FtueId::CombatRules)));a.input(Key::Confirm);CHECK(a.ftue.page==0);
 a.input(Key::EndTurn);a.input(Key::Map);a.ui_action(UiAction::OpenShop);unchanged();
 advance(1100);capture("ftue-combat-1");click(3);CHECK(a.ftue.page==1);advance(660);capture("ftue-combat-2");
 a.physical_key(InputLeft,true);CHECK(a.ftue.page==0);a.physical_key(InputLeft,true);CHECK(a.ftue.page==0);a.physical_key(InputLeft,false);
 a.physical_key(InputRight,true);CHECK(a.ftue.page==1);a.physical_key(InputRight,true);CHECK(a.ftue.page==1);a.physical_key(InputRight,false);
 click(3);CHECK(a.ftue.page==2);advance(660);capture("ftue-combat-3");CHECK(!(a.progress.tutorial_flags&ftue_bit(FtueId::CombatRules)));
 click(3);CHECK(!a.ftue.active()&&a.seen_ftue(FtueId::CombatRules));unchanged();
 CHECK(a.ftue.banner);advance(1100);capture("ftue-combat-start");unchanged();advance(1500);CHECK(a.ftue.banner_player);advance(2500);CHECK(!a.ftue.banner);unchanged();
}
static void source_checks(){
 scenario="EndTurn explanation neither ends turn nor commits a card";practice();only(FtueId::CanPlayCards);a.game.energy=99;snapshot();
 CHECK(a.ftue_before_end_turn());a.render();advance(330);modal_hits();capture("ftue-can-play");click(1);unchanged();CHECK(!a.ftue.active());
 scenario="three genuinely unplayable hands silently retire EndTurn hint";practice();only(FtueId::CanPlayCards);a.game.energy=0;
 for(unsigned i=0;i<a.game.hand.count;++i)a.game.cards[a.game.hand.ids[i]]={Wound,0};
 for(unsigned i=0;i<3;++i){CHECK(!a.ftue_before_end_turn());CHECK(a.seen_ftue(FtueId::CanPlayCards)==(i==2));}
 scenario="CannotPlay requires only EnergyCostTooHigh, not flags combination";practice(Character::Regent);only(FtueId::CannotPlayCard);CHECK(a.game.hand.count);
 const int id=a.game.hand.ids[0];a.game.cards[id]={Strike,0};a.game.energy=0;snapshot();CHECK(a.ftue_before_play(0));advance(330);capture("ftue-cannot-play");click(1);unchanged();
 only(FtueId::CannotPlayCard);a.game.cards[id]={Wound,0};CHECK(!a.ftue_before_play(0));
 a.game.cards[id]={Strike,0};a.game.enemy_world.active=1;a.game.enemy_world.affliction[id]=unsigned(EnemyAffliction::Smog);CHECK(!a.ftue_before_play(0));a.game.enemy_world.affliction[id]=unsigned(EnemyAffliction::None);a.game.enemy_world.active=0;
 a.game.cards[id]={RegentCrescentSpear,0};a.game.expansion.stars=0;CHECK(card_star_cost(a.game,a.game.cards[id],id)>0);CHECK(!a.ftue_before_play(0));
 a.game.cards[id]={Strike,0};a.game.energy=99;CHECK(!a.ftue_before_play(0));
 // The eighth successful manual play is session-scoped. A new profile
 // after that point must not receive another synthesized auto-completion.
 a.ftue.manual_cards=0;for(unsigned i=0;i<7;++i)a.ftue_manual_played();CHECK(!a.seen_ftue(FtueId::CannotPlayCard));a.ftue_manual_played();CHECK(a.seen_ftue(FtueId::CannotPlayCard));
 only(FtueId::CannotPlayCard);a.ftue_manual_played();CHECK(!a.seen_ftue(FtueId::CannotPlayCard));
 scenario="Select hotkey activates on release and held keys cannot dismiss new modal";only(FtueId::CannotPlayCard);a.game.cards[id]={Strike,0};a.game.energy=0;
 CHECK(a.open_ftue(FtueId::CannotPlayCard));a.keyboard_only=true;a.physical_key(' ',true);CHECK(a.ftue.active());a.physical_key(' ',true);CHECK(a.ftue.active());a.physical_key(' ',false);CHECK(!a.ftue.active());a.keyboard_only=false;
}
static void begin_world(){
 new_run(a.game,0x7195c3,false);CHECK(world_set_seed(a.game,"NSPIRE-WORLD-017"));CHECK(world_begin_run(a.game));prepare();
}
static void calculator_select_release(){
 const auto bindings=a.input_bindings;const bool calculator=a.calculator_keyboard,keyboard=a.keyboard_only;
 a.physical_keys_cancel();practice();a.keyboard_only=false;
 scenario="desktop card5 is blocked by FTUE rather than treated as Select";
 a.calculator_keyboard=false;a.input_bindings=default_input_bindings(false);only(FtueId::CannotPlayCard);
 CHECK(a.open_ftue(FtueId::CannotPlayCard));snapshot();
 CHECK(a.physical_key('5',true));CHECK(a.ftue.active());CHECK(a.physical_key('5',false));CHECK(a.ftue.active());unchanged();
 CHECK(a.physical_key(13,true));CHECK(a.ftue.active());unchanged();CHECK(a.physical_key(13,false));CHECK(!a.ftue.active());unchanged();
 scenario="calculator5 retains Select adapter and accepts only on release";
 a.calculator_keyboard=true;a.input_bindings=default_input_bindings(true);only(FtueId::CannotPlayCard);
 CHECK(a.open_ftue(FtueId::CannotPlayCard));snapshot();const uint32_t context=a.input_context();
 CHECK(a.physical_key('5',true));CHECK(a.ftue.active()&&a.input_context()==context);unchanged();
 CHECK(a.physical_key('5',true));CHECK(a.ftue.active()&&a.input_context()==context);unchanged();
 CHECK(a.physical_key('5',false));CHECK(!a.ftue.active());unchanged();CHECK(!a.physical_key('5',false));unchanged();
 scenario="explicit calculator5 bindings precede the Select adapter";
 const InputCommand commands[]={InputCommand::Cancel,InputCommand::EndTurn};
 for(const auto command:commands){
  a.input_bindings=default_input_bindings(true);CHECK(rebind_input(a.input_bindings,0,unsigned(command),'5'));only(FtueId::CannotPlayCard);
  CHECK(a.open_ftue(FtueId::CannotPlayCard));snapshot();CHECK(a.physical_key('5',true));CHECK(a.ftue.active());
  CHECK(a.physical_key('5',false));CHECK(a.ftue.active());unchanged();
  CHECK(a.physical_key(13,true));CHECK(a.ftue.active());CHECK(a.physical_key(13,false));CHECK(!a.ftue.active());unchanged();
 }
 a.input_bindings=default_input_bindings(true);
 scenario="a captured calculator5 cannot dismiss a replacement tutorial";
 only(FtueId::CannotPlayCard);CHECK(a.open_ftue(FtueId::CannotPlayCard));CHECK(a.physical_key('5',true));const uint32_t captured=a.input_context();
 click(1);CHECK(!a.ftue.active());only(FtueId::CannotPlayCard);CHECK(a.open_ftue(FtueId::CannotPlayCard));CHECK(a.input_context()!=captured);snapshot();
 CHECK(a.physical_key('5',true));CHECK(a.ftue.active());CHECK(a.physical_key('5',false));CHECK(a.ftue.active());unchanged();
 CHECK(a.physical_key('5',true));CHECK(a.ftue.active());CHECK(a.physical_key('5',false));CHECK(!a.ftue.active());unchanged();
 scenario="keyboard-only Confirm stays distinct from source Select";
 a.keyboard_only=true;only(FtueId::CannotPlayCard);CHECK(a.open_ftue(FtueId::CannotPlayCard));snapshot();
 CHECK(a.physical_key(13,true));CHECK(a.physical_key(13,false));CHECK(a.ftue.active());unchanged();
 CHECK(a.physical_key('5',true));CHECK(a.ftue.active());CHECK(a.physical_key('5',false));CHECK(!a.ftue.active());unchanged();
 scenario="calculator5 never skips a three-page CombatRules lesson";
 a.keyboard_only=false;only(FtueId::CombatRules);CHECK(a.open_ftue(FtueId::CombatRules));snapshot();
 CHECK(a.physical_key('5',true));CHECK(a.physical_key('5',false));CHECK(a.ftue.id==FtueId::CombatRules&&a.ftue.page==0);CHECK(!a.seen_ftue(FtueId::CombatRules));unchanged();
 a.physical_keys_cancel();a.reset_presentation();a.input_bindings=bindings;a.calculator_keyboard=calculator;a.keyboard_only=keyboard;
}
static void rooms(){
 scenario="Map source root, sweep even in Fast, then100ms prompt";begin_world();only(FtueId::MapSelect);a.fast_mode=true;
 a.game.original_events=OriginalEventState{};a.game.phase=Phase::Event;CHECK(world_finish_room(a.game));a.open_map_window();CHECK(a.map_act_intro.camera_running);a.render();CHECK(!a.ftue.active());
 uint8_t routes[reference::Columns];const unsigned route_count=world_available_points(a.game,routes,reference::Columns);CHECK(route_count&&!a.begin_map_travel(routes[0]));
 for(unsigned i=0;i<200&&!a.ftue.active();++i)advance(33);CHECK(a.ftue.id==FtueId::MapSelect);CHECK(a.ftue.target==world_current_map(a.game)->root);advance(330);capture("ftue-map");snapshot();
 const int old_scroll=a.map_scroll;const auto old_drawing=a.map_drawing; a.pointer_scroll(2);a.pointer_aux_button(1,true);a.pointer_move(150,140);a.pointer_aux_button(1,false);CHECK(a.map_scroll==old_scroll);CHECK(a.map_drawing.pressed==old_drawing.pressed&&a.map_drawing.temporary_button==old_drawing.temporary_button);
 click(1);unchanged();a.close_map_window(false);
 scenario="merchant prompt is Leave-before-inventory, confirmation stays";begin_world();a.game.phase=Phase::Shop;a.game.world.room_type=unsigned(WorldRoom::Shop);only(FtueId::Merchant);
 a.render();CHECK(!a.ftue.active());snapshot();a.ui_action(UiAction::LeaveShop);CHECK(a.ftue.id==FtueId::Merchant);advance(330);capture("ftue-merchant");click(1);unchanged();
 only(FtueId::Merchant);a.ui_action(UiAction::LeaveShop);CHECK(a.ftue.active());click(4);CHECK(a.shop_open&&a.seen_ftue(FtueId::Merchant));unchanged();
 scenario="rest controls wait500ms and remain blocked by modal";begin_world();a.game.phase=Phase::Camp;a.game.world.room_type=unsigned(WorldRoom::Rest);a.game.world.rest_count=2;a.game.world.rest_options[0]=uint8_t(WorldRestOption::Heal);a.game.world.rest_options[1]=uint8_t(WorldRestOption::Smith);only(FtueId::RestSite);
 a.render();snapshot();a.input(Key::Confirm);unchanged();advance(499);CHECK(!a.ftue.active());advance(1);CHECK(a.ftue.id==FtueId::RestSite);advance(550);capture("ftue-rest");click(1);unchanged();
 scenario="real reward potion acquisition observes the command, not a restored belt";begin_world();only(FtueId::ObtainPotion);
 for(auto&p:a.game.potions)p=Potion::Empty;a.game.potions[0]=Potion::Block;a.render();CHECK(!a.ftue.active()&&!a.ftue.pending);
 a.game.phase=Phase::Reward;a.game.world.room_type=unsigned(WorldRoom::Monster);a.game.reward_potion=Potion::Blood;a.game.reward_claimed=0;
 std::memcpy(&oracle_state,&a.game,sizeof(State));CHECK(claim_reward_potion(oracle_state));a.ui_action(UiAction::RewardPotion);
 CHECK(!std::memcmp(&oracle_state,&a.game,sizeof(State)));CHECK(a.game.potions[1]==Potion::Blood);CHECK(a.ftue.pending&ftue_bit(FtueId::ObtainPotion));
 a.render();CHECK(a.ftue.id==FtueId::ObtainPotion);snapshot();advance(330);capture("ftue-potion");click(1);unchanged();
 scenario="terminal reward skip hint does not discard loot";begin_world();a.game.phase=Phase::Reward;a.game.world.room_type=unsigned(WorldRoom::Monster);a.game.last_gold=18;a.game.reward_claimed=0;a.game.world.reward_count=3;only(FtueId::CombatReward);
 advance(600);snapshot();a.ui_action(UiAction::LeaveRewards);CHECK(a.ftue.id==FtueId::CombatReward);advance(330);capture("ftue-rewards");click(1);unchanged();
 scenario="visible relic reward and actual power card use separate source highlights";only(FtueId::RelicReward);a.game.world.reward_relic=WorldRelicAnchor;advance(100);CHECK(a.ftue.id==FtueId::RelicReward);advance(330);capture("ftue-relic");click(1);
 a.game.world.reward_cards[0]={Inflame,0};a.game.world.reward_cards[1]={Strike,0};a.game.world.reward_cards[2]={Defend,0};only(FtueId::PowerCard);a.ui_action(UiAction::RewardCards);CHECK(a.reward_cards);advance(400);CHECK(a.ftue.id==FtueId::PowerCard&&a.ftue.target==0);advance(330);capture("ftue-power");snapshot();click(1);unchanged();
 scenario="Ascension uses SeenPopup after tutorials declined, exact entry delay";practice();a.title=a.character_select=true;a.progress.characters[0].highest_ascension=1;a.progress.tutorial_flags=FtueInitialized|ftue_bit(FtueId::AcceptTutorials);a.render();CHECK(a.ftue.id==FtueId::AscensionSingleplayer);
 snapshot();advance(999);CHECK(!a.hit_count);a.input(Key::Confirm);CHECK(a.ftue.active());advance(401);capture("ftue-ascension");click(1);unchanged();CHECK(a.seen_popup(FtueId::AscensionSingleplayer));
 a.progress.characters[0].highest_ascension=0;
}
static void shuffle_barrier(){
 scenario="shuffle presentation waits with pre-shuffle pile counts and unchanged canonical state";practice();only(FtueId::Shuffle);
 a.ftue.shuffle_draw=0;a.ftue.shuffle_discard=7;a.ftue.shuffle_exhaust=2;a.ftue.shuffle_at=a.animation+2;a.ftue.shuffle_waiting=true;
 snapshot();a.tick();CHECK(!a.ftue.active());a.tick();CHECK(a.ftue.id==FtueId::Shuffle);const unsigned stopped=a.animation;
 advance(600);CHECK(a.animation==stopped);const auto counts=a.combat_pile_counts();CHECK(counts.draw==0&&counts.discard==7&&counts.exhaust==2);capture("ftue-shuffle");unchanged();
 click(1);advance(66);CHECK(a.animation>stopped);unchanged();
}
static void real_shuffle(){
 scenario="real DrawOne shuffle stops visual piles and hand before the command cue";practice();CHECK(encounter_setup(a.game,EncounterId::NibbitsNormal));only(FtueId::Shuffle);
 while(a.game.draw.count){const unsigned id=a.game.draw.ids[--a.game.draw.count];a.game.discard.ids[a.game.discard.count++]=uint16_t(id);a.game.zones[id]=Zone::Discard;}
 CHECK(a.game.discard.count&&a.game.hand.count<MaxHand);a.game.no_draw=0;a.render();
 for(unsigned i=0;a.combat_preparation.active&&i<1024;++i){a.advance(33);a.render();}CHECK(!a.combat_preparation.active);
 const auto before=a.combat_pile_counts();const auto hand=a.combat_hand();CHECK(before.draw==0&&before.discard==a.game.discard.count);
 std::memcpy(&oracle_state,&a.game,sizeof(State));draw_cards(oracle_state,1);
 {UiCoreObserver observer(a);a.begin_combat_feedback();draw_cards(a.game,1);}
 CHECK(!std::memcmp(&oracle_state,&a.game,sizeof(State)));CHECK(a.game.hand.count==hand.count+1);CHECK(a.ftue.shuffle_waiting&&!a.ftue.active());
 CHECK(a.ftue.shuffle_draw==before.draw&&a.ftue.shuffle_discard==before.discard&&a.ftue.shuffle_exhaust==before.exhaust);
 const auto queued=a.combat_pile_counts();CHECK(queued.draw==before.draw&&queued.discard==before.discard&&queued.exhaust==before.exhaust);CHECK(CombatHand::same(a.combat_hand(),hand));
 snapshot();a.tick();CHECK(a.ftue.id==FtueId::Shuffle);const unsigned stopped=a.animation;advance(600);CHECK(a.animation==stopped);
 const auto shown=a.combat_pile_counts();CHECK(shown.draw==before.draw&&shown.discard==before.discard&&shown.exhaust==before.exhaust);CHECK(CombatHand::same(a.combat_hand(),hand));unchanged();capture("ftue-shuffle-real-command");
 click(1);advance(1000);CHECK(a.animation>stopped);CHECK(CombatHand::same(a.combat_hand(),CombatHand::capture(a.game.hand)));
 const auto after=a.combat_pile_counts();CHECK(after.draw==a.game.draw.count&&after.discard==a.game.discard.count&&after.exhaust==a.game.exhaust.count);unchanged();
}
static void relic_arrow_pixels(){
 if(!actual)return;
 // obtain_relic_ftue.tscn authors flip_v=true. The treasure holder's
 // bottom-center anchors the original47x39 arrow; opaque source pixels
 // therefore appear in reversed row order at (161,125). Check the final
 // framebuffer, excluding only the popup which is drawn above its right.
 const auto p=a.content.image(a.content.ui_image(I9_FtueArrow));CHECK(p.pixels&&p.alpha&&p.w==47&&p.h==39&&p.format<=3);
 unsigned opaque=0,distinguishing=0;
 for(unsigned sy=0;sy<p.h;++sy)for(unsigned sx=0;sx<p.w;++sx){
  const unsigned at=sy*p.w+sx;if(p.alpha[at]!=255)continue;
  const unsigned x=161+sx,y=125+p.h-1-sy;if(x>=198&&y>=145)continue;
  const auto*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
  CHECK(a.pixels[y*320+x]==uint16_t(pixel[0]|unsigned(pixel[1])<<8));++opaque;
  if(!p.alpha[(p.h-1-sy)*p.w+sx])++distinguishing;
 }
 CHECK(opaque>20&&distinguishing>5);
}
static void treasure_wait(){
 scenario="real TreasureOpen blocks both Take paths until its real1000ms relic lesson";begin_world();only(FtueId::RelicReward);a.fast_mode=true;
 a.game.original_events=OriginalEventState{};a.game.phase=Phase::Treasure;a.game.world.room_type=unsigned(WorldRoom::Treasure);a.game.world.room_open=1;a.game.treasure_state=0;a.game.world.treasure_relic=WorldRelicAnchor;a.render();
 a.ui_action(UiAction::TreasureOpen);CHECK(a.game.treasure_state==1&&a.treasure_collection_started);snapshot();
 a.input(Key::Confirm);a.treasure_action(UiAction::TreasureTake);CHECK(!a.treasure_pointer_can_take());unchanged();
 advance(999);CHECK(!a.ftue.active()&&!a.seen_ftue(FtueId::RelicReward));a.input(Key::Confirm);CHECK(!a.treasure_pointer_can_take());unchanged();
 advance(1);CHECK(a.ftue.id==FtueId::RelicReward&&a.ftue.target==-2);CHECK(!a.treasure_pointer_can_take());a.treasure_action(UiAction::TreasureTake);unchanged();
 advance(330);capture("ftue-treasure");relic_arrow_pixels();click(1);unchanged();CHECK(a.treasure_pointer_can_take());
 std::memcpy(&oracle_state,&a.game,sizeof(State));CHECK(take_treasure(oracle_state));a.input(Key::Confirm);CHECK(a.game.treasure_state==2&&has_world_relic(a.game,WorldRelicAnchor));CHECK(!std::memcmp(&oracle_state,&a.game,sizeof(State)));
 scenario="legacy treasure keyboard Take retains its existing immediate behavior";begin_world();a.progress.tutorial_flags=0;
 a.game.original_events=OriginalEventState{};a.game.phase=Phase::Treasure;a.game.world.room_type=unsigned(WorldRoom::Treasure);a.game.world.room_open=1;a.game.treasure_state=0;a.game.world.treasure_relic=WorldRelicAnchor;
 a.ui_action(UiAction::TreasureOpen);CHECK(a.game.treasure_state==1);a.input(Key::Confirm);CHECK(a.game.treasure_state==2&&!a.ftue.active()&&!a.progress.tutorial_flags);
}
static void terminal_boundaries(){
 scenario="completed rewards use ActFloor, including later acts and skipped rows";begin_world();a.game.phase=Phase::Reward;a.game.world.room_type=unsigned(WorldRoom::Monster);
 a.game.last_gold=0;a.game.reward_claimed=255;a.game.reward_potion=Potion::Empty;a.game.world.reward_relic=NoWorldRelic;a.game.world.reward_count=0;a.game.necro.grimoire_pending=0;
 only(FtueId::CombatReward);a.game.world.total_floor=39;a.game.world.map_row=3;CHECK(ui_reward_actions(a.game,nullptr)==1);
 CHECK(!a.ftue_before_action(UiAction::LeaveRewards));CHECK(!a.seen_ftue(FtueId::CombatReward));a.game.world.map_row=4;
 CHECK(!a.ftue_before_action(UiAction::LeaveRewards));CHECK(a.seen_ftue(FtueId::CombatReward));
 scenario="postcombat relic-generated rewards are not terminal rooms";only(FtueId::CombatReward);a.game.phase=Phase::WorldRelicChoice;
 auto&w=a.game.world_relics;w.choice=WorldRelicChoice::OfferReward;w.return_phase=uint8_t(Phase::Reward);w.group_count=w.choice_group=w.offer_count=w.reward_count=1;w.offers[0]=0;w.rewards[0].kind=4;w.rewards[0].value=17;w.choice_min=0;
 w.group_owner[0]=1;CHECK(!a.ftue_before_action(UiAction::LeaveWorldRewards));CHECK(!a.ftue.active()&&!a.seen_ftue(FtueId::CombatReward));
 w.group_owner[0]=0;w.choice_min=1;CHECK(!a.ftue_before_action(UiAction::LeaveWorldRewards));
 w.choice_min=0;w.choice_group=2;CHECK(!a.ftue_before_action(UiAction::LeaveWorldRewards));
 w.choice_group=1;snapshot();CHECK(a.ftue_before_action(UiAction::LeaveWorldRewards));CHECK(a.ftue.id==FtueId::CombatReward);advance(330);capture("ftue-terminal-extra-rewards");click(1);unchanged();
}
static void text_bounds(){
 if(!actual)return;scenario="every original localized paragraph fits its source rectangle";Canvas c{a};c.bold_face=5;
 const char*keys[]={"CANNOT_PLAY_CARD_FTUE_DESCRIPTION","CAN_PLAY_CARDS_FTUE_DESCRIPTION","MAP_SELECT_DESCRIPTION","MERCHANT_FTUE_DESCRIPTION","POTION_FTUE_DESCRIPTION","POWER_FTUE_DESCRIPTION","RELIC_FTUE_DESCRIPTION","REST_SITE_FTUE_DESCRIPTION","REWARDS_FTUE_DESCRIPTION","SHUFFLE_FTUE_DESCRIPTION"};
 for(unsigned i=0;i<10;++i){const int w=i==1?124:i==6?108:96,h=i==1?33:i==6?28:23;const char*text=ftue_text(a,keys[i]);CHECK(*text);const auto fit=ftue_paragraph(c,text,0,0,w,h,4,false,4);
  if(fit.truncated)std::fprintf(stderr,"FTUE paragraph overflow %s: %d rows in %d px\n",keys[i],fit.total,h);CHECK(!fit.truncated);
  RichReader reader(text);RichChar ch;while(reader.next(ch))if(ch.codepoint>=32)CHECK(a.content.glyph_metrics(ch.codepoint,4,c.font_style(ch)).advance>0);
 }
 for(unsigned page=1;page<=3;++page){char key[40];std::snprintf(key,sizeof key,"TUTORIAL_FTUE_BODY_%u",page);CHECK(!ftue_paragraph(c,ftue_text(a,key),0,0,119,92,5,true).truncated);}
 CHECK(!ftue_paragraph(c,ftue_text(a,"ASCENSION_SINGLEPLAYER_FTUE_DESCRIPTION"),0,0,98,76,5).truncated);
 CHECK(!ftue_paragraph(c,ftue_text(a,"ASCENSION_SINGLEPLAYER_FTUE_DISCLAIMER"),0,0,98,23,5).truncated);
 CHECK(!ftue_paragraph(c,a.content.world_string("main_menu_ui","ENABLE_TUTORIALS.description"),0,0,80,63,5,true).truncated);
}
int main(int argc,char**argv){
 Bytes data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));actual=true;}else data=fixture();
 if(argc>2)captures=argv[2];CHECK(a.init(data.data(),data.size(),{}));CHECK(a.content.has_ftue_assets());
 embark();combat_pages();source_checks();calculator_select_release();rooms();shuffle_barrier();real_shuffle();treasure_wait();terminal_boundaries();text_bounds();CHECK(a.content.healthy());std::printf("FTUE UI: %u checks passed (%s)\n",checks,actual?"actual":"synthetic");
}
