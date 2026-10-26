// Actual private-resource gate. This fixture enters the production event,
// renders all source periods and operates the real merchant hit regions. It
// is not a natural run or a calculator-performance measurement.
#include "app.hpp"
#include "content_fixture.hpp"
#include "fake_merchant_assets_ui13.hpp"
#include "fake_merchant_presentation.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

using namespace spire;
static App app;
static SaveData before,after;
static unsigned checks=0;
static const char*captures=nullptr;
#define CHECK(value) do{++checks;if(!(value)){std::fprintf(stderr,"Fake Merchant UI13: %s:%d: %s\n",__FILE__,__LINE__,#value);std::exit(1);}}while(0)
static unsigned u16(const uint8_t*p){return p[0]|unsigned(p[1])<<8;}
static std::vector<uint32_t> picture(Picture p){
 CHECK(p.pixels&&p.w&&p.h);std::vector<uint32_t>out(unsigned(p.w)*p.h);
 if(p.format==4){const uint8_t*row=p.pixels;for(unsigned y=0;y<p.h;++y){unsigned spans=u16(row);row+=2;
  for(unsigned i=0;i<spans;++i){const unsigned x=u16(row),n=u16(row+2),mode=row[4];row+=5;CHECK(x+n<=p.w);
   for(unsigned j=0;j<n;++j){const unsigned a=mode?row[2]:255;out[y*p.w+x+j]=a?(a<<16)|u16(row):0;row+=mode?3:2;}}}
 }else for(unsigned i=0;i<out.size();++i){const uint8_t*pixel=p.palette?p.palette+2*p.pixels[i]:p.pixels+2*i;
  const unsigned a=p.alpha?p.alpha[i]:255;out[i]=a?(a<<16)|u16(pixel):0;}
 return out;
}
static uint64_t hash(const std::vector<uint32_t>&data){uint64_t h=1469598103934665603ULL;for(auto value:data)h=(h^value)*1099511628211ULL;return h;}
static uint64_t pixels(int x,int y,int w,int h){uint64_t value=1469598103934665603ULL;
 for(int row=y;row<y+h;++row)for(int col=x;col<x+w;++col)value=(value^app.pixels[row*ScreenW+col])*1099511628211ULL;return value;}
static void snapshot(){CHECK(validate(app.game));CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after));CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static void advance(unsigned ms){while(ms){const unsigned step=ms>250?250:ms;app.advance(step);ms-=step;}app.render();CHECK(app.content.healthy());}
static HitRegion action(UiAction wanted){app.render();HitRegion hit{};unsigned count=0;
 for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(wanted)){hit=app.hits[i];++count;}
 CHECK(count==1&&hit.enabled);return hit;}
