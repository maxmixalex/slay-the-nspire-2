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
 // Only untouched historical calculator bindings migrate. A custom choice
 // in either bank remains intact when loading a saved preference file.
 preferences.bindings=default_input_bindings(false);preferences.bindings_saved=true;const uint16_t cards[]={'1','3','7','9','0','Z','X','C','V','B'};
 for(auto&b:preferences.bindings.keys){b[2]='M';b[4]='Q';b[6]='D';b[7]='F';b[10]=' ';b[11]='8';b[12]='2';b[13]='4';b[14]='6';for(unsigned i=0;i<10;++i)b[15+i]=cards[i];}
 preferences.bindings.keys[1][0]='5';app.calculator_keyboard=true;app.callbacks.load_preferences=load_preferences;app.restore_preferences();CHECK(!std::memcmp(&keys,&app.input_bindings,sizeof keys));
 preferences.bindings.keys[0][2]='Y';app.restore_preferences();CHECK(!std::memcmp(&preferences.bindings,&app.input_bindings,sizeof keys));app.callbacks.load_preferences=nullptr;
 const auto metric=[](const RichChar&ch,int){return fitted_text::Metric{6,6,352,ch.previous?-32:0};};
 CHECK(fitted_text::line_count("AVAVA",15,10,metric)==3);const fitted_text::Plan plan{10,3,3,11,false};unsigned n=0;
 fitted_text::visit("AVAVA",15,plan,metric,[&](const RichChar&,int x,int row,int){CHECK(row==int(n/2)&&x==int(n%2)*5);++n;});CHECK(n==5);
 CHECK(valid_glyph_advance('A',0x8000|352,40)&&!valid_glyph_advance('A',0x8000|352,39));
 CHECK(!valid_glyph_advance('A',0x8000|2561,40));CHECK(valid_glyph_advance(kerning_codepoint('A','V'),0x4000|2040,40));
 CHECK(!valid_glyph_advance(kerning_codepoint('A','V'),12,40));
 std::vector<uint8_t>pack;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);pack.resize(spire_test::pack_size(f));CHECK(std::fread(pack.data(),1,pack.size(),f)==pack.size());std::fclose(f);CHECK(app.init(pack.data(),pack.size(),{}));CHECK(app.content.version()==40);
  CHECK(app.content.glyph_metrics(kerning_codepoint('A','V'),20).advance==0);
  Canvas canvas{app,app.pixels,320,320,240};
  if(app.content.english()){CHECK(app.content.glyph_kerning64('T','o',10)!=0);const auto a=app.content.glyph_metrics('a',10);CHECK(a.advance64&&a.advance64%64);
   for(int face:{0,4,5,6})for(int size:{8,10,12,14,18}){canvas.font_face=uint8_t(face);std::printf("font-width face=%d size=%d width=%d\n",face,size,canvas.width("AVATAR To Wa fj",size));}}
 }
 new_run(app.game,0x121212,false);app.game.phase=Phase::Map;app.title=app.character_select=false;app.reset_presentation();CHECK(add_potion_slots(app.game,14));CHECK(potion_slot_count(app.game)==16);
 for(unsigned i=0;i<16;++i)potion_set(app.game,i,Potion::Strength);CHECK(save(app.game,before));
 for(unsigned page=0;page<4;++page){app.potion_page=uint8_t(page);app.render();unsigned count=0;
  for(int i=0;i<app.hit_count;++i){const auto&h=app.hits[i];if(h.kind!=HitKind::PotionSlot)continue;++count;
   CHECK(h.value/5==int(page));CHECK(h.x>=114&&h.x+h.w<=194);CHECK(h.x+h.w/2==hud_potion_layout::landing_x(unsigned(h.value),page));}
  CHECK(count==(page==3?1u:5u));auto portrait=hit(HitKind::HudInfo,HudPortrait);CHECK(portrait.x==0&&portrait.x+portrait.w<=26);
 }
 app.potion_page=0;for(unsigned i=0;i<3;++i)click(HitKind::PotionPage,1);CHECK(app.potion_page==3);
 app.open_potion(15);CHECK(app.potion_page==3&&app.panel==4);app.close_potion();
 for(unsigned relic:{0u,23u,24u,40u})CHECK(hud_relic_layout::center_y(relic)>=22);
 CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));if(argc>1)CHECK(app.content.healthy());
 std::printf("UI12 LCD input, font wrapping and sixteen-slot inventory: %u checks passed\n",checks);
}
