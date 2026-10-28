// A real cold inventory starts its original rug tween immediately and yields
// between preparation steps. Native timing is not calculator FPS evidence.
#include "../src/app.cpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"merchant preparation:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t budget_clock_value=0,budget_clock_step=0;
static uint32_t budget_clock(){const auto result=budget_clock_value;budget_clock_value+=budget_clock_step;return result;}
static void budgets(){
 GlyphPreparationBudget offline;for(unsigned i=0;i<4;++i)CHECK(offline.take());CHECK(!offline.take());
 for(uint32_t start:{0u,0xfffffffdu}){
  budget_clock_value=start;budget_clock_step=1;GlyphPreparationBudget timed(budget_clock);
  for(unsigned i=0;i<64;++i)CHECK(timed.take());CHECK(!timed.take()&&!timed.remaining);CHECK(!timed.take());
 }
 budget_clock_value=123;budget_clock_step=65;GlyphPreparationBudget expired(budget_clock);
 CHECK(!expired.take()&&!expired.remaining);budget_clock_value=123;CHECK(!expired.take());
 budget_clock_value=7;budget_clock_step=0;GlyphPreparationBudget fixed(budget_clock);
 for(unsigned i=0;i<128;++i)CHECK(fixed.take());CHECK(!fixed.take());
 budget_clock_value=0xfffffffdu;budget_clock_step=65;
 GlyphPreparationBudget stock(budget_clock,GlyphPreparationBudget::InventoryTicks);
 for(unsigned i=0;i<4;++i)CHECK(stock.take());CHECK(!stock.take());
}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void shop(){
 new_run(a.game,73,false);a.game.map[0].room=Room::Shop;CHECK(choose_node(a.game,0));
 a.title=a.library=a.paused=a.help=a.character_select=a.settings_open=false;a.frontend_confirm=0;a.progress.tutorial_flags=0;
 a.reset_presentation();a.pointer.visible=false;a.map_clock_ms=1400;CHECK(save(a.game,before));
}
static bool action(UiAction value){for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(value))return true;return false;}
static void pending_controls(){
 shop();auto&p=a.merchant_preparation;a.shop_open=true;p.active=true;p.card=ShopCardCount;p.prices_ready=true;
 a.step_merchant_preparation();CHECK(p.active&&!p.shown&&!p.updates);unchanged();
 a.input(Key::Right);CHECK(p.active&&a.shop_open&&a.cursor!=0);unchanged();
 a.input(Key::Back);CHECK(p.active&&!a.shop_open);unchanged();
 p.active=p.shown=a.shop_open=true;p.card=ShopCardCount;a.room_presentation.merchant.set_open(true,a.map_clock_ms);
 const auto opened=a.map_clock_ms;a.paused=true;a.step_merchant_preparation();CHECK(p.active&&!p.updates);
 a.map_clock_ms+=33;
 a.paused=false;a.step_merchant_preparation();CHECK(!p.active&&p.updates==1&&a.room_presentation.merchant.open);
 CHECK(a.room_presentation.merchant.start==opened);unchanged();
 // The source opens its slots immediately. Background cache work must not
 // swallow an ordinary purchase or force the player to wait for an idle skin.
 shop();a.shop_open=true;p.active=p.shown=true;a.game.gold=1000;
 const int gold=a.game.gold,deck=a.game.deck_count;CHECK(!(a.game.shop_sold&1));
 a.input(Key::Confirm);CHECK(a.game.shop_sold&1);CHECK(a.game.gold<gold&&a.game.deck_count==deck+1);
}
static std::vector<uint8_t> read(const std::string&path){
 FILE*f=std::fopen(path.c_str(),"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));const long length=std::ftell(f);CHECK(length>40&&!std::fseek(f,0,SEEK_SET));
 std::vector<uint8_t>b(static_cast<size_t>(length));CHECK(std::fread(b.data(),1,b.size(),f)==b.size());CHECK(!std::fclose(f));return b;
}
static unsigned missing_stock_glyphs(){
 unsigned missing=0;
 const auto scan=[&](const char*text,int size,unsigned face){
  RichReader reader(text);RichChar ch{};
  while(reader.next(ch))if(ch.codepoint!='\n'&&!a.content.glyph_cached(ch.codepoint,size,rich_font_style(ch)|face))++missing;
 };
 Canvas c{a};const auto fonts=card_face_fonts(c,MerchantInventoryLayout::CardW);
 const unsigned label=source_label_face(a.content.original_label_fonts(),5);
 for(unsigned i=0;i<ShopCardCount;++i){
  if(a.game.shop_sold&(1u<<i))continue;
  const Card card=a.game.world.active?a.game.world.shop_cards[i]:Card{a.game.shop_cards[i],0};
  const auto*facts=a.game.world.active?&a.game.world.shop_facts[i]:nullptr;
  scan(a.content.card_string(card.kind),fonts.title,label);
  scan(card_type_label(a.card_face_type(card,-1,-1,facts),&a.content),fonts.type,fonts.compact?label:0);
  char body[1024]{};
  if(facts)a.world_card_description(card,-1,body,sizeof body,-1,-1,facts);
  else a.card_description(card,-1,body,sizeof body,-1,-1);
  scan(body,fonts.body,0);
 }
 return missing;
}
static void actual(const char*folder){
 const std::string root=folder;auto common=read(root+"/data/nSpire2.dat.tns"),locale=read(root+"/data/nSpire2.eng.dat.tns");SplitResourceSource split;
 CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(locale.data(),locale.size()),ResourceLanguage::English));CHECK(a.init(split.source(),{}));shop();
 // Resumed merchant stock is known during the original character wipe.
 // That existing interval performs bounded work and keeps its exact800ms
 // transition deadline, even if most of the background queue is unfinished.
 a.title=true;a.character_departure.phase=1;a.character_departure.continuing=true;
 a.character_departure.started=a.map_clock_ms;const auto departure_start=a.map_clock_ms;
 a.advance(33);CHECK(a.title&&a.character_departure.phase==1);
 CHECK(a.merchant_preparation.planned&&a.merchant_preparation.shown&&a.merchant_preparation.updates==1);
 const auto prepared=a.merchant_preparation.updates;a.render();a.advance(33);
 CHECK(a.merchant_preparation.updates==prepared+1&&a.merchant_preparation.active);unchanged();
 while(a.map_clock_ms-departure_start<800)a.advance(33);
 CHECK(!a.title&&a.character_departure.phase==2&&a.map_clock_ms-departure_start==825);
 CHECK(a.merchant_preparation.active&&a.character_departure.started==a.map_clock_ms);unchanged();
 CHECK(a.init(split.source(),{}));shop();a.render();
 auto&p=a.merchant_preparation;
 CHECK(!a.shop_open&&p.active&&p.shown&&p.planned&&p.count>0&&p.count<=MerchantPreparation::Capacity);
 CHECK(action(UiAction::OpenShop)&&!action(UiAction::LeaveShop)); // Proceed starts outside the LCD.
 CHECK(missing_stock_glyphs()>0);unchanged();
 // A pass whose clock budget is already spent must not start another
 // descriptor read or decoder step, including across timer wrap.
 const auto expired_reads=a.content.read_stats();
 budget_clock_value=0xfffffffdu;budget_clock_step=GlyphPreparationBudget::InventoryTicks;a.callbacks.foreground_clock_ticks=budget_clock;
 a.step_merchant_preparation();a.callbacks.foreground_clock_ticks=nullptr;
 CHECK(a.content.read_stats().callbacks==expired_reads.callbacks&&p.active);unchanged();
 unsigned steps=0,font_steps=0;const auto room_started=a.map_clock_ms;
 while(p.active&&steps<4096){
  const unsigned decoded=p.cold_images,card=p.card;
  const unsigned image_limit=p.prices_ready?p.count:p.stock_count;
  const bool fonts=p.cursor==p.stock_count&&p.card<ShopCardCount&&p.text_ready;
  const bool bounded=p.cursor<image_limit||fonts||p.card==ShopCardCount;
  const auto reads=a.content.read_stats();
  const uint32_t groups=a.content.retained_decode_serial(),now=a.map_clock_ms;
  a.advance(33);++steps;
  CHECK(a.content.healthy()&&p.cold_images-decoded<=4&&p.card-card<=1);
  if(bounded)CHECK(a.content.read_stats().callback_bytes-reads.callback_bytes<=4u*4096u);
  if(fonts){++font_steps;CHECK(a.content.retained_decode_serial()-groups<=1);}
  CHECK(!a.shop_open&&a.map_clock_ms==now+33);unchanged();a.render();
  CHECK(action(UiAction::OpenShop));
  if(a.map_clock_ms-room_started>=500)CHECK(action(UiAction::LeaveShop));
 }
 CHECK(steps>1&&steps<4096&&font_steps>0&&!p.active&&p.prices_ready);
 CHECK(p.cursor==p.count&&p.card==ShopCardCount&&missing_stock_glyphs()==0);
 const auto start=a.content.read_stats();const unsigned updates=p.updates;
 a.ui_action(UiAction::OpenShop);
 CHECK(a.shop_open&&!p.active&&p.planned&&p.updates==updates);
 CHECK(a.room_presentation.merchant.open&&a.room_presentation.merchant.start==a.map_clock_ms);
 CHECK(a.content.read_stats().callbacks==start.callbacks);a.step_merchant_preparation();CHECK(p.updates==updates);unchanged();
 a.render();CHECK(p.shown);
 bool close=action(UiAction::CloseShop);for(int i=0;i<a.hit_count;++i)
  close|=a.hits[i].kind==HitKind::Choice&&a.hits[i].value==ShopLeave&&a.hits[i].enabled;
 CHECK(close);
 const auto opened=a.map_clock_ms;
 for(unsigned i=0;i<40;++i){a.advance(33);a.render();}
 CHECK(a.room_presentation.merchant.start==opened&&a.room_presentation.merchant.open);
 unsigned cards=0;for(int i=0;i<a.hit_count;++i)cards+=a.hits[i].kind==HitKind::Choice&&a.hits[i].value>=0&&a.hits[i].value<ShopCardCount;
 CHECK(cards==ShopCardCount);
 CHECK(MerchantInventoryLayout::size(300,MerchantInventoryLayout::card_scale(800))==66);
 CHECK(MerchantInventoryLayout::size(422,MerchantInventoryLayout::card_scale(800))==93);
 bool focused_cache=false;for(const auto&entry:small_card_faces)
  focused_cache|=entry.valid&&entry.width==66&&entry.height==93;
 CHECK(focused_cache);
 const HitRegion*back=nullptr;for(int i=0;i<a.hit_count;++i)
  if(a.hits[i].kind==HitKind::Choice&&a.hits[i].value==ShopLeave)back=&a.hits[i];
 CHECK(back&&back->x==0&&back->y==209&&back->w==30&&back->h==31);
 for(int i=0;i<a.hit_count;++i){const auto&hit=a.hits[i];if(hit.kind!=HitKind::Choice||hit.value<0||hit.value>=ShopCardCount)continue;
  CHECK(hit.x>=back->x+back->w||hit.y+hit.h<=back->y);}
 unchanged();CHECK(a.content.healthy());
 const unsigned decoded=p.cold_images;a.input(Key::Back);CHECK(!p.active&&!a.shop_open&&p.planned);unchanged();
 // An immediate click still starts the source tween at once. Closing while
 // cold keeps the bounded background queue and leaves normal room input live.
 CHECK(a.init(split.source(),{}));shop();a.render();a.ui_action(UiAction::OpenShop);
 CHECK(p.active&&a.shop_open);const auto cold_open=a.map_clock_ms;
 a.input(Key::Right);unchanged();a.input(Key::Back);CHECK(p.active&&!a.shop_open);unchanged();
 for(unsigned i=0;p.active&&i<4096;++i){a.advance(33);a.render();CHECK(!a.shop_open);unchanged();}
 CHECK(!p.active&&p.prices_ready&&missing_stock_glyphs()==0);
 CHECK(!a.room_presentation.merchant.open&&a.room_presentation.merchant.start>=cold_open);
 std::printf("actual cold inventory: %u image decodes, %u font updates in %u yielded closed-room updates; all actual stock glyphs cached, seven stock slots visible\n",decoded,font_steps,steps);
}
int main(int argc,char**argv){CHECK(argc==1||argc==2);budgets();pending_controls();if(argc==2)actual(argv[1]);std::printf("merchant preparation: %u checks passed\n",checks);}
