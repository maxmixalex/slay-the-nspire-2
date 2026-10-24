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
static void native_ancient_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;
 const auto&motion=a.room_presentation.event;const unsigned age=a.map_clock_ms-motion.start;
 const int entering=int(182*(room_curve::Q-room_curve::expo_out(age,1000))/room_curve::Q);
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 c.centered(native_event_title(a),14,27,292,16,Gold);
 char key[128]{};TextBuffer k{key,sizeof key,0,&c.a.content};k.text(original_event_key(e.id));k.text(".epithet");
 c.centered(a.content.world_string("ancients",key),22,46,276,10,White);
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);
 const int lines=c.paragraph(text,66,64,231,0,0,10);
 a.scroll=mn(mx(0,a.scroll),mx(0,lines-2));
 if(*text){Canvas line=c;line.opacity=uint8_t(unsigned(line.opacity)*room_curve::linear(age,100)/room_curve::Q);
  const auto color=native_ancient_dialogue_color(e.id);const int y=58+entering;
  if(a.content.version()>=RoomUi7ContentVersion){
   line.ui(I7_AncientSpeakerStart+int(native_ancient_index(e.id)),25,y+7,23,23);
   Canvas shadow=line;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
   shadow.nine_ui(I7_AncientDialogue,57,y+1,249,37,6,6,6,6,64);shadow.ui(I7_AncientDialogueTail,50,y+13,8,9,64);
   Canvas bubble=line;bubble.tint_red=color.red;bubble.tint_green=color.green;bubble.tint_blue=color.blue;
   bubble.nine_ui(I7_AncientDialogue,56,y,249,37,6,6,6,6);bubble.ui(I7_AncientDialogueTail,49,y+12,8,9);
  }else line.dim(56,y,249,37,145);
  line.paragraph(text,66,64+entering,231,2,a.scroll,10,rgb(255,246,226));
  if(lines>2)line.text("…",287,79+entering,10,Gold,12);
  const int hit_y=mx(22,y),hit_bottom=mn(240,y+37);if(hit_bottom>hit_y)c.action(25,hit_y,281,hit_bottom-hit_y,UiAction::ReadEvent);
 }
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));
 // The original ancient scene is a centered VBox of three 1000x100 rows.
 // Keep that hierarchy while enlarging type and targets for 320x240.
 for(int i=0;i<count&&i<3;i++){
  const int y=(e.finished?196:106+i*42)+entering;bool enabled=original_event_option_enabled(s,i);
  const bool selected=enabled&&(a.pointer.visible||age>=800)&&merchant_hovered(a,i,14,y,292,40);
  int image=a.content.world_image("events","ANCIENT_OPTION");if(image<0)image=a.content.ui_image(U_EventButton);
  const auto color=native_ancient_button_color(e.id);Canvas button=c;
  button.tint_red=uint8_t(div255(button.tint_red*color.red));
  button.tint_green=uint8_t(div255(button.tint_green*color.green));
  button.tint_blue=uint8_t(div255(button.tint_blue*color.blue));
  button.picture(image,14,y,292,40,color.alpha);
  if(selected&&enabled){
   image=a.content.world_image("events","ANCIENT_OPTION_OUTLINE");if(image<0)image=a.content.ui_image(U_EventOutline);
   Canvas outline=c;outline.tint_red=uint8_t(div255(outline.tint_red*120));
   outline.tint_blue=uint8_t(div255(outline.tint_blue*254));
   // Original BlueGlow and additive material; its 1024x124 rect, scale
   // and pivot normalize to this independent 301x52 outline.
   outline.picture(image,10,y-6,301,52,192,presentation::Add);
  }
  bool icon=!e.finished&&i<e.offer_count&&e.offers[i]<WorldRelicKinds;
  if(icon)c.picture(a.relic_face_image(e.offers[i]),24,y+8,24,24);
  char label[256]{},description[4096]{};native_event_option(a,i,false,label,sizeof label);native_event_option(a,i,true,description,sizeof description);
  int tx=icon?60:26,tw=icon?234:268;
  Canvas label_canvas=c;if(!enabled)label_canvas.opacity=uint8_t(div255(label_canvas.opacity*179));
  if(e.finished)label_canvas.centered(label,26,y+12,268,12,enabled?Gold:Red);
  else{
   label_canvas.text(label,tx,y+2,10,enabled?Gold:Red,tw);
   int rows=label_canvas.paragraph(description,tx,y+14,tw,2,0,10,White);
   if(rows>2){c.dim(283,y+26,14,12,180);c.text("…",285,y+26,10,enabled?Gold:Muted,12);}
  }
  const int hit_y=mx(22,y),hit_bottom=mn(240,y+40);if(hit_bottom>hit_y)c.hit(14,hit_y,292,hit_bottom-hit_y,HitKind::Choice,i,enabled);
 }
}
static void native_fake_price(Canvas&c,int amount,int x,int y,bool enabled){
 char text[24]{};TextBuffer b{text,sizeof text,0,&c.a.content};b.number(amount);int left=x-(c.width(text,10)+13)/2;
 c.picture(A_Coin,left,y+2,10,10,enabled?255:135);c.text(text,left+12,y,10,enabled?Gold:Red);
}
static void native_fake_merchant_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;auto&motion=a.room_presentation.merchant;
 motion.set_open(a.shop_open,a.map_clock_ms);
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 if(!a.shop_open){
  // The original custom event first exposes its merchant and Proceed. The
  // six-slot rug is a separate inventory overlay, not the event's page body.
  c.action(195,92,67,95,UiAction::OpenShop);
  const char*label=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
  native_proceed(c,211,210,105,27,*label?label:"前进",true,!a.pointer.visible&&a.cursor==1);
  c.action(206,207,112,31,UiAction::LeaveShop);
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
 if(new_event&&e.id==OriginalEventId::FakeMerchant){a.shop_open=false;a.room_presentation.merchant=RoomUi7Presentation::Merchant{};}
 if(e.id==OriginalEventId::CrystalSphere&&e.finished&&e.crystal_item_count){crystal_screen(c);return;}
 if(ui_ancient_event(e.id)){native_ancient_screen(c);return;}
 if(e.id==OriginalEventId::FakeMerchant&&!e.finished){native_fake_merchant_screen(c);return;}
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 // default_event_layout.tscn places a centered story above one options VBox
 // on the right of the illustration. No extra dark column or duplicate
 // selected-description box belongs on top of the event artwork.
 Canvas title=c;title.opacity=uint8_t(div255(title.opacity*motion.title_alpha(a.map_clock_ms)));
 title.fitted(native_event_title(a),131,30,182,19,16,12,Gold,true,true,false,true);
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);
 Canvas body=c;body.opacity=uint8_t(unsigned(body.opacity)*motion.body_progress(a.map_clock_ms)/room_curve::Q);
 body.text_visible_characters=room_rich_characters(text)*motion.body_reveal(a.map_clock_ms)/room_curve::Q;
 const auto story=body.fitted(text,137,54,173,65,10,10,White,true,false,false,true);
 if(story.truncated&&motion.body_progress(a.map_clock_ms)==room_curve::Q)c.text("…",298,106,10,Gold,12);
 c.action(132,51,183,68,UiAction::ReadEvent);
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));int first=a.cursor/3*3;
 for(int i=first;i<mn(count,first+3);i++){const int y=123+(i-first)*38,x=130+motion.option_x(unsigned(i),a.map_clock_ms);
  const bool enabled=original_event_option_enabled(s,i),ready=motion.option_ready(unsigned(i),a.map_clock_ms);
  const bool selected=enabled&&ready&&merchant_hovered(a,i,x,y,186,37);
  const bool pressed=selected&&a.pointer.held&&a.pointer.captured.kind==HitKind::Choice&&a.pointer.captured.value==i&&a.pointer.captured.enabled;
  const auto visual=motion.buttons[i].observe(pressed?2:selected?1:0,a.map_clock_ms);
  const int width=186*visual.scale/4096,height=37*visual.scale/4096,left=x+(186-width)/2,top=y+(37-height)/2;
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
  words.paragraph_fitted(combined,x+10,y+3,164,32,10,White,8);
  c.hit(x,y,186,37,HitKind::Choice,i,enabled&&ready);
 }
 if(count>3){char page[32]{};TextBuffer b{page,sizeof page,0,&c.a.content};b.number(first/3+1);b.ch('/');b.number((count+2)/3);c.text(page,285,23,8,White,30);}
}
