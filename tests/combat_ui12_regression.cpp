// Actual input/command/presentation boundaries. Host timing is not device
// performance evidence; the integrated release is accepted in Firebird.
#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static State before,direct;
static unsigned checks;
static const char*scenario="initial";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"combat UI12 [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static void fresh(unsigned cards=2,Character character=Character::Ironclad){
 new_run(a.game,0x41cef1ea,false,character);auto&s=a.game;
 s.phase=Phase::Combat;s.current_node=0;s.turn=s.floor=1;s.relics=s.relic_count=s.relic_flash=0;
 s.energy=99;s.expansion.stars=99;s.enemy_count=1;s.enemies[0]={999,999,0,0,0,2};
 s.deck_count=s.card_count=s.hand.count=s.draw.count=s.discard.count=s.exhaust.count=0;
 for(unsigned i=0;i<cards;++i){s.deck[s.deck_count++]={Strike,0};s.cards[i]={Strike,0};s.zones[i]=Zone::Hand;s.hand.ids[s.hand.count++]=uint16_t(i);++s.card_count;}
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=a.profiles_open=a.compendium_open=false;
 a.statistics_open=a.history_open=a.map_preview.open=false;a.run_mode={};a.frontend_confirm=0;a.progress.tutorial_flags=0;
 a.reset_presentation();a.pointer.visible=false;a.hit_count=0;a.map_clock_ms=100;a.animation=3;
 CHECK(validate(s));CHECK(a.hand_can_focus());
 // Real packs preload poses before accepting gameplay. Complete that
 // production preparation phase without advancing the fixture's UI clock.
 a.sync_combat_preparation();a.combat_preparation.shown=true;
 for(unsigned i=0;i<256&&a.combat_preparation.active;++i)a.step_combat_preparation();
 CHECK(!a.combat_preparation.active);
}
static void unchanged(){CHECK(!std::memcmp(&before,&a.game,sizeof before));}
static void canonical_equal(const State&left,const State&right){
 static SaveData one{},two{};CHECK(save(left,one));CHECK(save(right,two));
 CHECK(one.size==two.size&&!std::memcmp(one.bytes,two.bytes,one.size));
}
static void aimed_preview(){
 scenario="only the selected card receives target modifiers";fresh();a.game.enemies[0].vulnerable=2;before=a.game;
 a.target=0;CHECK(a.card_preview_target(0)==-1&&a.card_preview_target(1)==-1);
 a.cursor=0;a.input(Key::Confirm);CHECK(a.keyboard_card==0&&a.card_preview_target(0)==0&&a.card_preview_target(1)==-1);unchanged();
 a.input(Key::Back);CHECK(a.keyboard_card<0&&a.card_preview_target(0)==-1);unchanged();
 a.pointer.visible=true;a.pointer.armed_card=0;a.pointer.x=20;a.pointer.y=40;
 CHECK(a.card_preview_target(0)==-1);
 a.add_hit(200,60,60,60,HitKind::Enemy,0,true);a.pointer.x=220;a.pointer.y=80;
 CHECK(a.card_preview_target(0)==0&&a.card_preview_target(1)==-1);unchanged();
 a.pointer.x=170;CHECK(a.card_preview_target(0)==-1);unchanged();
 a.pointer.x=220;a.detail_card=card_token(a.game.cards[0]);CHECK(a.card_preview_target(0)==-1);a.detail_card=-1;
 a.game.enemies[0].hp=0;CHECK(a.card_preview_target(0)==-1);
}
static void numeric_selection(){
 scenario="ten physical calculator digits select in both input banks";
 const auto previous_bindings=a.input_bindings;const bool previous_calculator=a.calculator_keyboard,previous_keyboard=a.keyboard_only;
 for(unsigned bank=0;bank<2;++bank){fresh(10);a.calculator_keyboard=true;a.keyboard_only=bank!=0;a.input_bindings=default_input_bindings(true);before=a.game;
 for(unsigned slot=0;slot<10;++slot){
  const auto key=uint16_t(slot==9?'0':'1'+slot);
  CHECK(key&&a.physical_key(key,true));CHECK(a.keyboard_card==int(slot)&&a.cursor==int(slot));unchanged();
  CHECK(a.physical_key(key,true));CHECK(a.keyboard_card==int(slot));unchanged();CHECK(a.physical_key(key,false));
 }
 a.input(Key::Back);CHECK(a.keyboard_card<0);unchanged();
 a.hand_focus=true;a.cursor=3;CHECK(a.physical_key(InputRight,true));CHECK(a.cursor==4&&a.keyboard_card<0);CHECK(a.physical_key(InputRight,false));unchanged();
 CHECK(a.physical_key('2',true));CHECK(a.keyboard_card==1&&a.cursor==1);CHECK(a.physical_key('2',false));unchanged();a.input(Key::Back);
 }
 a.input_bindings=previous_bindings;a.calculator_keyboard=previous_calculator;a.keyboard_only=previous_keyboard;
 // The enlarged focused face clears the two control rails even at the
 // extreme authored fan positions; every source holder remains addressable.
 for(unsigned count=1;count<=10;++count)for(unsigned slot=0;slot<count;++slot){
  const auto pose=hand_pose(int(count),int(slot),true);const auto bounds=hand_bounds(pose);
  CHECK(bounds.x>=70&&bounds.x+bounds.w<=246&&bounds.y>=100);
  CHECK(hand_contains(pose,pose.cx,pose.cy));
 }
 // Entering target selection must retain a full-size readable face even
 // before its horizontal interpolation has settled. This covers the LCD
 // regression where source .75 scale left the description below y=240.
 CombatCardMotion motion;
 for(unsigned slot=0;slot<10;++slot){
  motion.begin(a.game,0);motion.set_selected(int(slot));motion.fit_centered_selection();
  for(unsigned tick=0;tick<=30;++tick){
   motion.advance(tick);const auto pose=motion.hand_pose_at(slot,false);const auto bounds=hand_bounds(pose);
   CHECK(pose.scale==HandFocusScale&&bounds.y>=22&&bounds.y+bounds.h<=240);
   CHECK(bounds.x>=70&&bounds.x+bounds.w<=246);
  }
 }
 unchanged();
}
static void denied_plays(){
 scenario="keyboard denied play creates a thought message without commands";fresh();a.game.energy=0;before=a.game;
 a.input(Key::Confirm);CHECK(a.combat_speech_key&&!std::strcmp(a.combat_speech_key,"NOT_ENOUGH_ENERGY"));
 CHECK(a.combat_speech_at==100&&a.keyboard_card<0);unchanged();
 scenario="pointer can click a nonplayable card";fresh(1);a.game.energy=0;before=a.game;
 const auto pose=a.combat_hand_pose(0,false);const auto bounds=hand_bounds(pose);
 a.add_hit(bounds.x,bounds.y,bounds.w,bounds.h,HitKind::Hand,0,true);
 a.pointer_move(pose.cx,pose.cy>239?239:pose.cy);CHECK(a.hit_at(a.pointer.x,a.pointer.y));
 a.pointer_button(true);a.pointer_button(false);
 CHECK(a.combat_speech_key&&!std::strcmp(a.combat_speech_key,"NOT_ENOUGH_ENERGY"));unchanged();
 scenario="resource and intrinsic failure reasons";fresh(1,Character::Regent);a.game.cards[0]={RegentAstralPulse,0};a.game.expansion.stars=0;
 CHECK(card_star_cost(a.game,a.game.cards[0],0)>0);before=a.game;a.combat_play_denied(0);
 CHECK(!std::strcmp(a.combat_speech_key,"NOT_ENOUGH_STARS"));unchanged();
 a.game.cards[0]={Wound,0};before=a.game;a.combat_play_denied(0);CHECK(!std::strcmp(a.combat_speech_key,"UNPLAYABLE"));unchanged();
}
static void battle_entry(){
 scenario="cold combat intro and restored combat suppression";fresh();before=a.game;
 CHECK(a.turn_banner_suppress_intro);a.sync_turn_banner();CHECK(!a.turn_banner.battle_active&&!a.turn_banner.player_active);unchanged();
 CHECK(!a.turn_banner_suppress_intro);++a.game.floor;before=a.game;a.sync_turn_banner();CHECK(a.turn_banner.battle_active);unchanged();
 a.game.phase=Phase::Map;a.reset_presentation();a.game.phase=Phase::Combat;before=a.game;a.sync_turn_banner();
 CHECK(a.turn_banner.battle_active&&a.turn_banner.player_start==a.animation+TurnBannerPresentation::BattleTicks);unchanged();
 a.animation+=TurnBannerPresentation::BattleTicks;CHECK(a.turn_banner.sample(a.animation).kind==TurnBannerPresentation::Player);
}
static void actual_single_battle_banner(){
 scenario="ordinary first-turn log has one banner and one Player Turn clock";fresh();
 a.game.phase=Phase::Map;a.reset_presentation();a.game.phase=Phase::Combat;before=a.game;
 a.observe_ftue_log({LogKind::Turn,1,0});a.sync_turn_banner();
 CHECK(!a.ftue.banner&&a.turn_banner.battle_active);unchanged();
 const auto verify_single=[](){
  const uint32_t tick=a.animation,clock=a.map_clock_ms,player=a.turn_banner.player_start;
  Canvas c{a};
  for(unsigned age:{301u,1000u,2200u,2607u,4100u,5100u}){
   a.map_clock_ms=clock+age;a.animation=tick+age/33;a.sync_ftues();
   CHECK(a.turn_banner.player_start==player);
   for(unsigned i=0;i<320u*240u;++i)a.pixels[i]=uint16_t((i*3917u)^(i>>2)^0x8410);
   turn_banner_screen(c);const std::vector<uint16_t>single(a.pixels,a.pixels+320*240);
   // The FTUE pass follows turn_banner_screen in the real renderer. It
   // must neither duplicate the label nor darken its band a second time.
   ftue_screen(c);CHECK(!std::memcmp(single.data(),a.pixels,sizeof a.pixels));unchanged();
  }
  CHECK(a.turn_banner.player_start==tick+79);
 };
 verify_single();
 scenario="the three-page lesson gates the same Battle Start sequence";fresh();
 a.progress_loaded=true;a.progress.tutorial_flags=uint16_t(FtueFresh|(FtueSeenMask&~ftue_bit(FtueId::CombatRules)));
 a.game.phase=Phase::Map;a.reset_presentation();a.game.phase=Phase::Combat;before=a.game;
 a.observe_ftue_log({LogKind::Turn,1,0});a.sync_turn_banner();
 CHECK(!a.turn_banner.battle_active&&!a.turn_banner.player_active);
 CHECK(a.open_ftue(FtueId::CombatRules));
 a.animation+=152;a.map_clock_ms+=5016;a.sync_turn_banner();
 CHECK(!a.turn_banner.battle_active&&!a.turn_banner.player_active);unchanged();
 a.ftue_control(3);a.ftue_control(3);CHECK(a.ftue.active());a.ftue_control(3);
 CHECK(!a.ftue.active()&&a.seen_ftue(FtueId::CombatRules)&&a.ftue.banner);
 a.sync_turn_banner();CHECK(a.turn_banner.battle_active&&a.turn_banner.battle_start==a.animation);unchanged();
 verify_single();CHECK(!a.ftue.banner);
}
static void smith(){
 scenario="Smith only animates the committed upgraded card";fresh();auto&s=a.game;
 CHECK(world_begin_run(s,reference::ActKind::Overgrowth,0));s.original_events={};auto&w=s.world;
 w.room_type=uint8_t(WorldRoom::Rest);w.room_open=1;w.rest_count=1;w.rest_options[0]=uint8_t(WorldRestOption::Smith);w.rest_used=0;w.rest_pending=0;
 s.phase=Phase::Camp;a.reset_presentation();a.map_clock_ms=600;a.cursor=0;
 CHECK(a.camp_input(Key::Confirm));CHECK(s.phase==Phase::Upgrade&&a.room_presentation.rest.selection.pending);
 a.deck_candidate=0;a.deck_choice_revision=s.revision;a.deck_choice_phase=s.phase;CHECK(a.deck_candidate_valid());
 direct=s;CHECK(upgrade_card(direct,0));a.ui_action(UiAction::ConfirmDeckChoice);CHECK(!std::memcmp(&s,&direct,sizeof s));
 const auto&view=a.reward_presentation.upgrades;CHECK(view.count==1&&view.cards[0].card.kind==Strike&&view.cards[0].card.upgraded);
 CHECK(view.start==600&&view.cards[0].start==600&&view.cards[0].deck==0&&view.cards[0].style==RewardPresentation::Upgrades::Smith);
 CHECK(view.alive(3599)&&!view.alive(3600));
 before=s;Canvas c{a};
 // NCardSmithVfx reveals for250ms, strikes at250/500/750, then
 // hands this exact upgraded snapshot to NCardFlyVfx at1250ms.
 for(unsigned age:{0u,1u,125u,249u,250u,300u,499u,500u,550u,749u,750u,800u,1249u,1250u,1350u,2999u,3000u}){
  a.map_clock_ms=600+age;upgraded_card_animation(c);unchanged();
  CHECK(view.cards[0].launched==(age>=1250));
  unsigned flights=0;for(const auto&flight:a.reward_presentation.flights)if(flight.kind!=RewardPickupKind::None){
   ++flights;CHECK(flight.kind==RewardPickupKind::Card&&flight.start==1850&&flight.slot==0&&flight.has_snapshot);
   CHECK(card_token(flight.card)==card_token(s.deck[0])&&card_token(flight.snapshot.card)==card_token(s.deck[0]));
  }
  CHECK(flights==unsigned(age>=1250));
  upgraded_card_animation(c);CHECK(view.count==1);unchanged();
 }
 a.reset_presentation();CHECK(!a.reward_presentation.upgrades.count);
 scenario="stale or canceled Smith cannot create upgrade effects";fresh();s.phase=Phase::Upgrade;a.reset_presentation();
 a.deck_candidate=0;a.deck_choice_revision=s.revision+1;a.deck_choice_phase=s.phase;before=s;
 a.ui_action(UiAction::ConfirmDeckChoice);CHECK(!a.reward_presentation.upgrades.count);unchanged();
}
static void reward_overlap(){
 scenario="source Normal/Fast ordinary/boss reward clock retains the command barrier";
 for(unsigned boss=0;boss<2;++boss)for(unsigned fast=0;fast<2;++fast)for(unsigned barrier:{0u,20u}){
  fresh();CHECK(encounter_setup(a.game,boss?EncounterId::AeonglassBoss:EncounterId::NibbitsNormal));a.fast_mode=fast!=0;
  a.sync_enemy_feedback();CHECK(a.combat_status.had_live_scene);const uint32_t began=a.animation;
  if(barrier){a.begin_combat_feedback();CHECK(ui_feedback_bound(a));
   // Binding this fixture's first status projection resets the previous
   // visual identity. Observe the actual live scene before killing it.
   a.sync_enemy_feedback();CHECK(a.combat_status.had_live_scene);
   a.combat_feedback.finished=true;a.combat_feedback.end_at=began+barrier;}
  for(unsigned i=0;i<a.game.enemy_count;++i){a.game.enemies[i].hp=0;auto&m=a.enemy_presentation[i];CHECK(m.clip>=0);
   m.hp=0;m.dead=true;m.loop=false;m.until=began+(i?41u:121u);m.count=0;m.direct=false;m.next_clip=nullptr;
   if(barrier)a.combat_feedback.shown[i+1].hp=a.combat_feedback.observed[i+1].hp=0;
  }
  a.game.phase=Phase::Reward;a.enemy_visual_clock=began+1000;before=a.game;a.sync_enemy_feedback();
  // The longest finite track ends at121. Normal adds31 ticks (1000ms);
  // Fast ordinary adds16 (500ms); Fast boss halves the101/121 tick remainder.
  const uint32_t expected=began+(fast?(boss?barrier+(122u-barrier)/2u:barrier+16u):152u);
  if(!a.combat_status.reward_scheduled||a.combat_status.reward_at!=expected)
   std::fprintf(stderr,"reward case boss=%u fast=%u barrier=%u: scheduled=%d live=%d at=%u expected=%u\n",boss,fast,barrier,int(a.combat_status.reward_scheduled),int(a.combat_status.had_live_scene),a.combat_status.reward_at,expected);
  CHECK(a.combat_status.reward_scheduled&&a.combat_status.reward_at==expected);
  CHECK(int32_t(expected-(began+barrier))>=0);unchanged();
  ++a.animation;a.sync_enemy_feedback();CHECK(a.combat_status.reward_at==expected);unchanged();
  a.animation=expected;a.sync_enemy_feedback();CHECK(a.combat_status.reward_at==expected);unchanged();
 }
 scenario="held dead loops and absent clips cannot prolong the reward clock";
 for(unsigned boss=0;boss<2;++boss)for(unsigned fast=0;fast<2;++fast)for(unsigned missing=0;missing<2;++missing){
  fresh();CHECK(encounter_setup(a.game,boss?EncounterId::AeonglassBoss:EncounterId::NibbitsNormal));a.fast_mode=fast!=0;
  a.sync_enemy_feedback();const uint32_t began=a.animation;
  for(unsigned i=0;i<a.game.enemy_count;++i){a.game.enemies[i].hp=0;auto&m=a.enemy_presentation[i];
   m.hp=0;m.dead=true;m.loop=true;m.until=began+1000000u;m.count=0;m.direct=false;m.next_clip=nullptr;if(missing)m.clip=-1;
  }
  a.game.phase=Phase::Reward;before=a.game;a.sync_enemy_feedback();
  const uint32_t expected=began+(fast?(boss?0u:16u):31u);
  CHECK(a.combat_status.reward_scheduled&&a.combat_status.reward_at==expected);unchanged();
  a.animation=expected+1;a.sync_enemy_feedback();CHECK(a.combat_status.reward_at==expected);unchanged();
 }
}
static std::vector<uint8_t>read(const std::string&path){FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long n=std::ftell(f);CHECK(n>0);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>v(size_t(n),0);CHECK(std::fread(v.data(),1,v.size(),f)==v.size());CHECK(!std::fclose(f));return v;}
static void merchant_cache(){
 scenario="actual merchant normal card uses exact existing cache";fresh();a.game.phase=Phase::Shop;Canvas c{a};
 invalidate_card_face_cache();
 // Use every actual slot, not only a central card. On an identical plain
 // backdrop moving between central and edge slots also tests padded-window
 // identity; an incorrect replay shifts the complete face by five pixels.
 for(int slot:{2,4,0,5,6,4,1,3})for(bool focused:{false,true}){
  const int w=focused?MerchantInventoryLayout::FocusW:MerchantInventoryLayout::CardW;
  const int h=focused?MerchantInventoryLayout::FocusH:MerchantInventoryLayout::CardH;
  const int x=MerchantInventoryLayout::card_x(slot)-w/2;
  const int y=MerchantInventoryLayout::card_y(slot)-(focused&&slot>=5?12:0)-h/2;
  for(unsigned salt:{0u,1u}){
   const auto background=[&](){for(unsigned i=0;i<320u*240u;++i)a.pixels[i]=salt?uint16_t((i*3917u)^(i>>2)^0x8410):0x2925;};
   background();render_card_face(c,{Strike,0},x,y,w,h);const std::vector<uint16_t>expected(a.pixels,a.pixels+320*240);
   for(unsigned repeat=0;repeat<2;++repeat){begin_card_face_frame();background();draw_card_face(c,{Strike,0},x,y,w,h);
    CHECK(!std::memcmp(expected.data(),a.pixels,sizeof a.pixels));
    bool cached=false;for(const auto&entry:small_card_faces)cached|=entry.valid&&entry.width==w&&entry.height==h&&entry.seen==small_card_frame;CHECK(cached);
   }
  }
 }
 CHECK(a.content.healthy());
}
static void actual_label_metrics(){
 scenario="specialized combat text agrees with the ordinary font layout";fresh();Canvas c{a};
 const auto clear=[](){for(auto&pixel:a.pixels)pixel=0x2925;};
 for(unsigned face:{0u,5u})for(const char*text:{"80/80","999/999","AVATAR","Strength","Vulnerable"}){
  c.font_face=uint8_t(face);const int size=a.content.font_size(10,face),width=c.width(text,size);
  clear();c.text(text,40+(180-width)/2,50,size,White);const std::vector<uint16_t>expected(a.pixels,a.pixels+320*240);
  // Matching the flat backdrop hides only the outline, allowing an exact
  // independent comparison of glyph placement, bearings and kerning.
  clear();health_label(c,text,40,50,180,size,White,0x2925,true);
  CHECK(!std::memcmp(expected.data(),a.pixels,sizeof a.pixels));
 }
 c.font_face=4;c.sparse_scene_sizes=true;
 for(const char*text:{"BATTLE START","ENEMY TURN","PLAYER TURN","AVATAR"}){
  const int size=a.content.font_size(18,c.font_face,false,true);
  clear();c.text(text,160-c.width(text,size)/2,50,size,White);const std::vector<uint16_t>expected(a.pixels,a.pixels+320*240);
  clear();turn_banner_text(c,text,160,50,4096,size,White);
  CHECK(!std::memcmp(expected.data(),a.pixels,sizeof a.pixels));
 }
 c.font_face=uint8_t(ui_intent_face(a));c.sparse_scene_sizes=false;
 for(const char*text:{"123","x3","9999"})for(int size:{ui_intent_size(a),ui_intent_size(a,true)})
  CHECK(ui_intent_text_width(a,text,size)==c.width(text,size));
 CHECK(a.content.healthy());
}
static void actual_aimed_descriptions(){
 scenario="actual hand cache changes damage only while aiming";fresh();a.game.enemies[0].vulnerable=2;before=a.game;Canvas c{a};
 invalidate_hand_tiles();begin_card_face_frame();a.target=0;const std::string base=hand_tile(c,0).description;
 a.cursor=0;a.input(Key::Confirm);const std::string aimed=hand_tile(c,0).description;
 CHECK(aimed!=base&&std::strstr(aimed.c_str(),"9"));CHECK(base==hand_tile(c,1).description);unchanged();
 a.input(Key::Back);CHECK(base==hand_tile(c,0).description);unchanged();CHECK(a.content.healthy());
 begin_card_face_frame();CHECK(std::string(hand_tile(c,0,nullptr,-1,nullptr,false,"detached transform snapshot").description)=="detached transform snapshot");
 CHECK(base==hand_tile(c,0).description);unchanged();
}
static void actual_winning_card(){
 scenario="a real winning card keeps the original Fast500ms reward wait";fresh();CHECK(encounter_setup(a.game,EncounterId::NibbitsNormal));
 a.game.cards[0]={Thunderclap,0};for(unsigned i=0;i<a.game.enemy_count;++i)a.game.enemies[i].hp=1;a.fast_mode=true;
 a.sync_combat_preparation();a.combat_preparation.shown=true;for(unsigned i=0;i<256&&a.combat_preparation.active;++i)a.step_combat_preparation();CHECK(!a.combat_preparation.active);
 direct=a.game;a.begin_combat_feedback(0);{UiCoreObserver observer(a);CHECK(play(a.game,0,0));}
 CHECK(play(direct,0,0));canonical_equal(a.game,direct);CHECK(a.game.phase==Phase::Reward&&a.combat_feedback.finished);
 before=a.game;for(unsigned i=0;i<256&&!a.combat_status.reward_scheduled;++i){a.advance(33);a.sync_enemy_feedback();}
 CHECK(a.combat_status.reward_scheduled);const auto ready=a.combat_status.reward_at;
 CHECK(int32_t(ready-a.combat_feedback.end_at)>=16&&ready<=ui_feedback_later(a.animation,a.combat_feedback.end_at)+16u);
 while(int32_t(ready-a.animation)>0){a.advance(33);a.sync_enemy_feedback();}unchanged();
 bool retained=false;for(unsigned i=0;i<a.game.enemy_count;++i)retained|=a.enemy_opacity(int(i))>0;
 CHECK(retained&&int32_t(a.combat_linger_until-a.animation)<=0);CHECK(a.content.healthy());
}
static void actual_selected_description(){
 scenario="a selected long description reveals its final value without disturbing static faces";
 fresh();a.game.cards[0]={Rage,0};a.game.deck[0]={Rage,0};a.cursor=0;a.input(Key::Confirm);CHECK(a.selected_hand_card()==0);before=a.game;
 Canvas c{a};begin_card_face_frame();const auto&base=hand_tile(c,0);CHECK(base.description_truncated);
 const auto build=base.built;const std::vector<uint16_t> original(base.pixels,base.pixels+HandTileW*HandTileH);
 const auto neighbour=hand_tile(c,1).built;
 const auto&first=selected_hand_tile(c,0);CHECK(selected_hand_reader.skip==0&&selected_hand_reader.maximum>0);
 const std::vector<uint16_t> initial(first.pixels,first.pixels+HandTileW*HandTileH);
 const auto rect=card_face::layout(HandFaceX,HandFaceY,HandFaceW,HandFaceH).description;
 const int maximum=selected_hand_reader.maximum;
 for(int skip=1;skip<=maximum;++skip){
  a.map_clock_ms=selected_hand_reader.started+1400u+unsigned(skip)*125u;begin_card_face_frame();const auto&current=selected_hand_tile(c,0);CHECK(selected_hand_reader.skip==skip);
  unsigned changed=0;for(int y=0;y<HandTileH;++y)for(int x=0;x<HandTileW;++x){const unsigned at=y*HandTileW+x;
   if(x<rect.x||x>=rect.x+rect.w||y<rect.y||y>=rect.y+rect.h)CHECK(initial[at]==current.pixels[at]);else changed+=initial[at]!=current.pixels[at];}
  CHECK(changed>10);CHECK(base.built==build&&!std::memcmp(base.pixels,original.data(),sizeof base.pixels));CHECK(hand_tile(c,1).built==neighbour);unchanged();
 }
 a.map_clock_ms=selected_hand_reader.started+2800u+unsigned(maximum)*250u;begin_card_face_frame();selected_hand_tile(c,0);CHECK(!selected_hand_reader.skip);unchanged();
 a.input(Key::Back);CHECK(a.selected_hand_card()<0);a.hand_focus=false;a.render();CHECK(selected_hand_reader.instance<0);unchanged();CHECK(a.content.healthy());
}
int main(int argc,char**argv){CHECK(argc==1||argc==2);aimed_preview();numeric_selection();denied_plays();battle_entry();smith();reward_overlap();
 if(argc==2){const std::string root=argv[1];auto common=read(root+"/data/nSpire2.dat.tns"),language=read(root+"/data/nSpire2.eng.dat.tns");SplitResourceSource split;
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage::English));CHECK(a.init(split.source(),{}));
  aimed_preview();denied_plays();battle_entry();actual_single_battle_banner();smith();reward_overlap();merchant_cache();actual_label_metrics();actual_aimed_descriptions();actual_winning_card();actual_selected_description();
 }
 std::printf("combat UI12: %u checks passed%s\n",checks,argc==2?"; actual source cards rendered":"");
}
