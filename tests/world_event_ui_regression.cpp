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
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"world event UI: %s:%d %s (event=%s phase=%u)\n",__FILE__,__LINE__,#x,original_event_key(a.game.original_events.id),unsigned(a.game.phase));std::exit(1);}}while(0)

struct TextIndex {const char* key;uint16_t index;};
static const TextIndex world_text[]={
#include "../src/world_text_index.inc"
};
static const TextIndex world_images[]={
#include "../src/world_image_index.inc"
};
static unsigned image_slot(const char*key){for(const auto&row:world_images)if(!std::strcmp(row.key,key))return row.index;CHECK(false);return 0;}
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
 const unsigned ancient_body=image_slot("events.ANCIENT_OPTION");
 for(unsigned i=0;i<images;i++){
  unsigned w=i?2:82,h=i?2:60,offset=unsigned(b.size()),length=w*h*2;
  put(b,40+i*16,offset);put(b,44+i*16,w,2);put(b,46+i*16,h,2);put(b,52+i*16,length);b.resize(offset+length);
  for(unsigned pixel=0;pixel<w*h;pixel++)put(b,offset+pixel*2,i==ancient_body?0xffff:
      ((i*13+pixel*5)&31)<<11|((i*29+pixel*7)&63)<<5|((i*7+pixel*3)&31),2);
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
static HitRegion choice(int value){
 int found=0;HitRegion result{};for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::Choice&&a.hits[i].value==value){result=a.hits[i];++found;}
 CHECK(found==1);CHECK(result.x>=0&&result.y>=22&&result.w>0&&result.h>0&&result.x+result.w<=ScreenW&&result.y+result.h<=ScreenH);return result;
}
static unsigned choice_count(){unsigned count=0;for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==HitKind::Choice)++count;return count;}
static bool overlap(const HitRegion&x,const HitRegion&y){return x.x<y.x+y.w&&y.x<x.x+x.w&&x.y<y.y+y.h&&y.y<x.y+x.h;}
static uint32_t image_hash(int x,int y,int w,int h){
 CHECK(x>=0&&y>=0&&w>0&&h>0&&x+w<=ScreenW&&y+h<=ScreenH);
 uint32_t hash=2166136261u;for(int row=y;row<y+h;row++)for(int col=x;col<x+w;col++){hash^=a.pixels[row*ScreenW+col];hash*=16777619u;}return hash;
}
static void press_choice(int value){
 a.render();auto hit=choice(value);a.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;
}
static void capture(const char*name){
 if(!capture_directory)return;a.pointer.visible=false;a.render();
 char path[1024]{};int n=std::snprintf(path,sizeof path,"%s/%s.ppm",capture_directory,name);CHECK(n>0&&unsigned(n)<sizeof path);
 FILE*f=std::fopen(path,"wb");CHECK(f);CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[ScreenW*3];
 for(unsigned y=0;y<ScreenH;y++){for(unsigned x=0;x<ScreenW;x++){uint16_t p=a.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void begin(OriginalEventId id,uint64_t seed,unsigned act){
 new_run(a.game,seed,false,Character::Ironclad);CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));
 CHECK(a.game.original_events.id==OriginalEventId::Neow);
 // Substitute the requested model into the already open native event room.
 // Keep that room open so the public Proceed action still leaves it normally.
 if(id!=OriginalEventId::Neow){a.game.original_events=OriginalEventState{};CHECK(original_event_begin(a.game,id,seed,act));}
 a.game.gold=1000;a.title=a.paused=a.help=a.library=a.character_select=false;a.run_settings=false;a.map_preview.open=false;a.reset_presentation();a.pointer.visible=false;CHECK(validate(a.game));
}
static void reader(bool require_scroll){
 snapshot();uint32_t calls=a.game.original_events.rng_calls,revision=a.game.revision;
 a.input(Key::Detail);CHECK(a.event_reader);a.render();a.pointer_scroll(9);a.render();if(require_scroll)CHECK(a.event_reader_scroll>0);
 a.input(Key::Up);a.render();a.ui_action(UiAction::CloseEventReader);CHECK(!a.event_reader);a.render();
 CHECK(a.game.original_events.rng_calls==calls&&a.game.revision==revision);unchanged();
}
static void finished_offer_hidden(){
 a.pointer.visible=false;a.render();CHECK(choice_count()==1);auto hit=choice(0);CHECK(hit.enabled);
 uint32_t old=image_hash(hit.x,hit.y,hit.w,hit.h);auto saved=a.game.original_events.offers[0];snapshot();
 // Finish keeps the old offer array. Changing that retained identity must
 // not change the sole Proceed button or turn it back into a relic offer.
 a.game.original_events.offers[0]=saved==WorldRelicGoldenPearl?WorldRelicFakeAnchor:WorldRelicGoldenPearl;
 a.render();CHECK(image_hash(hit.x,hit.y,hit.w,hit.h)==old);a.game.original_events.offers[0]=saved;unchanged();
}
static void merchant(){
 begin(OriginalEventId::FakeMerchant,23,1);a.render();CHECK(choice_count()==7);HitRegion slots[7];
 for(int i=0;i<7;i++){slots[i]=choice(i);CHECK(slots[i].enabled);if(i<6)CHECK(slots[i].w>=36&&slots[i].h>=36);for(int j=0;j<i;j++)CHECK(!overlap(slots[i],slots[j]));}
 CHECK(slots[0].x<slots[1].x&&slots[2].x<slots[3].x&&slots[3].x<slots[4].x);
 CHECK(slots[0].y<slots[2].y&&slots[1].y<slots[4].y&&slots[2].y<slots[5].y&&slots[3].y<slots[5].y&&slots[4].y<slots[5].y);
 snapshot();for(int i=0;i<7;i++){const auto&h=slots[i];a.pointer_move(h.x+h.w/2,h.y+h.h/2);CHECK(a.cursor==i);a.render();const auto*p=a.hit_at(h.x+h.w/2,h.y+h.h/2);CHECK(p&&p->kind==HitKind::Choice&&p->value==i);unchanged();}
 a.cursor=0;a.pointer.visible=false;a.render();auto h=choice(0);int price=a.game.original_events.choice_prices[0];
 uint32_t old_price=image_hash(h.x,h.y+h.h-14,h.w,14);snapshot();
 a.game.original_events.choice_prices[0]=int16_t(price+1);a.render();CHECK(image_hash(h.x,h.y+h.h-14,h.w,14)!=old_price);
 a.game.original_events.choice_prices[0]=int16_t(price);a.render();unchanged();capture("fake-merchant-stock");reader(true);
 // Affordability comes from the core predicate; an unfunded click is inert.
 a.game.gold=0;snapshot();a.render();for(int i=0;i<6;i++)CHECK(!choice(i).enabled);CHECK(choice(6).enabled);press_choice(0);unchanged();
 a.game.gold=1000;auto bought=RelicId(a.game.original_events.offers[0]);unsigned count=a.game.world_relics.count;int gold=a.game.gold;press_choice(0);
 CHECK(a.game.phase==Phase::Event&&!a.game.original_events.finished);CHECK(a.game.original_events.offers[0]==NoWorldRelic&&has_world_relic(a.game,bought));
 CHECK(a.game.gold==gold-price&&a.game.world_relics.count==count+1);CHECK(validate(a.game));a.advance(1000);a.render();CHECK(choice_count()==7&&!choice(0).enabled);capture("fake-merchant-sold");
 snapshot();press_choice(0);unchanged();press_choice(6);CHECK(a.game.original_events.finished);a.advance(1000);a.render();finished_offer_hidden();capture("fake-merchant-complete");
 count=a.game.world_relics.count;gold=a.game.gold;press_choice(0);
 CHECK(a.game.original_events.active&&a.game.original_events.finished&&a.game.phase==Phase::Event);
 CHECK(world_can_travel(a.game)&&a.map_window.visible&&a.game.world_relics.count==count&&a.game.gold==gold);CHECK(validate(a.game));
}
static void ancient_original_modulation(){
 // A white source fixture exposes the missing original CanvasItem.Modulate
 // that made real ancient buttons opaque white. Compare an untitled,
 // unfocused third-row pixel with the original per-model RGBA constants,
 // independently quantized and composited over the scene background.
 struct Original{OriginalEventId id;unsigned act;double r,g,b,a;};
 const Original originals[]={
  {OriginalEventId::Neow,0,0,.1,.2,.5},{OriginalEventId::Darv,1,.06,0,.08,.5},
  {OriginalEventId::Nonupeipe,2,0,.1,.16,.75},{OriginalEventId::Tanx,2,.05,.02,0,.5},
  {OriginalEventId::Orobas,1,.05,0,.1,.35},{OriginalEventId::Pael,1,.03,.08,0,.75},
  {OriginalEventId::Tezcatara,1,.08,.04,0,.75},{OriginalEventId::Vakuu,2,.05,.06,.12,.8}};
 for(const auto&entry:originals){
  begin(entry.id,0x71ef23,entry.act);a.cursor=0;snapshot();a.render();unchanged();
  Picture background=a.content.image(a.content.world_image("events",original_event_key(entry.id)));
  CHECK(background.pixels&&!background.alpha&&!background.palette&&background.w==2&&background.h==2);
  const unsigned at=3*2;const uint16_t back=uint16_t(background.pixels[at]|unsigned(background.pixels[at+1])<<8);
  const unsigned alpha=unsigned(entry.a*255.+.5);
  const unsigned r=(31*unsigned(entry.r*255.+.5)+127)/255;
  const unsigned g=(63*unsigned(entry.g*255.+.5)+127)/255;
  const unsigned b=(31*unsigned(entry.b*255.+.5)+127)/255;
  const uint16_t expected=uint16_t((((back>>11)*(255-alpha)+r*alpha+127)/255<<11)|
      ((((back>>5)&63)*(255-alpha)+g*alpha+127)/255<<5)|((back&31)*(255-alpha)+b*alpha+127)/255);
  CHECK(a.pixels[234*ScreenW+300]==expected);
 }
}
static void ancients(){
 struct Entry{OriginalEventId id;unsigned act;const char*file;};
 const Entry entries[]={{OriginalEventId::Neow,0,"ancient-neow"},{OriginalEventId::Darv,1,"ancient-darv"},{OriginalEventId::Orobas,1,"ancient-orobas"},{OriginalEventId::Pael,1,"ancient-pael"},{OriginalEventId::Tezcatara,1,"ancient-tezcatara"},{OriginalEventId::Nonupeipe,2,"ancient-nonupeipe"},{OriginalEventId::Tanx,2,"ancient-tanx"},{OriginalEventId::Vakuu,2,"ancient-vakuu"}};
 for(const auto&entry:entries){
  begin(entry.id,0x71ef23,entry.act);a.render();CHECK(choice_count()==3);HitRegion previous{};snapshot();
  for(int i=0;i<3;i++){auto hit=choice(i);CHECK(hit.x==previous.x||!i);CHECK(hit.w>=250&&hit.h>=36);if(i)CHECK(hit.y>=previous.y+previous.h);previous=hit;
   a.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);CHECK(a.cursor==i);a.input(Key::Down);a.render();unchanged();}
  a.cursor=0;a.scroll=0;capture(entry.file);reader(false);
  if(entry.id==OriginalEventId::Orobas){
   // Explicit UI fixture for the original locked third offer, generated by
   // Orobas when neither starter upgrade can be prepared.
   a.game.original_events.offers[2]=NoWorldRelic;std::strcpy(a.game.original_events.option_keys[2],"INITIAL.options.OPTION_POOL_3_LOCKED");a.cursor=2;a.render();CHECK(!choice(2).enabled);snapshot();press_choice(2);unchanged();capture("ancient-orobas-locked");
  }
  if(entry.id==OriginalEventId::Neow){
   // Exercise the post-Finish rendering contract independently of whichever
   // random ancient pickup might open its own card selection/reward group.
   auto&e=a.game.original_events;e.finished=1;e.option_count=1;e.options[0]=0;std::strcpy(e.page_key,"DONE");std::strcpy(e.option_keys[0],"LEAVE");a.cursor=0;a.scroll=0;
   CHECK(validate(a.game));finished_offer_hidden();capture("ancient-neow-complete");
  }
 }
}
static void ordinary_events(bool synthetic){
 const OriginalEventId ids[]={OriginalEventId::Wellspring,OriginalEventId::ColorfulPhilosophers,OriginalEventId::RelicTrader};
 const char*names[]={"event-wellspring","event-colorful-philosophers","event-relic-trader"};
 for(unsigned model=0;model<sizeof ids/sizeof ids[0];++model){
  begin(ids[model],0x6ab19,0);snapshot();a.render();const unsigned count=original_event_option_count(a.game);CHECK(count>0);
  for(unsigned selected=0;selected<count;++selected){
   a.cursor=int(selected);a.render();CHECK(choice_count()==(count-selected/3*3<3?count-selected/3*3:3));
   HitRegion previous{};for(unsigned i=selected/3*3;i<count&&i<selected/3*3+3;++i){const auto h=choice(int(i));
    CHECK(h.x>=128&&h.w>=180&&h.h>=35);if(i%3)CHECK(!overlap(h,previous));previous=h;}
   unchanged();
  }
  a.cursor=0;a.pointer.visible=false;a.render();
  if(synthetic){
   // The authored illustration remains unobscured: no repeated option panel
   // over its lower-left corner and no unrelated right-edge dark gradient.
   const auto p=a.content.image(a.content.world_image("events",original_event_key(ids[model])));CHECK(p.w==2&&p.h==2&&!p.alpha&&!p.palette);
   CHECK(a.pixels[200*ScreenW+20]==uint16_t(p.pixels[4]|unsigned(p.pixels[5])<<8));
   CHECK(a.pixels[50*ScreenW+319]==uint16_t(p.pixels[2]|unsigned(p.pixels[3])<<8));
  }
  capture(names[model]);reader(false);unchanged();
 }
}
int main(int argc,char**argv){
 std::vector<uint8_t>data;
 if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=synthetic_profile();
 // Only an explicit caller-selected private directory receives captures.
 if(argc>2)capture_directory=argv[2];CHECK(a.init(data.data(),data.size(),{}));CHECK(a.content.current_profile());
 if(argc==1)ancient_original_modulation();merchant();ancients();ordinary_events(argc==1);std::printf("world event UI: %u checks passed; %s profile; merchant, eight ancients and centered ordinary events\n",checks,argc>1?"actual":"synthetic");
}
