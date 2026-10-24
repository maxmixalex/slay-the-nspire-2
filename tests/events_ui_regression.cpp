#include "app.hpp"
#include "content_fixture.hpp"
#include "rich_text.hpp"
#include "fitted_text.hpp"
#include "feedback_assets.hpp"
#include "combat_layout.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static App a;static SaveData stored;static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static bool save_cb(const State&s){return save(s,stored);}static bool load_cb(State&s){return load(s,stored.bytes,stored.size);}
static SaveData snap(){SaveData b{};CHECK(save(a.game,b));return b;}static void same(const SaveData&b){auto n=snap();CHECK(n.size==b.size&&!memcmp(n.bytes,b.bytes,b.size));}
static void setup(EventId id){a.title=a.library=a.paused=a.help=a.character_select=a.map_preview.open=false;new_run(a.game,73,false);a.reset_presentation();auto&s=a.game;s.phase=Phase::Event;s.event_id=id;s.current_node=3;s.next_row=1;s.floor=2;s.map[3].event=(uint8_t)id;s.map[3].room=Room::Event;a.callbacks={save_cb,load_cb};a.render();}
static HitRegion hit(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];CHECK(false);return {};}
static void click(HitRegion h){a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);a.render();}
static void action(UiAction cmd){click(hit(HitKind::UiAction,(int)cmd));}
static int reader_lines(unsigned e){
 char localized[1024];const char*text=a.ui_text(a.content.string(EventTextBase+e*EventTextStride+1),localized,sizeof localized);
 auto metrics=[](const RichChar&ch,int size){int advance=a.content.glyph_metrics(ch.codepoint,size,rich_font_style(ch)).advance;return fitted_text::Metric{advance,advance};};
 // The reader reserves complete English words. Counting glyph wrap alone
 // undercounts Baths (12 instead of 13 lines in the real English pack).
 if(a.content.english()){int lines=fitted_text::line_count(text,277,12,metrics);return lines?lines:1;}
 RichReader reader(text);RichChar ch{};int lines=1,width=0;
 while(reader.next(ch)){int advance=metrics(ch,12).advance;
  if(ch.codepoint=='\n'||(width>0&&width+advance>277)){++lines;width=0;if(ch.codepoint=='\n')continue;}
  width+=advance;
 }return lines;
}
static void reading(bool art){
 for(unsigned e=0;e<EventKinds;e++){
  setup((EventId)e);auto before=snap();auto first=hit(HitKind::Choice,0),second=hit(HitKind::Choice,1);
  CHECK(first.x>=130&&second.x>=130&&first.y+first.h<=second.y&&second.y+second.h<=240);
  a.pointer_scroll(1);a.render();same(before);action(UiAction::ReadEvent);CHECK(a.event_reader);same(before);
  for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Choice&&a.hits[i].kind!=HitKind::OwnedRelic&&a.hits[i].kind!=HitKind::Hand);
  for(Key key:{Key::EndTurn,Key::Collection,Key::Map,Key::Save,Key::Load,Key::Potions,Key::Relics,Key::ReferenceMap}){a.input(key);CHECK(a.event_reader&&!a.library&&!a.map_preview.open);same(before);}
  a.input(Key::Down);a.render();CHECK(a.event_reader_scroll>=0&&a.event_reader_scroll<=1);
  CHECK(a.event_reader_scroll==(reader_lines(e)>10?1:0));same(before);
  a.pointer_scroll(1000);a.render();CHECK(a.event_reader_scroll==(reader_lines(e)>10?reader_lines(e)-10:0));same(before);a.input(Key::Back);CHECK(!a.event_reader);same(before);
  // A captured option released after opening/closing the reader cannot select it.
  first=hit(HitKind::Choice,0);a.pointer_move(first.x+12,first.y+12);a.pointer_button(true);a.input(Key::Detail);a.input(Key::Back);a.pointer_button(false);same(before);
 }
 RichReader reader("[rainbow]x[red]y[/red]z[/rainbow]w");RichChar ch{};CHECK(reader.next(ch)&&ch.codepoint=='x'&&ch.style.rainbow);CHECK(reader.next(ch)&&ch.codepoint=='y'&&ch.style.rainbow);CHECK(reader.next(ch)&&ch.codepoint=='z'&&ch.style.rainbow);CHECK(reader.next(ch)&&ch.codepoint=='w'&&!ch.style.rainbow);CHECK(!reader.next(ch));
}
static void choices(){
 setup(EventId::AromaOfChaos);click(hit(HitKind::Choice,1));CHECK(a.game.phase==Phase::Upgrade);auto pending=snap();a.cursor=4;a.input(Key::Confirm);CHECK(a.deck_candidate==4);same(pending);
 a.input(Key::Back);CHECK(a.deck_candidate<0&&a.game.phase==Phase::Upgrade);same(pending);a.input(Key::Back);CHECK(a.game.phase==Phase::Event&&a.game.event_page==0&&!a.game.deck[4].upgraded);
 click(hit(HitKind::Choice,1));a.input(Key::Save);pending=snap();a.cursor=3;a.input(Key::Confirm);action(UiAction::ConfirmDeckChoice);CHECK(a.game.phase==Phase::Event&&a.game.event_page==3&&a.game.deck[3].upgraded&&!a.game.map[3].visited);
 a.input(Key::Load);CHECK(a.game.phase==Phase::Upgrade&&a.deck_candidate<0);same(pending);a.input(Key::Back);CHECK(a.game.phase==Phase::Event);
 setup(EventId::DoorsOfLightAndDark);a.game.gold=0;int fee=remove_price(a.game);auto count=a.game.deck_count;
 click(hit(HitKind::Choice,1));CHECK(a.game.phase==Phase::Remove);a.cursor=1;a.input(Key::Confirm);action(UiAction::ConfirmDeckChoice);CHECK(a.game.phase==Phase::Event&&a.game.event_page==3&&a.game.deck_count==count-1&&a.game.gold==0&&remove_price(a.game)==fee);
 auto completed=snap();a.input(Key::Detail);a.input(Key::EndTurn);same(completed);a.input(Key::Back);click(hit(HitKind::Choice,0));CHECK(a.game.phase==Phase::Map);
}
static void shop(){
 setup(EventId::Baths);auto&s=a.game;s.phase=Phase::Map;s.current_node=1;s.next_row=1;s.hp=40;CHECK(gain_relic(s,MealTicket));CHECK(choose_node(s,1));a.reset_presentation();a.render();CHECK(s.hp==55&&!a.shop_open);auto before=snap();
 if(a.content.version()>=FeedbackContentVersion){
  const auto body=hit(HitKind::UiAction,int(UiAction::OpenShop));
  CHECK(body.x==MerchantLayout::BodyX-2&&body.y==MerchantLayout::BodyY-2&&body.w==MerchantLayout::BodyW+4&&body.h==MerchantLayout::BodyH+4);
  a.pointer_move(166,190);a.pointer_button(true);a.pointer_button(false);CHECK(!a.shop_open);same(before);
 }
 action(UiAction::OpenShop);CHECK(a.shop_open);same(before);a.input(Key::Back);CHECK(!a.shop_open&&!a.paused);same(before);a.input(Key::Confirm);CHECK(a.shop_open);same(before);
 // The original rug moves from y=-1000 to y=80 over 700 ms. Inspection
 // assertions below deliberately use its settled geometry after this real
 // clock, rather than searching for offscreen slots during its first frame.
 a.advance(250);a.advance(250);a.advance(200);a.render();same(before);
 for(int item=0;item<ShopRemove;item++){
  auto target=hit(HitKind::Choice,item);const auto*resolved=a.hit_at(target.x+target.w/2,target.y+target.h/2);CHECK(resolved&&resolved->kind==HitKind::Choice&&resolved->value==item);a.cursor=item;a.input(Key::Detail);CHECK(item<7?a.detail_card>=0:a.detail_item>=0);same(before);a.input(Key::Back);CHECK(a.shop_open);same(before);
 }
 s.gold=9999;a.cursor=0;a.input(Key::Confirm);CHECK(s.shop_sold&1);before=snap();
 // Purchase moves controller focus to the nearest remaining slot. Selecting
 // the sold slot explicitly still cannot buy it a second time.
 CHECK(a.cursor!=0);a.cursor=0;a.input(Key::Confirm);same(before);
 a.input(Key::Save);a.input(Key::Back);a.input(Key::Load);CHECK(!a.shop_open&&s.phase==Phase::Shop&&s.hp==55);same(before);action(UiAction::OpenShop);a.input(Key::Back);action(UiAction::LeaveShop);CHECK(s.phase==Phase::Map);
}
int main(int argc,char**argv){std::vector<uint8_t>data;if(argc>1){FILE*f=fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));size_t n=fread(data.data(),1,data.size(),f);fclose(f);CHECK(a.init(data.data(),n,{save_cb,load_cb}));}
reading(!data.empty());choices();shop();if(!data.empty()){for(int i=U_EventFull0;i<U_Count;i++)CHECK(a.content.image(a.content.ui_image(i)).pixels);CHECK(strcmp(a.content.string(255),a.content.string(AromaSilentText)));CHECK(!strstr(a.content.string(AromaSilentText),"{AromaPrinciple}"));CHECK(a.content.glyph(0x5546,8).pixels);}
printf("event/shop UI: %u checks passed\n",checks);}
