#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
static const char* capture_directory;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"world reward UI: %s:%d %s (event=%s phase=%u)\n",__FILE__,__LINE__,#x,original_event_key(a.game.original_events.id),unsigned(a.game.phase));std::exit(1);}}while(0)

struct TextIndex {const char* key;uint16_t index;};
static const TextIndex world_text[]={
#include "../src/world_text_index.inc"
};
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=uint8_t(value>>(8*i));}
static bool ends(const std::string&s,const char*tail){size_t n=std::strlen(tail);return s.size()>=n&&s.compare(s.size()-n,n,tail)==0;}
static std::vector<uint8_t> synthetic_profile(){
 // Distinct test glyphs and image colors make a changed price, stock icon or
 // leaked old offer observable. This is an interaction fixture, not artwork.
 constexpr unsigned images=U_Count>CardKinds?U_Count:CardKinds,glyphs=4*95;
 constexpr unsigned glyphdir=40+images*16,stringdir=glyphdir+glyphs*16;
 constexpr unsigned clipdir=stringdir+ContentStrings*8,carddir=clipdir+AnimationKinds*16;
 constexpr unsigned uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
 std::vector<uint8_t>b(payload);
 put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);put(b,20,glyphs);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
 for(unsigned i=0;i<images;i++){
  unsigned w=i?2:82,h=i?2:60,offset=unsigned(b.size()),length=w*h*2;
  put(b,40+i*16,offset);put(b,44+i*16,w,2);put(b,46+i*16,h,2);put(b,52+i*16,length);b.resize(offset+length);
  for(unsigned pixel=0;pixel<w*h;pixel++)put(b,offset+pixel*2,((i*13+pixel*5)&31)<<11|((i*29+pixel*7)&63)<<5|((i*7+pixel*3)&31),2);
 }
 const unsigned sizes[]={8,10,12,16};unsigned glyph=0;
 for(unsigned size:sizes)for(unsigned cp=32;cp<127;cp++,glyph++){
  unsigned at=glyphdir+glyph*16,offset=unsigned(b.size());put(b,at,cp);put(b,at+4,offset);put(b,at+8,size,2);b[at+10]=5;b[at+11]=7;put(b,at+12,6,2);b.resize(offset+35);
  for(unsigned y=0;y<7;y++)for(unsigned x=0;x<5;x++)b[offset+y*5+x]=cp==' '?0:uint8_t((cp>>((x+y*3)%7)&1)?255:0);
 }
 std::vector<std::string>strings(ContentStrings,"SYNTHETIC_UI_TEXT");
 for(unsigned i=0;i<CardKinds;i++){strings[i*3]=definitions[i].key;strings[i*3+1]="CARD_DESCRIPTION";strings[i*3+2]="UPGRADED_DESCRIPTION";}
 for(const auto&row:world_text){
  CHECK(row.index<strings.size());std::string key=row.key;
  strings[row.index]=key;
  if(ends(key,".description")||ends(key,".eventDescription")||key.find(".talk.firstVisitEver.")!=std::string::npos){
   strings[row.index]="EVENT_READER_TEXT ";for(unsigned i=0;i<40;i++)strings[row.index]+="visible rules and saved choices ";
  }
  if(key=="ancients.PROCEED.title"||key=="events.PROCEED.title")strings[row.index]="PROCEED";
 }
 for(unsigned i=0;i<ContentStrings;i++){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(strings[i].size()));b.insert(b.end(),strings[i].begin(),strings[i].end());b.push_back(0);}
 for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
 for(unsigned i=0;i<CardKinds;i++){put(b,carddir+i*16,0);for(unsigned j=1;j<4;j++)put(b,carddir+i*16+j*4,i*3+j-1);}
 for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
 put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static unsigned hit_count(HitKind kind){unsigned count=0;for(int i=0;i<a.hit_count;i++)count+=a.hits[i].kind==kind;return count;}