static void click(HitRegion h){app.pointer_move(h.x+h.w/2,h.y+h.h/2);app.pointer_button(true);app.pointer_button(false);app.render();}
static void capture(const char*kind,unsigned character){
 if(!captures)return;char path[1024];CHECK(std::snprintf(path,sizeof path,"%s/fake-%u-%s.ppm",captures,character,kind)<int(sizeof path));
 FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[ScreenW*3];
 for(unsigned y=0;y<ScreenH;++y){for(unsigned x=0;x<ScreenW;++x){const uint16_t p=app.pixels[y*ScreenW+x];
  row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}
  CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(std::fclose(f)==0);
}
static void original_sequences(){
 // Independently reconstructed from every pinned source static/moving pair
 // after LCD quantization. The source foreground has only 11 distinct views
 // across its full 144 samples; a generic animation-density threshold is wrong.
 static constexpr unsigned source_distinct[]={133,11,80,80,259,89,288,290,144,133};
 const bool full=app.content.ui_image(I13_FakeMerchantWithoutRug)>=0;unsigned frames=0;
 for(unsigned n=0;n<(full?10u:9u);++n){const auto&s=FakeMerchantSequences[n];const int fixed_id=app.content.ui_image(s.base);CHECK(fixed_id>=0);
  const unsigned width=n==FakeMerchantRegentSequence?FakeMerchantRegentAtlasWidth:unsigned(s.w);
  const unsigned height=n==FakeMerchantRegentSequence?FakeMerchantRegentAtlasHeight:unsigned(s.h);
  const auto fixed_picture=app.content.image(fixed_id);CHECK(fixed_picture.w==width&&fixed_picture.h==height);
  auto fixed=picture(fixed_picture);CHECK(fixed.size()==width*height);std::set<uint64_t>distinct;
  for(unsigned frame=0;frame<s.count;++frame){const int image=app.content.ui_image(s.base+1+frame);CHECK(image>=0);
   const auto p=app.content.image(image);CHECK(p.w==width&&p.h==height);auto moving=picture(p);unsigned ink=0;
   for(unsigned i=0;i<moving.size();++i){CHECK(!(fixed[i]>>16)||!(moving[i]>>16));moving[i]|=fixed[i];ink+=bool(moving[i]>>16);}
   CHECK(ink>100);
   if(n==FakeMerchantRegentSequence)for(const auto&band:FakeMerchantRegentBands){unsigned band_ink=0;
    for(int y=band.sy;y<band.sy+band.h;++y)for(int x=band.sx;x<band.sx+band.w;++x)band_ink+=bool(moving[y*width+x]>>16);
    if(band.blend)CHECK(band_ink>0);
   }
   distinct.insert(hash(moving));++frames;
  }
  CHECK(distinct.size()==source_distinct[n]);CHECK(fake_merchant_frame(n,0)==0);
  CHECK(fake_merchant_frame(n,uint64_t(s.count)*1000000/24+1)==0);
 }
 CHECK(frames==(full?1800u:1656u));CHECK(app.content.healthy());
 CHECK(app.content.image(app.content.ui_image(I13_FakeMerchantNoise)).w==50);
 const auto glitter=app.content.image(app.content.ui_image(I13_FakeMerchantGlitter));CHECK(glitter.w==146&&glitter.h==195);
 const auto square=app.content.image(app.content.ui_image(I13_FakeMerchantSquareSpeck));CHECK(square.w==24&&square.h==32);
}
static void particle_schedule(){
 using namespace fake_merchant;Squares stepped,jumped;
 for(unsigned t=0;t<=Squares::Preprocess+2345;++t)stepped.advance(t,0x61eac39u);
 jumped.advance(Squares::Preprocess+2345,0x61eac39u);
 for(unsigned i=0;i<Squares::Count;++i){const auto&a=stepped.particles[i];const auto&b=jumped.particles[i];
  CHECK(a.active&&b.active);CHECK(a.age==b.age&&a.birth==b.birth&&a.seed==b.seed);
  CHECK(a.x==b.x&&a.y==b.y&&a.vx==b.vx&&a.vy==b.vy);
  CHECK(a.scale>=300&&a.scale<=1000&&a.angular>=-160&&a.angular<=160);
 }
 const auto once=jumped;jumped.advance(Squares::Preprocess+2345,0x61eac39u);
 CHECK(jumped.tick==once.tick);for(unsigned i=0;i<Squares::Count;++i)CHECK(jumped.particles[i].x==once.particles[i].x&&jumped.particles[i].y==once.particles[i].y);
 CHECK(curve(SquareScale,0)==0&&curve(SquareScale,Q)==0);CHECK(curve(SquareAlpha,0)==Q&&curve(SquareAlpha,Q)==0);
}
static void event(unsigned character){
 new_run(app.game,0x71351u+character,false,Character(character));CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));
 app.game.original_events={};CHECK(original_event_begin(app.game,OriginalEventId::FakeMerchant,23+character,1));app.game.gold=1000;
 app.title=app.paused=app.help=app.library=app.character_select=app.run_settings=false;app.reset_presentation();
 app.cursor=1;app.pointer.visible=false;app.render();advance(5000);snapshot();capture("closed",character);
 const uint64_t initial_player=pixels(20,40,112,150),initial_body=pixels(195,80,70,110);
 advance(750);CHECK(pixels(20,40,112,150)!=initial_player);CHECK(pixels(195,80,70,110)!=initial_body);unchanged();capture("animated",character);
 app.pointer_move(319,23);app.render();const auto unfocused=pixels(193,82,75,108);
 app.pointer_move(224,130);app.render();CHECK(pixels(193,82,75,108)!=unfocused);unchanged();capture("focused",character);
 // A repeated draw must neither re-simulate preprocessed particles nor move
 // a retained actor. Pointer ownership is settled before the two frames.
 app.pointer_move(319,23);app.render();const auto still=pixels(0,22,320,218);app.render();CHECK(pixels(0,22,320,218)==still);unchanged();
 click(action(UiAction::OpenShop));CHECK(app.shop_open);advance(1000);capture("inventory",character);unchanged();
 HitRegion stock{};unsigned count=0;for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::Choice){++count;if(!app.hits[i].value)stock=app.hits[i];}
 CHECK(count==6&&stock.enabled);const auto relic=RelicId(app.game.original_events.offers[0]);const int price=app.game.original_events.choice_prices[0],gold=app.game.gold;
 click(stock);CHECK(has_world_relic(app.game,relic));CHECK(app.game.gold==gold-price&&app.game.original_events.offers[0]==NoWorldRelic);CHECK(validate(app.game));
 advance(1000);snapshot();click(action(UiAction::CloseShop));CHECK(!app.shop_open);advance(800);unchanged();capture("returned",character);
 click(action(UiAction::LeaveShop));CHECK(app.map_window.visible&&world_can_travel(app.game));CHECK(validate(app.game));capture("map",character);
 snapshot();app.close_map_window(false);app.render();advance(700);CHECK(!app.event_body_active()&&!app.shop_open);
 HitRegion proceed{};unsigned choices=0;for(int i=0;i<app.hit_count;++i){const auto&h=app.hits[i];
  CHECK(!(h.kind==HitKind::UiAction&&h.value==int(UiAction::OpenShop)));
  if(h.kind==HitKind::Choice){proceed=h;++choices;}}
 CHECK(choices==1&&proceed.value==0&&proceed.enabled);unchanged();capture("finished-room",character);
 click(proceed);CHECK(app.map_window.visible);unchanged();
}
static void after_fight(bool obtain_rug){
 // The encounter is entered by the actual potion command. The win is an
 // explicit controlled outcome, as in the event-reward correctness gate;
 // it is not counted as a player battle or a naturally earned victory.
 new_run(app.game,0x71383u,false,Character::Ironclad);CHECK(world_begin_run(app.game,reference::ActKind::Overgrowth,0));
 app.game.original_events={};CHECK(potion_set(app.game,0,Potion::FoulPotion));
 CHECK(original_event_begin(app.game,OriginalEventId::FakeMerchant,23,1));CHECK(use_potion(app.game,0,-1));
 CHECK(app.game.enemy_world.encounter==EncounterId::FakeMerchantEventEncounter&&app.game.original_events.combat_pending);
 for(unsigned i=0;i<app.game.enemy_count;++i)app.game.enemies[i].hp=0;resolve_pending(app.game);
 auto&w=app.game.world_relics;CHECK(w.choice==WorldRelicChoice::OfferReward);CHECK(validate(app.game));
 if(obtain_rug){uint16_t selected=0xffff;
  for(unsigned i=0;i<w.offer_count;++i){const auto&r=w.rewards[w.offers[i]];if(r.kind==2&&r.value==WorldRelicFakeMerchantsRug)selected=uint16_t(i);}
  CHECK(selected!=0xffff&&world_relic_select(app.game,&selected,1));CHECK(has_world_relic(app.game,WorldRelicFakeMerchantsRug));
 }
 CHECK(world_relic_select(app.game,nullptr,0));CHECK(world_finish_room(app.game));CHECK(app.game.phase==Phase::Event&&app.game.original_events.finished);
 app.title=app.paused=app.help=app.library=app.character_select=app.run_settings=false;app.reset_presentation();app.pointer.visible=false;
 app.render();advance(1800);snapshot();CHECK(!app.event_body_active()&&!app.shop_open);CHECK(app.content.healthy());
 for(int i=0;i<app.hit_count;++i)CHECK(!(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(UiAction::OpenShop)));
 const auto absent=pixels(194,85,70,92);int16_t wares[6];
 for(unsigned i=0;i<6;++i){wares[i]=app.game.original_events.values[2+i];app.game.original_events.values[2+i]=0;}
 app.render();CHECK(pixels(194,85,70,92)!=absent);
 for(unsigned i=0;i<6;++i)app.game.original_events.values[2+i]=wares[i];app.render();unchanged();
 capture(obtain_rug?"fight-rug-obtained":"fight-rug-retained",0);
 CHECK(app.content.healthy());
}
int main(int argc,char**argv){
 CHECK(argc>=2);if(argc>2)captures=argv[2];FILE*f=std::fopen(argv[1],"rb");CHECK(f);
 std::vector<uint8_t>data(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));
 CHECK(app.init(data.data(),data.size(),{}));CHECK(app.content.current_profile());CHECK(app.content.ui_image(I13_FakeMerchantSquareSpeck)>=0);
 particle_schedule();original_sequences();for(unsigned character=0;character<5;++character)event(character);after_fight(false);after_fight(true);
 std::printf("Fake Merchant UI13: %u checks; complete source frames, exact disjoint planes,5 original actors,60 bounded square particles, real open/buy/close/map flow and controlled after-fight/rug variants; actual resources\n",checks);
}
