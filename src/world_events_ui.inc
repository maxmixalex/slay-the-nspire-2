static bool ui_ancient_event(OriginalEventId id){switch(id){case OriginalEventId::Neow:case OriginalEventId::Darv:case OriginalEventId::Nonupeipe:case OriginalEventId::Tanx:case OriginalEventId::Orobas:case OriginalEventId::Pael:case OriginalEventId::Tezcatara:case OriginalEventId::Vakuu:return true;default:return false;}}
static uint32_t native_event_revision(const OriginalEventState&e){
 // Animate when the displayed page/options change. Using the whole core
 // revision would restart a paragraph after an unrelated potion or HUD action.
 uint32_t value=2166136261u;const auto add=[&](unsigned byte){value=(value^byte)*16777619u;};
 for(const char*p=e.page_key;*p;++p)add(uint8_t(*p));
 for(unsigned i=0;i<e.option_count&&i<MaxOriginalEventOptions;++i){add(e.options[i]);for(const char*p=e.option_keys[i];*p;++p)add(uint8_t(*p));}
 for(int n:e.values){add(unsigned(n)&255);add(unsigned(n)>>8&255);}add(e.offer_count);
 return value;
}
struct AncientButtonColor {uint8_t red,green,blue,alpha;};
static AncientButtonColor native_ancient_button_color(OriginalEventId id){
 // v0.111.0 EventModel.ButtonColor overrides, rounded to the RGB565
 // canvas's 8-bit multipliers. The source NinePatch is white plus alpha.
 switch(id){
  case OriginalEventId::Neow:return {0,26,51,128};
  case OriginalEventId::Darv:return {15,0,20,128};
  case OriginalEventId::Nonupeipe:return {0,26,41,191};
  case OriginalEventId::Tanx:return {13,5,0,128};
  case OriginalEventId::Orobas:return {13,0,26,89};
  case OriginalEventId::Pael:return {8,20,0,191};
  case OriginalEventId::Tezcatara:return {20,10,0,191};
  case OriginalEventId::Vakuu:return {13,15,31,204};
  default:return {0,0,0,89};
 }
}
static const char* native_event_title(const App&a){const char*text=a.content.world_title(ui_ancient_event(a.game.original_events.id)?"ancients":"events",original_event_key(a.game.original_events.id));return *text?text:original_event_key(a.game.original_events.id);}
static unsigned native_ancient_index(OriginalEventId id){switch(id){case OriginalEventId::Darv:return 1;case OriginalEventId::Nonupeipe:return 2;case OriginalEventId::Tanx:return 3;case OriginalEventId::Orobas:return 4;case OriginalEventId::Pael:return 5;case OriginalEventId::Tezcatara:return 6;case OriginalEventId::Vakuu:return 7;default:return 0;}}
static AncientButtonColor native_ancient_dialogue_color(OriginalEventId id){
 static constexpr AncientButtonColor colors[]={{0x28,0x45,0x4f,255},{0x51,0x2e,0x66,255},{0x0a,0x49,0x4d,255},{0x73,0x17,0x17,255},{0x5c,0x5f,0x7a,255},{0x33,0x2c,0x29,255},{0x33,0x25,0x1e,255},{0x3c,0x19,0x31,255}};
 return colors[native_ancient_index(id)];
}
static void native_event_option(const App&a,int option,bool description,char*out,size_t size){
 const auto&e=a.game.original_events;if(!size)return;out[0]=0;
 // The finished page retains its old offers for saved continuations, but its
 // sole Choice 0 is now Proceed. It must not look like a second acquisition.
 if(e.finished){
  a.original_event_text(option,description,out,size);
  if(!*out&&!description){TextBuffer b{out,size,0,&a.content};const char*text=a.content.world_string(ui_ancient_event(e.id)?"ancients":"events","PROCEED.title");b.text(*text?text:"继续前进");}return;
 }
 if(ui_ancient_event(e.id)&&option>=0&&option<e.offer_count){
  // EventOption.FromRelic gives an ancient's own option text precedence,
  // then uses the relic's acquisition description before its normal tooltip.
  char key[256]{};
  if(original_event_text_key(a.game,option,description,key,sizeof key))a.localized_text("ancients",key,out,size,int(e.id));
  if(*out)return;
  if(e.offers[option]<WorldRelicKinds){
   RelicId relic=RelicId(e.offers[option]);
   if(description){key[0]=0;TextBuffer b{key,sizeof key,0,&a.content};b.text(world_relic_key(relic));b.text(".eventDescription");a.localized_text("relics",key,out,size,int(relic));if(!*out)a.relic_description(relic,out,size);}
   else{TextBuffer b{out,size,0,&a.content};b.text(a.relic_title(relic));}
  }else if(!description){TextBuffer b{out,size,0,&a.content};b.text("当前无法选择");}
  return;
 }
 if(e.id==OriginalEventId::FakeMerchant&&option>=0&&option<6){
  if(e.offers[option]>=WorldRelicKinds){TextBuffer b{out,size,0,&a.content};b.text("售罄");return;}
  if(description){a.relic_description(RelicId(e.offers[option]),out,size);return;}
  TextBuffer b{out,size,0,&a.content};b.text(a.relic_title(RelicId(e.offers[option])));b.ch('\n');b.number(e.choice_prices[option]);b.text(" 金币");return;
 }
 if(e.id==OriginalEventId::FakeMerchant&&option==6){if(!description){TextBuffer b{out,size,0,&a.content};b.text("离开");}return;}
 a.original_event_text(option,description,out,size);
 if(!*out&&!description){TextBuffer b{out,size,0,&a.content};b.text(e.finished?"继续前进":"继续");}
}
#include "ancient_dialogue_input.inc"
#include "ancient_dialogue_ui.inc"
static void native_ancient_screen(Canvas&c){
 auto&a=c.a;const auto&e=a.game.original_events;a.sync_ancient_dialogue();
 auto&d=a.ancient_dialogue;const unsigned count=unsigned(mn(3,original_event_option_count(a.game)));
 a.cursor=mn(mx(0,a.cursor),mx(0,int(count)-1));
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 const int dialogue_height=ancient_measure(c,count),top=d.y(a.map_clock_ms)+d.scroll;
 Canvas clipped=c;clipped.pixels+=ancient_layout::ClipY*clipped.pitch+ancient_layout::ClipX;
 clipped.clip_w=ancient_layout::ClipW;
 clipped.clip_h=(d.last()?ancient_layout::Bottom:ancient_layout::NextBottom)-ancient_layout::ClipY;
 ancient_dialogue_lines(c,clipped,top);
 if(d.last())for(unsigned i=0;i<count;++i)ancient_option(c,clipped,int(i),top+dialogue_height+(d.count?2:0)+ancient_layout::option_y(i));
 else ancient_next_button(c);
 native_ancient_banner(c);
}
static void native_fake_price(Canvas&c,int amount,int x,int y,bool enabled){
 char text[24]{};TextBuffer b{text,sizeof text,0,&c.a.content};b.number(amount);int left=x-(c.width(text,10)+13)/2;
 c.picture(A_Coin,left,y+2,10,10,enabled?255:135);c.text(text,left+12,y,10,enabled?Gold:Red);
}
static void native_fake_merchant_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;auto&motion=a.room_presentation.merchant;
 motion.set_open(a.shop_open,a.map_clock_ms);
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 fake_merchant_welcome(c);
 const char*label=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
 native_proceed(c,*label?label:"前进",!a.shop_open,!a.pointer.visible&&a.cursor==1);
 if(!a.shop_open){
  // The original custom event first exposes its merchant and Proceed. The
  // six-slot rug is a separate inventory overlay, not the event's page body.
  c.action(195,92,67,95,UiAction::OpenShop);
  native_proceed_action(c,UiAction::LeaveShop);
  if(!motion.visible(a.map_clock_ms))return;
 }
 const int dy=motion.offset(a.map_clock_ms);
 c.dim(0,22,320,218,motion.alpha(a.map_clock_ms));
 c.picture(a.content.world_image("events","FAKE_MERCHANT_RUG"),MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY+dy,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH);
 static const int positions[6][2]={{869,218},{1144,288},{737,516},{1010,583},{1345,498},{823,847}};
 int focused=-1,focus_x=0,focus_y=0;
 for(int i=0;i<6;i++){
  const int x=MerchantInventoryLayout::project(-122+positions[i][0]),y=MerchantInventoryLayout::project(80+positions[i][1])+dy;
  const bool stocked=i<e.offer_count&&e.offers[i]<WorldRelicKinds;
  if(a.shop_open&&stocked&&merchant_hovered(a,i,x-18,y-17,36,42)){focused=i;focus_x=x;focus_y=y;}
 }
 motion.observe_focus(focused,focus_x,focus_y,a.map_clock_ms);
 for(int i=0;i<6;i++){
  const int x=MerchantInventoryLayout::project(-122+positions[i][0]),y=MerchantInventoryLayout::project(80+positions[i][1])+dy;
  const bool stocked=i<e.offer_count&&e.offers[i]<WorldRelicKinds;
  if(stocked){const int size=MerchantInventoryLayout::size(128,motion.scale(unsigned(i),a.map_clock_ms));
   c.picture(a.relic_face_image(e.offers[i]),x-size/2+motion.wiggle(unsigned(i),a.map_clock_ms),y-size/2,size,size);
   native_fake_price(c,e.choice_prices[i],x,y+MerchantInventoryLayout::size(52,motion.scale(unsigned(i),a.map_clock_ms)),s.gold>=e.choice_prices[i]);
  }
  const int top=mx(22,y-17),bottom=mn(240,y+25);
  if(a.shop_open&&bottom>top)c.hit(x-18,top,36,bottom-top,HitKind::Choice,i,stocked);
 }
 if(a.shop_open){native_back(c,3,212,52,28,"返回",true,!a.pointer.visible&&a.cursor==6);c.action(0,209,61,31,UiAction::CloseShop);}
 merchant_hand(c,true);merchant_dialogue(c,true);
 // NFakeMerchantRelic shares the normal relic hover-tip set. Its event owner
 // is collected by event_hover_target; a second local panel would duplicate it.
}
static void native_event_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;
 auto&motion=a.room_presentation.event;
 const bool new_event=!motion.bound||motion.id!=unsigned(e.id);
 motion.observe(unsigned(e.id),e.page,e.finished,native_event_revision(e),a.fast_mode,a.map_clock_ms);motion.ancient=ui_ancient_event(e.id);
 if(new_event&&e.id==OriginalEventId::FakeMerchant){a.shop_open=false;a.room_presentation.merchant=RoomUi7Presentation::Merchant{};
  if(!e.finished)a.room_presentation.merchant.greet(a.map_clock_ms,s.seed);
 }
 if(e.id==OriginalEventId::CrystalSphere&&e.finished&&e.crystal_item_count){crystal_screen(c);return;}
 if(ui_ancient_event(e.id)){native_ancient_screen(c);return;}
 if(e.id==OriginalEventId::FakeMerchant&&!e.finished){native_fake_merchant_screen(c);return;}
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 // default_event_layout.tscn: VBox at (viewport/2-38,255-39), width800.
 // Its64px title,280px minimum body,24px spacer and100px options belong to
 // the full1680x1260 UI, not the room artwork's cropped218px camera.
 constexpr int x0=153,width0=152,title_y=41,title_h=12,body_y=54,body_min=53;
 constexpr int option_h=19,option_step=20,spacer=6,max_visible=6;
 Canvas title=c;title.opacity=uint8_t(div255(title.opacity*motion.title_alpha(a.map_clock_ms)));
 title.font_face=a.content.english()?4:0;title.sparse_scene_sizes=true;
 const int title_size=a.content.font_size(7,title.font_face,false,true);
 title.fitted(native_event_title(a),x0,title_y+mx(0,title_h-title_size-2),width0,title_h,7,5,rgb(239,200,81),true,true,false,true);
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);
 Canvas body=c;body.bold_face=5;body.opacity=uint8_t(unsigned(body.opacity)*motion.body_progress(a.map_clock_ms)/room_curve::Q);
 body.text_visible_characters=room_rich_characters(text)*motion.body_reveal(a.map_clock_ms)/room_curve::Q;
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));const int first=a.cursor/max_visible*max_visible,shown=mn(count-first,max_visible);
 const int body_size=a.content.font_size(5);
 const auto metric=[&](const RichChar&ch,int px){const auto g=a.content.glyph_metrics(ch.codepoint,px,body.font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.advance),int(g.w))};};
 const int rows=fitted_text::line_count(text,width0,body_size,metric);
 const int options_height=shown?(shown-1)*option_step+option_h:0;
 // Godot fit_content grows the body before placing the options. Only prose
 // that exceeds the calculator viewport uses its existing complete reader;
 // ordinary four/five-option pages no longer lose rows behind a3-item page.
 const int body_h=mn(mx(body_min,rows*(body_size+1)+2),240-4-body_y-spacer-options_height);
 body.fitted(text,x0,body_y,width0,body_h,5,5,rgb(255,246,226),true,false,false,true);
 c.action(x0,body_y,width0,body_h,UiAction::ReadEvent);
 const int options_y=body_y+body_h+spacer;
 for(int i=first;i<first+shown;i++){const int y=options_y+(i-first)*option_step,x=x0+motion.option_x(unsigned(i),a.map_clock_ms);
  const bool enabled=original_event_option_enabled(s,i),ready=motion.option_ready(unsigned(i),a.map_clock_ms);
  const bool selected=enabled&&ready&&merchant_hovered(a,i,x,y,width0,option_h);
  const bool pressed=selected&&a.pointer.held&&a.pointer.captured.kind==HitKind::Choice&&a.pointer.captured.value==i&&a.pointer.captured.enabled;
  const auto visual=motion.buttons[i].observe(pressed?2:selected?1:0,a.map_clock_ms);
  const int width=width0*visual.scale/4096,height=option_h*visual.scale/4096,left=x+(width0-width)/2,top=y+(option_h-height)/2;
  Canvas entry=c;entry.opacity=uint8_t(unsigned(entry.opacity)*motion.option_progress(unsigned(i),a.map_clock_ms)/room_curve::Q);
  // NEventOptionButton: a shadow plus a blue additive focus outline, and a
  // single rich label with a gold/red bold title followed by its description.
  Canvas shadow=entry;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.ui(U_EventOutline,left,top+1,width,height,128);
  Canvas button=entry;button.brightness=uint8_t(div255(button.brightness*(enabled?visual.shade:166)));
  button.ui(U_EventButton,left,top,width,height,230);
  if(visual.glow&&enabled){Canvas outline=entry;outline.tint_red=uint8_t(div255(outline.tint_red*120));outline.tint_blue=uint8_t(div255(outline.tint_blue*254));
   outline.picture(a.content.ui_image(U_EventOutline),left-1,top-1,width+2,height+2,unsigned(visual.glow)*192/255,presentation::Add);}
  char label[256]{},description[2048]{},combined[2432]{};native_event_option(a,i,false,label,sizeof label);native_event_option(a,i,true,description,sizeof description);
  TextBuffer body{combined,sizeof combined,0,&a.content};
  if(*label){body.text(enabled?"[gold][b]":"[red][b]");body.text(label);body.text(enabled?"[/b][/gold]":"[/b][/red]");if(*description)body.ch('\n');}body.text(description);
  Canvas words=entry;if(!enabled)words.opacity=uint8_t(div255(words.opacity*179));
  native_room_compact_text(words,combined,x+8,y+2,138,14);
  c.hit(x,y,width0,option_h,HitKind::Choice,i,enabled&&ready);
 }
 if(count>max_visible){char page[32]{};TextBuffer b{page,sizeof page,0,&c.a.content};b.number(first/max_visible+1);b.ch('/');b.number((count+max_visible-1)/max_visible);c.text(page,285,23,5,White,30);}
}
