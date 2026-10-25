#include "app.hpp"
#include "rich_text.hpp"
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
 constexpr unsigned images=U_Count>CardKinds?U_Count:CardKinds,glyphs=5*95;
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
 const unsigned sizes[]={6,8,10,12,16};unsigned glyph=0;
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
  if(key=="ancients.DARV.talk.IRONCLAD.2-1.char")strings[row.index]="[i][font_size=22]Character reply.[/font_size][/i]";
 }
 for(unsigned i=0;i<ContentStrings;i++){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(strings[i].size()));b.insert(b.end(),strings[i].begin(),strings[i].end());b.push_back(0);}
 for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
 for(unsigned i=0;i<CardKinds;i++){put(b,carddir+i*16,0);for(unsigned j=1;j<4;j++)put(b,carddir+i*16+j*4,i*3+j-1);}
 for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
 put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static void advance_ms(unsigned milliseconds){while(milliseconds){const unsigned step=milliseconds>250?250:milliseconds;a.advance(step);milliseconds-=step;}a.render();}
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
static void press_action(UiAction action){
 a.render();const HitRegion*found=nullptr;
 for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(action)){CHECK(!found);found=&a.hits[i];}
 CHECK(found&&found->enabled);const auto hit=*found;a.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;
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
 a.game.gold=1000;a.title=a.paused=a.help=a.library=a.character_select=false;a.run_settings=false;a.map_preview.open=false;a.reset_presentation();a.pointer.visible=false;a.fast_mode=false;CHECK(validate(a.game));
 // These are settled layout/pixel checks. The separate room-motion suite
 // checks the exact original entry/press boundaries and blocked early input.
 a.render();advance_ms(1800);
}
static void reader(bool require_scroll){
 snapshot();uint32_t calls=a.game.original_events.rng_calls,revision=a.game.revision;
 a.input(Key::Detail);CHECK(a.event_reader);a.render();a.pointer_scroll(9);a.render();if(require_scroll)CHECK(a.event_reader_scroll>0);
 a.input(Key::Up);a.render();a.ui_action(UiAction::CloseEventReader);CHECK(!a.event_reader);a.render();
 CHECK(a.game.original_events.rng_calls==calls&&a.game.revision==revision);unchanged();
}
static void finished_offer_hidden(){
 a.pointer.visible=false;a.render();advance_ms(1800);CHECK(choice_count()==1);auto hit=choice(0);CHECK(hit.enabled);
 uint32_t old=image_hash(hit.x,hit.y,hit.w,hit.h);auto saved=a.game.original_events.offers[0];snapshot();
 // Finish keeps the old offer array. Changing that retained identity must
 // not change the sole Proceed button or turn it back into a relic offer.
 a.game.original_events.offers[0]=saved==WorldRelicGoldenPearl?WorldRelicFakeAnchor:WorldRelicGoldenPearl;
 a.render();CHECK(image_hash(hit.x,hit.y,hit.w,hit.h)==old);a.game.original_events.offers[0]=saved;unchanged();
}
static void merchant(){
 begin(OriginalEventId::FakeMerchant,23,1);a.render();CHECK(!a.shop_open&&!choice_count());
 press_action(UiAction::OpenShop);CHECK(a.shop_open);advance_ms(1000);CHECK(choice_count()==6);HitRegion slots[6];
 for(int i=0;i<6;i++){slots[i]=choice(i);CHECK(slots[i].enabled&&slots[i].w==36&&slots[i].h==42);for(int j=0;j<i;j++)CHECK(!overlap(slots[i],slots[j]));}
 CHECK(slots[0].x<slots[1].x&&slots[2].x<slots[3].x&&slots[3].x<slots[4].x);
 CHECK(slots[0].y<slots[2].y&&slots[1].y<slots[4].y&&slots[2].y<slots[5].y&&slots[3].y<slots[5].y&&slots[4].y<slots[5].y);
 snapshot();for(int i=0;i<6;i++){const auto&h=slots[i];a.pointer_move(h.x+h.w/2,h.y+h.h/2);CHECK(a.cursor==i);a.render();const auto*p=a.hit_at(h.x+h.w/2,h.y+h.h/2);CHECK(p&&p->kind==HitKind::Choice&&p->value==i);unchanged();}
 a.cursor=0;a.pointer.visible=false;a.render();auto h=choice(0);int price=a.game.original_events.choice_prices[0];
 uint32_t old_price=image_hash(h.x,h.y+h.h-14,h.w,14);snapshot();
 a.game.original_events.choice_prices[0]=int16_t(price+1);a.render();CHECK(image_hash(h.x,h.y+h.h-14,h.w,14)!=old_price);
 a.game.original_events.choice_prices[0]=int16_t(price);a.render();unchanged();capture("fake-merchant-stock");
 // Stock remains focusable when unaffordable, matching the original failed
 // purchase speech/wiggle; the core still rejects the actual purchase.
 a.game.gold=0;snapshot();a.render();for(int i=0;i<6;i++)CHECK(choice(i).enabled);press_choice(0);unchanged();CHECK(a.room_presentation.merchant.dialogue==1);
 a.game.gold=1000;auto bought=RelicId(a.game.original_events.offers[0]);unsigned count=a.game.world_relics.count;int gold=a.game.gold;press_choice(0);
 CHECK(a.game.phase==Phase::Event&&!a.game.original_events.finished);CHECK(a.game.original_events.offers[0]==NoWorldRelic&&has_world_relic(a.game,bought));
 CHECK(a.game.gold==gold-price&&a.game.world_relics.count==count+1);CHECK(validate(a.game));advance_ms(1000);CHECK(choice_count()==6&&!choice(0).enabled);capture("fake-merchant-sold");
 snapshot();press_choice(0);unchanged();press_action(UiAction::CloseShop);CHECK(!a.shop_open);
 // Observe the new closed-rug Proceed before spending its 500ms entry.
 // The existing 800ms room-settle interval also covers that authored move.
 a.render();advance_ms(800);unchanged();
 count=a.game.world_relics.count;gold=a.game.gold;
 static State leave_oracle;static SaveData leave_expected;
 CHECK(load(leave_oracle,before.bytes,before.size));CHECK(event_choice(leave_oracle,6)&&original_event_leave(leave_oracle));CHECK(save(leave_oracle,leave_expected));
 press_action(UiAction::LeaveShop);
 CHECK(save(a.game,after)&&after.size==leave_expected.size&&!std::memcmp(after.bytes,leave_expected.bytes,after.size));
 // Finished native rooms remain underneath the map until actual travel.
 CHECK(a.game.original_events.active&&a.game.original_events.finished&&a.game.phase==Phase::Event&&a.game.world.room_open==2);
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
  begin(entry.id,0x71ef23,entry.act);a.cursor=0;
  // Park the pointer outside every option. Some legitimate first-offer
  // mechanic tooltips (Tanx in this seed) overlap the third row when keyboard
  // focus is active; this assertion measures its button's own RGBA surface.
  a.pointer_move(318,23);a.pointer_move(319,23);snapshot();a.render();unchanged();
  Picture background=a.content.image(a.content.world_image("events",original_event_key(entry.id)));
  CHECK(background.pixels&&!background.alpha&&!background.palette&&background.w==2&&background.h==2);
  const unsigned at=3*2;const uint16_t back=uint16_t(background.pixels[at]|unsigned(background.pixels[at+1])<<8);
  const unsigned alpha=unsigned(entry.a*255.+.5);
  const unsigned r=(31*unsigned(entry.r*255.+.5)+127)/255;
  const unsigned g=(63*unsigned(entry.g*255.+.5)+127)/255;
  const unsigned b=(31*unsigned(entry.b*255.+.5)+127)/255;
  const uint16_t expected=uint16_t((((back>>11)*(255-alpha)+r*alpha+127)/255<<11)|
      ((((back>>5)&63)*(255-alpha)+g*alpha+127)/255<<5)|((back&31)*(255-alpha)+b*alpha+127)/255);
  CHECK(a.pixels[229*ScreenW+251]==expected);
 }
}
static void ancients(){
 struct Entry{OriginalEventId id;unsigned act;const char*file;};
 const Entry entries[]={{OriginalEventId::Neow,0,"ancient-neow"},{OriginalEventId::Darv,1,"ancient-darv"},{OriginalEventId::Orobas,1,"ancient-orobas"},{OriginalEventId::Pael,1,"ancient-pael"},{OriginalEventId::Tezcatara,1,"ancient-tezcatara"},{OriginalEventId::Nonupeipe,2,"ancient-nonupeipe"},{OriginalEventId::Tanx,2,"ancient-tanx"},{OriginalEventId::Vakuu,2,"ancient-vakuu"}};
 for(const auto&entry:entries){
  begin(entry.id,0x71ef23,entry.act);a.render();CHECK(choice_count()==3);HitRegion previous{};snapshot();
  for(int i=0;i<3;i++){auto hit=choice(i);CHECK(hit.x==65&&hit.w==190&&hit.h==19);if(i)CHECK(hit.y-previous.y==18||hit.y-previous.y==19);previous=hit;
   a.pointer_move(hit.x+hit.w/2,hit.y+hit.h/2);CHECK(a.cursor==i);a.input(Key::Down);a.render();unchanged();}
  a.cursor=0;a.scroll=0;capture(entry.file);reader(false);
  if(entry.id==OriginalEventId::Orobas){
   // Explicit UI fixture for the original locked third offer, generated by
   // Orobas when neither starter upgrade can be prepared.
   a.game.original_events.offers[2]=NoWorldRelic;std::strcpy(a.game.original_events.option_keys[2],"INITIAL.options.OPTION_POOL_3_LOCKED");a.cursor=2;a.render();
   // RefreshEventState clears the old dialogue. Inspect the source's new
   // bottom-aligned option stack after its content movement settles.
   advance_ms(1000);CHECK(!choice(2).enabled);snapshot();press_choice(2);unchanged();capture("ancient-orobas-locked");
  }
  if(entry.id==OriginalEventId::Neow){
   // Exercise the post-Finish rendering contract independently of whichever
   // random ancient pickup might open its own card selection/reward group.
   auto&e=a.game.original_events;e.finished=1;e.option_count=1;e.options[0]=0;std::strcpy(e.page_key,"DONE");std::strcpy(e.option_keys[0],"LEAVE");a.cursor=0;a.scroll=0;
   CHECK(validate(a.game));finished_offer_hidden();capture("ancient-neow-complete");
  }
 }
}
static void recorded_ancient_visits(unsigned count,Character character,bool complete){
 clear_ui_progress(a.progress);a.progress_loaded=true;
 a.progress.count=uint8_t(count);a.progress.next_serial=count+2;
 a.progress.current.serial=count+1;a.progress.current.seed=a.game.seed;
 a.progress.current.run_seed=a.game.world.run_seed;a.progress.current.character=unsigned(a.game.character);
 a.progress.current.partial=!complete;
 for(unsigned i=0;i<count;++i){auto&r=a.progress.history[i];r.serial=i+1;r.character=unsigned(character);r.floor_count=1;
  r.floors[0].event=unsigned(a.game.original_events.id);r.floors[0].act=1;r.floors[0].point=0;}
 a.ancient_dialogue={};a.render();
}
static void ancient_conversation(bool synthetic){
 // These records are an explicit fixture, not invented history in the game.
 // The locked Darv model's visit 4 is an Ancient / Character / Ancient
 // sequence. Dialogue clicks must not choose a relic or consume saved RNG.
 begin(OriginalEventId::Darv,0x8ac412,1);recorded_ancient_visits(4,Character::Ironclad,true);
 auto&d=a.ancient_dialogue;CHECK(d.history_complete&&d.count==3&&d.current==0&&!d.last());
 CHECK(!std::strcmp(d.keys[0],"DARV.talk.IRONCLAD.2-0.ancient"));
 CHECK(!std::strcmp(d.keys[1],"DARV.talk.IRONCLAD.2-1.char")&&(d.character_lines&2));
 CHECK(!choice_count());snapshot();advance_ms(1600);capture("ancient-dialogue-visit4-first");unchanged();
 a.input(Key::Confirm);a.render();CHECK(d.current==1&&!choice_count());advance_ms(1000);
 char response[1024]{};a.ancient_dialogue_text(1,response,sizeof response);RichReader response_reader(response);RichChar response_char{};unsigned response_count=0;
 while(response_reader.next(response_char)){CHECK(response_char.codepoint!='['&&response_char.codepoint!=']'&&response_char.style.italic);++response_count;}CHECK(response_count);
 capture("ancient-dialogue-character-response");unchanged();reader(false);
 a.pointer_move(305,170);const auto*next=a.hit_at(305,170);CHECK(next&&next->kind==HitKind::AncientDialogueNext);
 a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;a.render();CHECK(d.current==2&&d.last());
 a.input(Key::Confirm);unchanged();advance_ms(1000);CHECK(choice_count()==3);
 a.cursor=0;a.input(Key::Up);CHECK(a.cursor==2);a.input(Key::Down);CHECK(!a.cursor);
 a.input(Key::Right);CHECK(!a.cursor);unchanged();
 capture("ancient-dialogue-history-and-options");unchanged();
 // Hover any older visible line restores all history, then leaving restores
 // the original quarter-alpha. Wheel / arrow review moves only the content.
 a.pointer_scroll(-100);a.render();CHECK(d.scroll==d.max_scroll);unchanged();
 const int y=d.y(a.map_clock_ms)+d.scroll;
 a.pointer_move(90,y+6);a.render();advance_ms(100);
 CHECK(d.history_hovered&&d.history_alpha(a.map_clock_ms)==255);unchanged();
 a.pointer_move(310,80);a.render();advance_ms(100);CHECK(!d.history_hovered&&d.history_alpha(a.map_clock_ms)==64);unchanged();
 a.pointer_scroll(100);a.render();CHECK(!d.scroll);unchanged();
 // Losing the early history cannot assert a fifth visit. It selects only a
 // source repeatable line; it never silently replays this numbered sequence.
 recorded_ancient_visits(4,Character::Ironclad,false);CHECK(!d.history_complete);
 CHECK(d.count&&std::strstr(d.keys[0],"r.ancient")&&!std::strstr(d.keys[0],".2-0.ancient"));
 clear_ui_progress(a.progress);a.progress_loaded=false;
 begin(OriginalEventId::Neow,0x8ac413,0);CHECK(d.count==1&&std::strstr(d.keys[0],"firstVisitEver"));
 if(synthetic){CHECK(d.max_scroll>0);snapshot();a.pointer_scroll(-1000);a.render();CHECK(d.scroll==d.max_scroll);
  a.input(Key::Down);a.render();CHECK(d.scroll<d.max_scroll);unchanged();a.pointer_scroll(1000);a.render();CHECK(!d.scroll);}
 // A genuine page refresh clears the conversation. The resulting page has
 // only its Proceed option, with no synthetic completion speech bubble.
 auto&e=a.game.original_events;e.finished=1;e.option_count=1;e.options[0]=0;
 std::strcpy(e.page_key,"DONE");std::strcpy(e.option_keys[0],"LEAVE");a.render();advance_ms(1000);
 CHECK(!d.count&&choice_count()==1);capture("ancient-dialogue-cleared-on-finish");
}
static void ordinary_events(bool synthetic){
 const OriginalEventId ids[]={OriginalEventId::Wellspring,OriginalEventId::ColorfulPhilosophers,OriginalEventId::RelicTrader};
 const char*names[]={"event-wellspring","event-colorful-philosophers","event-relic-trader"};
 for(unsigned model=0;model<sizeof ids/sizeof ids[0];++model){
  begin(ids[model],0x6ab19,0);snapshot();a.render();const unsigned count=original_event_option_count(a.game);CHECK(count>0);
  // The source VBox grows its body and retains every option that fits. Up/
  // Down still navigate the saved option list; they do not scroll prose.
  a.cursor=0;a.input(Key::Up);CHECK(a.cursor==int(count)-1);a.input(Key::Down);CHECK(a.cursor==0);
  for(unsigned i=0;i<count;++i)a.input(Key::Down);CHECK(a.cursor==0&&a.scroll==0);unchanged();
  for(unsigned selected=0;selected<count;++selected){
   a.cursor=int(selected);a.render();const unsigned first=selected/6*6;CHECK(choice_count()==(count-first<6?count-first:6));
   HitRegion previous{};for(unsigned i=first;i<count&&i<first+6;++i){const auto h=choice(int(i));
    CHECK(h.x==153&&h.w==152&&h.h==19);if(i>first)CHECK(!overlap(h,previous)&&h.y==previous.y+20);previous=h;}
   unchanged();
  }
  HitRegion body{};for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(UiAction::ReadEvent))body=a.hits[i];
  CHECK(body.x==153&&body.y==54&&body.w==152&&body.h>=53);
  CHECK(choice(int(a.cursor/6*6)).y==body.y+body.h+6);
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
 if(argc==1)ancient_original_modulation();merchant();ancients();ancient_conversation(argc==1);ordinary_events(argc==1);std::printf("world event UI: %u checks passed; %s profile; merchant, eight ancients, revealed dialogue/history and centered ordinary events\n",checks,argc>1?"actual":"synthetic");
}
