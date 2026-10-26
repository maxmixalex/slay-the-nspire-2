// Actual input ownership, fractional wrap and five-slot inventory boundaries.
// Native correctness only. Visual and timing acceptance is performed in Firebird.
#include "../src/app.cpp"
#include "calculator_pad_input.hpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App app;static SaveData before,after;static UiPreferences preferences;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"UI12 LCD line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static bool load_preferences(UiPreferences&out){out=preferences;return true;}
static HitRegion hit(HitKind kind,int value){app.render();for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==kind&&app.hits[i].value==value)return app.hits[i];CHECK(false);return {};}
static void click(HitKind kind,int value){auto h=hit(kind,value);app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);}
int main(int argc,char**argv){
 const auto keys=default_input_bindings(true);CHECK(valid_input_bindings(keys));
 for(unsigned bank=0;bank<2;++bank){for(unsigned i=0;i<10;++i)CHECK(keys.keys[bank][unsigned(InputCommand::Card1)+i]==(i==9?'0':'1'+i));
  CHECK(keys.keys[bank][unsigned(InputCommand::Up)]==InputUp);CHECK(keys.keys[bank][unsigned(InputCommand::Map)]==9);
  CHECK(keys.keys[bank][unsigned(InputCommand::Deck)]==InputDoc);CHECK(keys.keys[bank][unsigned(InputCommand::EndTurn)]==InputMenu);}
 CHECK(keys.keys[1][unsigned(InputCommand::Select)]==InputPad);
 char label[16]{};input_key_label(InputFlag,label,sizeof label,true);CHECK(uint8_t(label[0])==0xee&&label[3]==0);
 CalculatorPadInput pad;unsigned pressed=0,released=0;uint16_t observed=0;
 const auto emit=[&](uint16_t key,bool down){observed=key;if(down)++pressed;else ++released;};
 TouchSample s{50,50,100,100,true,true,true};CHECK(!pad.sample(s,0,false,emit));s.x=0;
 CHECK(!pad.sample(s,100,false,emit)&&pressed==0);s.pressed=false;CHECK(!pad.sample(s,120,false,emit));
 s.pressed=true;CHECK(pad.sample(s,200,false,emit)&&observed==InputLeft&&pressed==1);
 CHECK(pad.sample(s,599,false,emit)&&pressed==1);CHECK(pad.sample(s,600,false,emit)&&pressed==2);
 s.x=101;CHECK(pad.sample(s,620,false,emit)&&released==1);CHECK(!pad.pressed);
 s.x=50;s.y=100;CHECK(pad.sample(s,700,false,emit)&&observed==InputUp);s.pressed=false;pad.sample(s,720,false,emit);
 s.y=50;s.pressed=true;CHECK(pad.sample(s,800,true,emit)&&observed==InputPad);const auto count=pressed;
 CHECK(pad.sample(s,2000,true,emit)&&pressed==count);s.pressed=false;pad.sample(s,2020,true,emit);
 // Each untouched historical calculator bank migrates independently. A
 // customization in the other bank must not strand its numeric navigation.
 preferences.bindings=default_input_bindings(false);preferences.bindings_saved=true;const uint16_t cards[]={'1','3','7','9','0','Z','X','C','V','B'};
 for(auto&b:preferences.bindings.keys){b[2]='M';b[4]='Q';b[6]='D';b[7]='F';b[10]=' ';b[11]='8';b[12]='2';b[13]='4';b[14]='6';for(unsigned i=0;i<10;++i)b[15+i]=cards[i];}
 preferences.bindings.keys[1][0]='5';app.calculator_keyboard=true;app.callbacks.load_preferences=load_preferences;app.restore_preferences();CHECK(!std::memcmp(&keys,&app.input_bindings,sizeof keys));
 preferences.bindings.keys[0][2]='Y';app.restore_preferences();
 CHECK(!std::memcmp(preferences.bindings.keys[0],app.input_bindings.keys[0],sizeof keys.keys[0]));
 CHECK(!std::memcmp(keys.keys[1],app.input_bindings.keys[1],sizeof keys.keys[1]));
 preferences.bindings.keys[1][2]='J';app.restore_preferences();CHECK(!std::memcmp(&preferences.bindings,&app.input_bindings,sizeof keys));app.callbacks.load_preferences=nullptr;
 const auto metric=[](const RichChar&ch,int){return fitted_text::Metric{6,6,352,ch.previous?-32:0};};
 CHECK(fitted_text::line_count("AVAVA",15,10,metric)==3);const fitted_text::Plan plan{10,3,3,11,false};unsigned n=0;
 fitted_text::visit("AVAVA",15,plan,metric,[&](const RichChar&,int x,int row,int){CHECK(row==int(n/2)&&x==int(n%2)*5);++n;});CHECK(n==5);
 CHECK(valid_glyph_advance('A',0x8000|352,40)&&!valid_glyph_advance('A',0x8000|352,39));
 CHECK(!valid_glyph_advance('A',0x8000|2561,40));CHECK(valid_glyph_advance(kerning_codepoint('A','V'),0x4000|2040,40));
 CHECK(!valid_glyph_advance(kerning_codepoint('A','V'),12,40));
 std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.init(pack.data(),pack.size(),{}));CHECK(app.content.version()==40);
  CHECK(app.content.glyph_metrics(kerning_codepoint('A','V'),20).advance==0);
  Canvas canvas{app,app.pixels,320,320,240};
  // The displayed Player Turn counter must use a complete 10px alphabet,
  // including Chinese and the optional locales' shaped private graphemes.
  char turn[96]{};const auto turn_number=[](const void*,const char*key)->LocalizedValue{return wt_equal(key,"turnNumber")?wt_number(7):LocalizedValue{};};
  LocalizedTemplate(turn,sizeof turn,turn_number,nullptr).for_language(app.content.language()).format(app.content.world_string("gameplay_ui","TURN_COUNT"));
  const auto read32=[&](size_t at){return uint32_t(pack[at])|uint32_t(pack[at+1])<<8|uint32_t(pack[at+2])<<16|uint32_t(pack[at+3])<<24;};
  const size_t glyph_at=40+size_t(read32(16))*16;const uint32_t glyph_count=read32(20);const char*cursor=turn;
  while(*cursor){const uint32_t cp=utf8(cursor);if(cp==' '||cp=='\n')continue;bool present=false;
   for(uint32_t i=0;i<glyph_count;++i){const size_t at=glyph_at+size_t(i)*16;const unsigned band=unsigned(pack[at+8])|unsigned(pack[at+9])<<8;
    if(read32(at)==cp&&(band&31u)==10u)present=true;}
   CHECK(present);
  }
  if(app.content.english()){CHECK(app.content.glyph_kerning64('T','o',10)!=0);const auto a=app.content.glyph_metrics('a',10);CHECK(a.advance64&&a.advance64%64);
   for(int face:{0,4,5,6})for(int size:{8,10,12,14,18}){canvas.font_face=uint8_t(face);std::printf("font-width face=%d size=%d width=%d\n",face,size,canvas.width("AVATAR To Wa fj",size));}}
 }
 new_run(app.game,0x121212,false);app.game.phase=Phase::Map;app.title=app.character_select=false;app.reset_presentation();CHECK(add_potion_slots(app.game,14));CHECK(potion_slot_count(app.game)==16);
 for(unsigned i=0;i<16;++i)potion_set(app.game,i,Potion::Strength);CHECK(save(app.game,before));
 for(unsigned page=0;page<4;++page){app.potion_page=uint8_t(page);app.render();unsigned count=0;
  for(int i=0;i<app.hit_count;++i){const auto&h=app.hits[i];if(h.kind!=HitKind::PotionSlot)continue;++count;
   CHECK(h.value/5==int(page));CHECK(h.x>=114&&h.x+h.w<=179);CHECK(h.x+h.w/2==hud_potion_layout::landing_x(unsigned(h.value),page,16));}
  CHECK(count==(page==3?1u:5u));auto portrait=hit(HitKind::HudInfo,HudPortrait);CHECK(portrait.x==0&&portrait.x+portrait.w<=26);
 }
 app.potion_page=0;for(unsigned i=0;i<3;++i)click(HitKind::PotionPage,1);CHECK(app.potion_page==3);
 app.open_potion(15);CHECK(app.potion_page==3&&app.panel==4);app.close_potion();
 for(unsigned relic:{0u,23u,24u,40u})CHECK(hud_relic_layout::center_y(relic)>=22);
 CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));if(argc>1)CHECK(app.content.healthy());
 new_run(app.game,0x131313,false);CHECK(world_begin_run(app.game));auto&w=app.game.world;auto&rooms=w.rooms[w.act_index];auto&map=w.maps[w.act_index];
 rooms.boss_count=2;rooms.bosses[1]=EncounterId::QueenBoss;map.second_boss=uint8_t(map.boss+1);
 for(unsigned slots=2;slots<=16;++slots){if(slots>2)CHECK(add_potion_slots(app.game,1));const auto p=hud_room_geometry(app.game);
  CHECK(p.room-hud_potion_layout::Left-hud_potion_layout::width(slots)==4);CHECK(p.floor-p.room-10==4);CHECK(p.boss-p.floor-22==4);
  CHECK(p.timer-p.boss-17==4&&p.timer+7<=257);
 }
 CHECK(!hud_portrait_enabled(app.game));app.game.world.ascension=1;CHECK(hud_portrait_enabled(app.game));
 app.game.world.ascension=0;app.game.run_options.mode=RunMode::Custom;CHECK(hud_portrait_enabled(app.game));
 app.show_run_timer=false;app.panel=0;app.map_window.visible=false;app.game.phase=Phase::Map;CHECK(app.run_timer_visible());
 app.game.phase=Phase::Combat;CHECK(!app.run_timer_visible());app.show_run_timer=true;CHECK(app.run_timer_visible());
 CHECK(combat_controls::StarX<combat_controls::EnergyX&&combat_controls::StarY>combat_controls::EnergyY);
 CHECK(combat_controls::StarX+combat_controls::StarSize>combat_controls::EnergyX);
 CHECK(combat_controls::ExhaustY>=combat_controls::EndY+combat_controls::EndH&&combat_controls::ExhaustY+combat_controls::PileSize<=combat_controls::DiscardY);
 CHECK(combat_controls::DrawY+26<=240&&combat_controls::DiscardY+26<=240);
 for(int count=1;count<=10;++count)for(int slot=0;slot<count;++slot){const auto pose=hand_pose(count,slot,false);
  for(int y=combat_controls::ExhaustY;y<combat_controls::ExhaustY+combat_controls::PileSize;++y)
   for(int x=combat_controls::ExhaustX;x<combat_controls::ExhaustX+combat_controls::PileSize;++x)CHECK(!hand_contains(pose,x,y));}
 std::printf("UI12 LCD input, font wrapping and sixteen-slot inventory: %u checks passed\n",checks);
}