static bool has_action(UiAction action){for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(action)&&a.hits[i].enabled)return true;return false;}
static HitRegion hit(HitKind kind,int value){
 unsigned count=0;HitRegion found{};for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value){found=a.hits[i];++count;}
 CHECK(count==1);CHECK(found.x>=0&&found.y>=0&&found.w>0&&found.h>0&&found.x+found.w<=ScreenW&&found.y+found.h<=ScreenH);return found;
}
static void settle_list(){
 a.render();for(unsigned i=0;i<48&&a.reward_presentation.list_scroll.position!=a.reward_presentation.list_scroll.target;++i){a.advance(33);a.render();}
 CHECK(a.reward_presentation.list_scroll.position==a.reward_presentation.list_scroll.target);
}
static void press(HitKind kind,int value){
 a.render();if(a.card_reward_active()&&(kind==HitKind::WorldChoice||kind==HitKind::Choice)&&!a.card_reward_input_ready()){
  a.advance(250);a.advance(100);a.render();
 }
 if(a.reward_list_interactive())settle_list();
 const auto h=hit(kind,value);CHECK(h.enabled);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;
}
static void press_action(UiAction action){press(HitKind::UiAction,int(action));}
static void present(){
 a.title=a.paused=a.help=a.library=a.character_select=false;a.run_settings=false;a.map_preview.open=false;a.reset_presentation();a.pointer.visible=false;
}
static void reset(uint64_t seed=0x5a8317){
 new_run(a.game,seed,false,Character::Ironclad);init_world_relics(a.game);a.game.phase=Phase::Map;
 a.game.world_relics.count=0;a.game.relic_count=0;a.game.relics=0;for(auto&r:a.game.world_relics.entries)r=WorldRelicInstance{};present();
}
static void add(RelicId id){CHECK(gain_world_relic(a.game,id));CHECK(!world_relic_pending(a.game));}
static void capture(const char*name){
 if(!capture_directory)return;a.pointer.visible=false;a.render();char path[1024]{};int n=std::snprintf(path,sizeof path,"%s/%s.ppm",capture_directory,name);CHECK(n>0&&unsigned(n)<sizeof path);
 FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[ScreenW*3];
 for(unsigned y=0;y<ScreenH;y++){for(unsigned x=0;x<ScreenW;x++){uint16_t p=a.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void single_column(unsigned total){
 a.render();const unsigned visible=hit_count(HitKind::WorldChoice);CHECK(visible>0&&visible<=6&&visible<=total);if(total<=4)CHECK(visible==total);int x=-1,y=-1,right=-1;
 // Original 402x86 rows and the 474x484 clipping mask at 4/21 scale.
 const int dy=a.reward_presentation.window(a.map_clock_ms).y;
 for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::WorldChoice){const auto&h=a.hits[i];CHECK(h.enabled&&h.w==81&&h.h>0&&h.h<=18&&h.y>=82+dy&&h.y+h.h<=174+dy);if(x>=0)CHECK(h.x==x&&h.x+h.w==right&&h.y>=y);x=h.x;right=h.x+h.w;y=h.y+h.h;}
 CHECK(!has_action(UiAction::PreviousWorldPage)&&!has_action(UiAction::NextWorldPage));
}
static unsigned reward_position(unsigned kind,unsigned value=65535){
 const auto&w=a.game.world_relics;for(unsigned i=0;i<w.offer_count;i++){const auto&r=w.rewards[w.offers[i]];if(r.kind==kind&&(value==65535||r.value==value))return i;}CHECK(false);return 0;
}
static void press_reward(unsigned position){a.cursor=int(position);press(HitKind::WorldChoice,int(position));}
static void single_reward(unsigned kind,unsigned value){auto&w=a.game.world_relics;
 w.group_count=w.reward_count=1;w.group_active[0]=w.group_skip[0]=1;w.rewards[0].group=1;w.rewards[0].kind=uint8_t(kind);w.rewards[0].value=uint16_t(value);
 w.choice=WorldRelicChoice::OfferReward;w.choice_group=1;w.return_phase=uint8_t(Phase::Map);w.choice_max=1;w.choice_cancelable=1;w.offer_count=1;w.offers[0]=0;a.game.phase=Phase::WorldRelicChoice;present();
}
static void glass_eye(){
 reset();add(WorldRelicDriftwood);add(WorldRelicPaelsWing);CHECK(gain_world_relic(a.game,WorldRelicGlassEye));present();auto&w=a.game.world_relics;
 CHECK(w.choice==WorldRelicChoice::OfferReward&&w.offer_count==5&&a.world_choice_page_size()>=w.offer_count);single_column(5);snapshot();
 CHECK(a.reward_presentation.window(a.map_clock_ms).alpha==0);
 a.advance(250);a.render();CHECK(a.reward_presentation.window(a.map_clock_ms).alpha>100&&a.reward_presentation.window(a.map_clock_ms).alpha<200);unchanged();capture("rewards-glass-eye-entering");
 a.advance(250);a.render();CHECK(a.reward_presentation.window(a.map_clock_ms).alpha==255);unchanged();capture("rewards-glass-eye-settled");
 a.input(Key::Down);CHECK(a.cursor==1);a.input(Key::Up);CHECK(a.cursor==0);a.render();a.pointer_scroll(1);CHECK(a.cursor==0&&a.reward_presentation.list_scroll.target>0);settle_list();single_column(5);unchanged();capture("rewards-glass-eye-scroll");
 a.pointer_move(190,130);a.pointer_button(true);a.pointer_move(190,105);a.pointer_button(false);CHECK(a.game.deck_count>0&&w.choice==WorldRelicChoice::OfferReward);unchanged();
 a.pointer_scroll(-40);settle_list();while(a.cursor)a.input(Key::Up);a.pointer.visible=false;settle_list();CHECK(a.cursor==0);single_column(5);unchanged();capture("rewards-glass-eye");
 const unsigned reward=w.offers[0],deck=a.game.deck_count;const int gold=a.game.gold;const auto rng=a.game.world.rng[unsigned(WorldRng::Rewards)];
 uint16_t original[8]{};for(unsigned i=0;i<w.rewards[reward].count;i++)original[i]=w.rewards[reward].tokens[i];
 press_reward(0);CHECK(a.world_choice_is_card_reward()&&w.choice_reward==reward+1&&a.world_choice_page_size()>=w.offer_count);a.render();CHECK(hit_count(HitKind::WorldChoice)==3);CHECK(has_action(UiAction::RerollReward)&&has_action(UiAction::SacrificeReward));
 CHECK(a.cursor==1);snapshot();CHECK(!hit(HitKind::WorldChoice,1).enabled);a.input(Key::Confirm);unchanged();
 a.advance(250);a.advance(99);a.render();CHECK(!hit(HitKind::WorldChoice,1).enabled);a.input(Key::Confirm);unchanged();
 a.advance(1);a.render();CHECK(hit(HitKind::WorldChoice,1).enabled);a.advance(150);a.render();
 CHECK(hit(HitKind::WorldChoice,0).x+hit(HitKind::WorldChoice,0).w/2==93);
 CHECK(hit(HitKind::WorldChoice,1).x+hit(HitKind::WorldChoice,1).w/2==160);
 CHECK(hit(HitKind::WorldChoice,2).x+hit(HitKind::WorldChoice,2).w/2==227);
 CHECK(hit(HitKind::WorldChoice,1).w>hit(HitKind::WorldChoice,0).w);a.advance(250);a.advance(250);a.render();unchanged();capture("rewards-card-row");
 snapshot();a.input(Key::Detail);CHECK(a.detail_card>=0);a.render();a.pointer_scroll(4);a.input(Key::Back);CHECK(a.detail_card<0);a.render();unchanged();
 // Back only closes the card overlay. It does not spend the parent reward,
 // repopulate it, or modify the deck; reopening sees those same three cards.
 a.input(Key::Back);CHECK(w.choice==WorldRelicChoice::OfferReward&&!(w.rewards[reward].flags&1)&&a.game.deck_count==deck&&a.game.gold==gold);
 CHECK(!std::memcmp(&rng,&a.game.world.rng[unsigned(WorldRng::Rewards)],sizeof rng));press_reward(0);for(unsigned i=0;i<w.offer_count;i++)CHECK(w.offers[i]==original[i]);
 a.render();snapshot();const auto alternate_key=a.input_bindings.keys[0][unsigned(InputCommand::Exhaust)];
 CHECK(a.physical_key(alternate_key,true));unchanged();CHECK(a.physical_key(alternate_key,false));
 CHECK(a.world_choice_is_card_reward()&&w.choice_reward==reward+1&&!world_relic_can_reroll_reward(a.game));a.render();CHECK(!has_action(UiAction::RerollReward));
 for(unsigned i=0;i<w.offer_count;i++)CHECK(card_rarity(card_from_token(w.offers[i]).kind)==2);snapshot();a.ui_action(UiAction::RerollReward);unchanged();
 const auto selected=w.offers[1];press(HitKind::WorldChoice,1);CHECK(a.game.deck_count==deck+1&&card_token(a.game.deck[deck])==selected&&w.choice==WorldRelicChoice::OfferReward&&w.offer_count==4);
 CHECK(!a.choice_selected_count);press_reward(0);const unsigned sacrificed=w.choice_reward-1;press_action(UiAction::SacrificeReward);
 CHECK(w.choice==WorldRelicChoice::OfferReward&&(w.rewards[sacrificed].flags&1)&&w.offer_count==3&&a.game.deck_count==deck+1);
 a.render();CHECK(!has_action(UiAction::SacrificeReward)&&!has_action(UiAction::RerollReward));press_action(UiAction::LeaveWorldRewards);CHECK(a.game.phase==Phase::Map&&!world_relic_pending(a.game));snapshot();
}
static void potion_and_special(){
 reset();CHECK(add_potion_slots(a.game,MaxPotionSlots-potion_slot_count(a.game)));for(unsigned i=0;i<MaxPotionSlots;i++)potion_set(a.game,i,Potion::Blood);
 single_reward(3,unsigned(Potion::Strength));auto&w=a.game.world_relics;snapshot();press_reward(0);CHECK(w.choice==WorldRelicChoice::OfferReward&&!(w.rewards[0].flags&1));unchanged();
 // Free the actual sixteenth belt slot without leaving the reward menu.
 a.input(Key::Potions);CHECK(a.hud_focus==2&&!a.panel);a.input(Key::Confirm);CHECK(a.panel==4);a.input(Key::Left);CHECK(a.scroll==15);a.render();unchanged();
 press_action(UiAction::DiscardPotion);CHECK(!a.panel&&potion_at(a.game,15)==Potion::Empty&&w.choice==WorldRelicChoice::OfferReward);snapshot();
 press_reward(0);CHECK(a.game.phase==Phase::Map&&potion_at(a.game,15)==Potion::Strength);for(unsigned i=0;i<15;i++)CHECK(potion_at(a.game,i)==Potion::Blood);snapshot();
 reset();add(WorldRelicDriftwood);add(WorldRelicPaelsWing);single_reward(5,card_token({DefectGeneticAlgorithm,1}));auto&r=a.game.world_relics.rewards[0];
 r.tokens[0]=17;r.facts[0].enchantment=EventEnchantment::Nimble;r.facts[0].enchantment_amount=3;a.render();snapshot();a.input(Key::Detail);
 CHECK(a.detail_has_facts&&a.detail_genetic==17&&a.detail_scythe==0&&a.detail_facts.enchantment==EventEnchantment::Nimble&&a.detail_facts.enchantment_amount==3);a.render();a.input(Key::Back);unchanged();
 CHECK(!world_card_reward_alternatives(a.game));unsigned deck=a.game.deck_count;press_reward(0);CHECK(a.game.phase==Phase::Map&&a.game.deck_count==deck+1&&a.game.defect.genetic_deck[deck]==17&&a.game.event_cards.deck[deck].enchantment==EventEnchantment::Nimble);snapshot();
 reset();single_reward(5,card_token({NecroTheScythe,0}));a.game.world_relics.rewards[0].tokens[1]=11;snapshot();a.input(Key::Detail);CHECK(a.detail_scythe==11&&a.detail_genetic==0);a.input(Key::Back);unchanged();deck=a.game.deck_count;press_reward(0);CHECK(a.game.necro.scythe_deck[deck]==11);snapshot();
}
static void last_reward_indexes(){
 reset();single_reward(4,1);auto&w=a.game.world_relics;w.reward_count=w.offer_count=MaxWorldRelicRewards;
 for(unsigned i=0;i<MaxWorldRelicRewards;i++){auto&r=w.rewards[i];r=WorldRelicReward{};r.group=1;r.kind=4;r.value=uint16_t(i+1);w.offers[i]=uint16_t(i);}present();a.cursor=MaxWorldRelicRewards-2;snapshot();a.input(Key::Down);CHECK(a.cursor==MaxWorldRelicRewards-1);settle_list();single_column(MaxWorldRelicRewards);unchanged();
 const int gold=a.game.gold;press_reward(MaxWorldRelicRewards-1);CHECK(a.game.gold==gold+MaxWorldRelicRewards&&w.offer_count==MaxWorldRelicRewards-1);snapshot();
 reset();CHECK(original_event_begin(a.game,OriginalEventId::CrystalSphere,31,1)&&original_event_choose(a.game,1));auto&e=a.game.original_events;
 e.finished=1;e.selection=OriginalEventSelection::Rewards;e.select_min=0;e.select_max=1;e.select_cancelable=1;e.reward_active=1;e.reward_count=MaxOriginalEventRewards;e.reward_current=NoOriginalEventReward;e.reward_card_count=0;
 for(auto&r:e.rewards){r=OriginalEventReward{};r.kind=1;r.amount=1;}a.game.phase=Phase::OriginalEventChoice;present();a.cursor=MaxOriginalEventRewards-2;snapshot();a.input(Key::Down);CHECK(a.cursor==MaxOriginalEventRewards-1);settle_list();single_column(MaxOriginalEventRewards);unchanged();
 const int event_gold=a.game.gold;press(HitKind::WorldChoice,MaxOriginalEventRewards-1);CHECK(a.game.gold==event_gold+1&&e.rewards[MaxOriginalEventRewards-1].claimed);snapshot();capture("rewards-crystal-last-index");
}
static void event_card_rewards(){
 reset();add(WorldRelicDriftwood);CHECK(original_event_begin(a.game,OriginalEventId::ColorfulPhilosophers,5,0)&&original_event_choose(a.game,0));present();auto&e=a.game.original_events;CHECK(e.selection==OriginalEventSelection::Rewards);single_column(e.reward_count);
 const unsigned deck=a.game.deck_count;press(HitKind::WorldChoice,0);CHECK(a.world_choice_is_card_reward());const unsigned current=e.reward_current;uint16_t original[MaxOriginalEventOffers]{};for(unsigned i=0;i<e.offer_count;i++)original[i]=e.offers[i];
 a.input(Key::Back);CHECK(e.selection==OriginalEventSelection::Rewards&&!e.rewards[current].claimed);press(HitKind::WorldChoice,0);for(unsigned i=0;i<e.offer_count;i++)CHECK(e.offers[i]==original[i]);
 const unsigned chosen=e.offers[0];press(HitKind::WorldChoice,0);CHECK(a.game.deck_count==deck+1&&card_token(a.game.deck[deck])==chosen);CHECK(e.selection==OriginalEventSelection::Rewards&&e.rewards[current].claimed);snapshot();press_action(UiAction::LeaveWorldRewards);CHECK(a.game.phase==Phase::Event&&e.finished&&!e.reward_active);snapshot();
}
static void event_combat_rewards(){
 reset();CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));a.game.original_events=OriginalEventState{};potion_set(a.game,0,Potion::FoulPotion);
 CHECK(original_event_begin(a.game,OriginalEventId::FakeMerchant,23,1)&&use_potion(a.game,0,-1));for(unsigned i=0;i<a.game.enemy_count;i++)a.game.enemies[i].hp=0;resolve_pending(a.game);present();auto&w=a.game.world_relics;
 CHECK(w.choice==WorldRelicChoice::OfferReward&&w.offer_count>=9);single_column(w.offer_count);const unsigned group=w.choice_group;press_reward(reward_position(1));CHECK(a.world_choice_is_card_reward());a.input(Key::Back);CHECK(w.choice==WorldRelicChoice::OfferReward);
 press_reward(reward_position(2,WorldRelicFakeMerchantsRug));CHECK(has_world_relic(a.game,WorldRelicFakeMerchantsRug)&&w.choice_group==group);press_reward(reward_position(1));const unsigned deck=a.game.deck_count,token=w.offers[0];press(HitKind::WorldChoice,0);
 CHECK(a.game.deck_count==deck+1&&card_token(a.game.deck[deck])==token&&a.game.original_events.combat_pending&&w.choice_group==group);capture("rewards-combat-extras");snapshot();
 // Terminal Skip continues through the saved combat parent in one public UI
 // action; it cannot leave a second legacy reward screen or award extras twice.
 press_action(UiAction::LeaveWorldRewards);CHECK(a.game.phase==Phase::Event&&a.game.original_events.finished&&!a.game.original_events.combat_pending&&original_event_combat_reward_count(a.game)==0);snapshot();
}
static void camp_proceed(bool keyboard){
 reset();a.fast_mode=false;CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));a.game.original_events=OriginalEventState{};a.game.world.room_type=uint8_t(WorldRoom::Rest);a.game.phase=Phase::Camp;
 auto&w=a.game.world;w.rest_count=2;w.rest_options[0]=uint8_t(WorldRestOption::Heal);w.rest_options[1]=uint8_t(WorldRestOption::Smith);add(WorldRelicMiniatureTent);a.game.hp=30;present();a.render();
 CHECK(!world_rest_can_proceed(a.game)&&!has_action(UiAction::LeaveCamp));snapshot();a.input(Key::EndTurn);unchanged();
 // NRestSiteButton.MouseFilter remains blocked for its 500ms entrance.
 CHECK(!hit(HitKind::Choice,0).enabled);a.advance(250);a.advance(249);a.render();CHECK(!hit(HitKind::Choice,0).enabled);a.advance(1);a.render();CHECK(hit(HitKind::Choice,0).enabled);unchanged();
 press(HitKind::Choice,0);CHECK(a.game.phase==Phase::Camp&&world_rest_can_proceed(a.game)&&world_rest_option_enabled(a.game,1));a.render();CHECK(!has_action(UiAction::LeaveCamp));snapshot();
 for(unsigned i=0;i<9;++i)a.advance(250);a.advance(249);a.render();CHECK(!has_action(UiAction::LeaveCamp));unchanged();a.advance(1);a.render();
 CHECK(a.camp_proceed_ready()&&a.proceed_motion.enabled&&!has_action(UiAction::LeaveCamp));unchanged();
 // Enable begins the original offscreen 500ms slide; its hit follows it.
 a.advance(250);a.advance(250);a.render();CHECK(has_action(UiAction::LeaveCamp));const int hp=a.game.hp;capture(keyboard?"rest-tent-keyboard":"rest-tent-proceed");
 if(keyboard)a.input(Key::EndTurn);else press_action(UiAction::LeaveCamp);CHECK(a.game.phase==Phase::Camp&&w.room_open==2&&a.map_window.is_open&&a.game.hp==hp);snapshot();
}
static void reward_animation_curves(){
 RewardPresentation p;p.observe_window(1,17,3,1000);CHECK(p.window(1000).alpha==0&&p.window(1000).y==20);
 CHECK(p.window(1250).alpha>100&&p.window(1250).alpha<200);CHECK(p.window(1500).alpha==255&&p.window(1500).y==0);
 p.observe_window(1,17,2,1500);CHECK(p.window_start==1000);p.observe_window(1,17,0,1500);
 CHECK(p.window(1625).alpha>100&&p.window(1625).alpha<200);CHECK(p.window(1750).alpha==0);
 p.append(RewardPickupKind::Potion,unsigned(Potion::Strength),0,86,120,2000);
 CHECK(p.hidden(RewardPickupKind::Potion,0,2349)&&!p.hidden(RewardPickupKind::Potion,0,2350));
 const auto potion=p.sample(p.flights[0],135,11,2175);CHECK(potion.x==122&&potion.y==39);
 p.append(RewardPickupKind::Relic,WorldRelicGoldenPearl,1,86,120,2000);const auto relic=p.sample(p.flights[1],30,32,2175);
 CHECK(relic.x<58&&relic.x>42&&relic.y<76&&relic.y>50);
 // Compare the bounded arithmetic gold path with the literal source delay
 // sequence, including positive/negative and very large changes.
 for(int delta:{1,9,51,101,9999,-1,-101,-9999})for(unsigned elapsed:{0u,399u,400u,450u,900u,3000u,20000u}){
  p.begin_gold(10000,10000+delta,0);int remaining=delta;unsigned at=400;
  while(remaining&&elapsed>=at){const int amount=remaining<0?-remaining:remaining,step=amount>100?75:amount>50?10:1;remaining+=remaining>0?-step:step;const int next=remaining<0?-remaining:remaining;at+=10+unsigned(next<10?10-next:0)*10;}
  CHECK(p.remaining_gold(elapsed)==remaining);CHECK(p.displayed_gold(elapsed)==10000+delta-remaining);
 }
}
static void acquisition_flights(){
 reset();potion_set(a.game,0,Potion::Empty);single_reward(3,unsigned(Potion::Strength));a.render();a.advance(500);a.render();
 const uint32_t serial=a.reward_presentation.serial;press_reward(0);CHECK(potion_at(a.game,0)==Potion::Strength);
 CHECK(a.reward_presentation.serial==serial+1&&a.reward_pickup_hidden(RewardPickupKind::Potion,0));
 snapshot();a.advance(175);a.render();unchanged();capture("reward-potion-flight");a.advance(175);a.render();CHECK(!a.reward_pickup_hidden(RewardPickupKind::Potion,0));unchanged();
 reset();single_reward(2,WorldRelicGoldenPearl);a.render();a.advance(500);a.render();const unsigned slot=ui_owned_relic_count(a.game);press_reward(0);
 CHECK(a.reward_pickup_hidden(RewardPickupKind::Relic,slot));snapshot();a.advance(175);a.render();unchanged();capture("reward-relic-flight");a.advance(175);a.render();CHECK(!a.reward_pickup_hidden(RewardPickupKind::Relic,slot));unchanged();
}
static void acquired_card_visual_facts(){
 // Isolate the pickup renderer at a fixed clock. Actual localized templates
 // must preserve permanent growth and enchantments after the holder vanishes.
 const Card cards[]={{DefectGeneticAlgorithm,0},{NecroTheScythe,0},{EventCardMadScience,0},{Defend,0},{Strike,0}};
 const auto pixels=[](){uint32_t value=2166136261u;for(auto pixel:a.pixels)value=(value^pixel)*16777619u;return value;};
 for(unsigned i=0;i<sizeof cards/sizeof cards[0];++i){
  reset();a.game.deck[0]=cards[i];a.game.event_cards.deck[0]=EventCardFacts{};a.game.defect.genetic_deck[0]=a.game.necro.scythe_deck[0]=0;
  a.reward_presentation.append(RewardPickupKind::Card,cards[i].kind,0,120,125,a.map_clock_ms,cards[i]);a.render();const auto plain=pixels();
  if(i==0)a.game.defect.genetic_deck[0]=40;
  else if(i==1)a.game.necro.scythe_deck[0]=23;
  else if(i==2)a.game.event_cards.deck[0].science_type=2;
  else if(i==3)CHECK(event_card_apply_enchantment(a.game.deck[0],a.game.event_cards.deck[0],EventEnchantment::Nimble,3));
  else CHECK(event_card_apply_enchantment(a.game.deck[0],a.game.event_cards.deck[0],EventEnchantment::TezcatarasEmber,1));
  a.render();CHECK(pixels()!=plain);snapshot();a.render();unchanged();if(i==0)capture("reward-card-genetic-flight");
  a.game.event_cards.deck[0]=EventCardFacts{};a.game.defect.genetic_deck[0]=a.game.necro.scythe_deck[0]=0;a.render();CHECK(pixels()==plain);
 }
}
int main(int argc,char**argv){
 std::vector<uint8_t>data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=synthetic_profile();
 if(argc>2)capture_directory=argv[2];CHECK(a.init(data.data(),data.size(),{}));CHECK(a.content.current_profile());
 reward_animation_curves();glass_eye();potion_and_special();last_reward_indexes();event_card_rewards();event_combat_rewards();camp_proceed(false);camp_proceed(true);acquisition_flights();
 if(a.content.version()>=LocaleContentVersion)acquired_card_visual_facts();
 std::printf("world reward UI: %u checks passed; %s profile; reward ownership, direct claim, alternatives and Camp Proceed\n",checks,argc>1?"actual":"synthetic");
}
