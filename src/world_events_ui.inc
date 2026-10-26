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
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,17,320,223);
 const int dialogue_height=ancient_measure(c,count),top=d.y(a.map_clock_ms)+d.scroll;
 Canvas clipped=c;clipped.pixels+=ancient_layout::ClipY*clipped.pitch+ancient_layout::ClipX;
 clipped.clip_w=ancient_layout::ClipW;
 clipped.clip_h=(d.last()?ancient_layout::Bottom:ancient_layout::NextBottom)-ancient_layout::ClipY;
 ancient_dialogue_lines(c,clipped,top);
 if(d.last())for(unsigned i=0;i<count;++i)ancient_option(c,clipped,int(i),top+dialogue_height+(d.count?2:0)+ancient_layout::option_y(i));
 else ancient_next_button(c);
 native_ancient_banner(c);
}
static void native_fake_merchant_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;auto&motion=a.room_presentation.merchant;
 motion.set_open(a.shop_open,a.map_clock_ms);
 if(!fake_merchant_room(c))c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
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
 int focused=-1,focus_x=0,focus_y=0;
 for(int i=0;i<6;i++){
  const int x=MerchantInventoryLayout::fake_x(unsigned(i)),y=MerchantInventoryLayout::fake_y(unsigned(i))+dy;
  const bool stocked=i<e.offer_count&&e.offers[i]<WorldRelicKinds;
  const auto icon=MerchantInventoryLayout::relic_bounds(x,y,motion.scale(unsigned(i),a.map_clock_ms),true,!a.content.english());
  if(a.shop_open&&stocked&&merchant_hovered(a,i,icon.x,icon.y,icon.w,icon.h)){focused=i;focus_x=x;focus_y=y;}
 }
 motion.observe_focus(focused,focus_x,focus_y,a.map_clock_ms);
 for(int i=0;i<6;i++){
  const int x=MerchantInventoryLayout::fake_x(unsigned(i)),y=MerchantInventoryLayout::fake_y(unsigned(i))+dy;
  const bool stocked=i<e.offer_count&&e.offers[i]<WorldRelicKinds;
  const unsigned scale=motion.scale(unsigned(i),a.map_clock_ms);
  const auto icon=MerchantInventoryLayout::relic_bounds(x,y,scale,true,!a.content.english());
  if(stocked){c.picture(a.relic_face_image(e.offers[i]),icon.x+motion.wiggle(unsigned(i),a.map_clock_ms),icon.y,icon.w,icon.h);
   merchant_cost_label(c,e.choice_prices[i],x,y+MerchantInventoryLayout::size(52,scale),scale,s.gold>=e.choice_prices[i]);
  }
  const int top=mx(22,icon.y),bottom=mn(240,icon.y+icon.h);
  if(a.shop_open&&bottom>top)c.hit(icon.x,top,icon.w,bottom-top,HitKind::Choice,i,stocked);
 }
 if(a.shop_open){native_back(c,3,212,52,28,"返回",true,!a.pointer.visible&&a.cursor==6);c.action(0,209,61,31,UiAction::CloseShop);}
 merchant_hand(c,true);merchant_dialogue(c,true);
 // NFakeMerchantRelic shares the normal relic hover-tip set. Its event owner
 // is collected by event_hover_target; a second local panel would duplicate it.
}
static void native_architect_dialogue(Canvas&c){
 auto&a=c.a;const auto&e=a.game.original_events;auto&motion=a.room_presentation.event;
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);
 // The original uses TalkCmd at the speaker, with an empty event body.
 // On the240px LCD keep the full scrolling line below both actor silhouettes
 // and above Proceed, reusing the original dialogue frame and scrollbar.
 constexpr int x=8,y=168,w=304,h=33,tx=x+8,ty=y+5,tw=w-28,th=h-10;
 const bool player=(a.game.character==Character::Regent||a.game.character==Character::Necrobinder)?(e.values[1]%2==0):(e.values[1]%2!=0);
 const auto color=player?ancient_character_color(a.game.character):AncientButtonColor{51,51,51,255};
 // NSpeechBubbleVfx starts its alpha and visible_ratio together over400ms;
 // it has none of the default event body's extra750/450ms title delay.
 const unsigned reveal=room_curve::linear(a.map_clock_ms-motion.start,400);
 Canvas body=c;body.bold_face=5;body.opacity=uint8_t(unsigned(body.opacity)*reveal/room_curve::Q);
 Canvas panel=body;panel.tint_red=color.red;panel.tint_green=color.green;panel.tint_blue=color.blue;
 panel.nine_ui(I7_AncientDialogue,x,y,w,h,6,6,6,6);
 const uint32_t key=native_event_revision(e)^(unsigned(e.id)<<24);
 if(a.event_body_key!=key){a.event_body_key=key;a.event_body_scroll.reset();a.event_body_focus=false;}
 const int size=a.content.font_size(10);const auto metric=[&](const RichChar&ch,int px){return body.text_metric(ch,px);};
 auto&scroll=a.event_body_scroll;scroll.configure(fitted_text::line_count(text,tw,size,metric)*(size+1)+2,th);
 a.event_body_top=ty;a.event_body_height=th;
 body.text_visible_characters=room_rich_characters(text)*reveal/room_curve::Q;
 body.text_effect_identity=0x41445242u^key;
 native_room_scroll_text(body,text,tx,ty,tw,th,scroll.offset(),size);
 if(scroll.maximum){
  constexpr int sx=x+w-10,sy=ty+6,length=th-12;
  c.ui(I6_ScrollbarTrack,sx,sy,5,length);c.ui(I6_ScrollbarEdge,sx,ty,5,6);
  ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),sx+2,ty+th-3,5,6,0,-1024);
  c.ui(I6_ScrollbarGrabber,sx-2,scroll.thumb_y(sy,length)-5,9,10);
  c.hit(tx,ty,tw,th,HitKind::EventScroll,0);c.hit(sx-4,ty,13,th,HitKind::EventScroll,1);
 }
 char label[256]{};a.original_event_text(0,false,label,sizeof label);
 // A line's authored next label takes precedence over Continue/Respond.
 if(!*label&&!e.finished){const char*next=a.content.world_string("ancients",player?"THE_ARCHITECT.CONTINUE":"THE_ARCHITECT.RESPOND");TextBuffer b{label,sizeof label,0};b.text(next);}
 if(!*label)native_event_option(a,0,false,label,sizeof label);
 const bool ready=motion.option_ready(0,a.map_clock_ms);a.cursor=0;
 // NEventOptionButton still delays/enables at1s Normal / .5s Fast. Start
 // the retained Proceed image when this line is created, not another .5s
 // after its input gate has opened.
 Canvas button=c;button.opacity=uint8_t(unsigned(button.opacity)*motion.option_progress(0,a.map_clock_ms)/room_curve::Q);
 native_proceed(button,label,true,ready&&!a.pointer.visible);native_proceed_choice(c,0,ready);
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
 if(e.id==OriginalEventId::TheArchitect)architect_scene_actors(c);
 if(e.id==OriginalEventId::TheArchitect&&a.screen_motion.sequence_active(a.map_clock_ms)){
  // PlayCurrentLine awaits the initial reading/head-stop sequence, including
  // the Silent's attack when present, before revealing dialogue or options.
  motion.start=a.screen_motion.sequence_end(a.map_clock_ms);return;
 }
 if(e.id==OriginalEventId::TheArchitect){native_architect_dialogue(c);return;}
 // default_event_layout.tscn: VBox at (viewport/2-38,255-39), width800.
 // Its64px title,280px minimum body,24px spacer and100px options belong to
 // the full1680x1260 UI, not the room artwork's cropped218px camera.
 // The event keeps a104px hover lane. Preserve the bevel's source aspect
 // in the NinePatch edges instead of stretching its full image sideways.
 constexpr int x0=126,width0=186,title_y=31,title_h=20,body_y=54,body_min=49;
 constexpr int option_h=40,option_step=42,spacer=6,max_visible=3;
 Canvas title=c;title.opacity=uint8_t(div255(title.opacity*motion.title_alpha(a.map_clock_ms)));
 title.font_face=a.content.original_label_fonts()?4:0;title.sparse_scene_sizes=true;
 const int title_size=a.content.font_size(10,title.font_face,false,true);
 title.fitted(native_event_title(a),x0,title_y+mx(0,title_h-title_size-2),width0,title_h,10,8,rgb(239,200,81),true,true,false,true);
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);
 Canvas body=c;body.bold_face=5;body.opacity=uint8_t(unsigned(body.opacity)*motion.body_progress(a.map_clock_ms)/room_curve::Q);
 body.text_visible_characters=room_rich_characters(text)*motion.body_reveal(a.map_clock_ms)/room_curve::Q;
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));const int first=a.cursor/max_visible*max_visible,shown=mn(count-first,max_visible);
 const int body_size=a.content.font_size(10);
 const auto metric=[&](const RichChar&ch,int px){return body.text_metric(ch,px);};
 const int rows=fitted_text::line_count(text,width0,body_size,metric);
 const int options_height=shown?(shown-1)*option_step+option_h:0;
 // Keep the authored prose and options together. The independent body
 // scroll has real source scrollbar art, wheel/drag and keyboard focus.
 const int body_h=mn(mx(body_min,rows*(body_size+1)+2),240-4-body_y-spacer-options_height);
 const uint32_t body_key=native_event_revision(e)^(unsigned(e.id)<<24);
 if(a.event_body_key!=body_key){a.event_body_key=body_key;a.event_body_scroll.reset();a.event_body_focus=false;}
 auto&scroller=a.event_body_scroll;const int prose_width=width0-13;
 const int prose_rows=fitted_text::line_count(text,prose_width,body_size,metric);
 scroller.configure(prose_rows*(body_size+1)+2,body_h);a.event_body_top=body_y;a.event_body_height=body_h;
 body.text_effect_identity=0x45565442u^body_key;
 native_room_scroll_text(body,text,x0,body_y,prose_width,body_h,scroller.offset(),body_size);
 if(scroller.maximum){
  const int sx=x0+width0-8,sy=body_y+6,length=mx(1,body_h-12);
  c.ui(I6_ScrollbarTrack,sx,sy,5,length);c.ui(I6_ScrollbarEdge,sx,body_y,5,6);
  ui_picture_transform(c,a.content.ui_image(I6_ScrollbarEdge),sx+2,body_y+body_h-3,5,6,0,-1024);
  c.ui(I6_ScrollbarGrabber,sx-2,scroller.thumb_y(sy,length)-5,9,10);
  c.hit(x0,body_y,prose_width,body_h,HitKind::EventScroll,0);
  c.hit(sx-4,body_y,13,body_h,HitKind::EventScroll,1);
 }
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
  const bool source_edges=a.content.ui_image(I13_EventButton)>=0;
  const unsigned button_id=source_edges?I13_EventButton:U_EventButton,outline_id=source_edges?I13_EventOutline:U_EventOutline;
  Canvas shadow=entry;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=uint8_t(div255(shadow.opacity*128));
  shadow.nine_ui(outline_id,left,top+1,width,height,12,10,12,10);
  Canvas button=entry;button.brightness=uint8_t(div255(button.brightness*(enabled?visual.shade:166)));
  button.opacity=uint8_t(div255(button.opacity*230));button.nine_ui(button_id,left,top,width,height,12,10,12,10);
  if(visual.glow&&enabled){Canvas outline=entry;outline.tint_red=uint8_t(div255(outline.tint_red*120));outline.tint_blue=uint8_t(div255(outline.tint_blue*254));
   outline.opacity=uint8_t(div255(outline.opacity*unsigned(visual.glow)*192/255));
   outline.nine_ui(outline_id,left-1,top-1,width+2,height+2,12,10,12,10,255,presentation::Add);}
  char label[256]{},description[2048]{},combined[2432]{};native_event_option(a,i,false,label,sizeof label);native_event_option(a,i,true,description,sizeof description);
  TextBuffer body{combined,sizeof combined,0,&a.content};
  if(*label){body.text(enabled?"[gold][b]":"[red][b]");body.text(label);body.text(enabled?"[/b][/gold]":"[/b][/red]");if(*description)body.ch('\n');}body.text(description);
  Canvas words=entry;if(!enabled)words.opacity=uint8_t(div255(words.opacity*179));
  const int tx=left+15,ty=top+6,tw=width-30,th=height-12;
  const auto plan=native_room_compact_plan(words,combined,tw,th);
  const int overflow=mx(0,plan.total*plan.step+2-th);
  if(overflow)native_room_scroll_text(words,combined,tx,ty,tw,th,selected?native_room_marquee_offset(overflow,a.map_clock_ms-motion.buttons[i].start):0,plan.size);
  else native_room_compact_text(words,combined,tx,ty,tw,th);
  c.hit(x,y,width0,option_h,HitKind::Choice,i,enabled&&ready);
 }
 if(count>max_visible){char page[32]{};TextBuffer b{page,sizeof page,0,&c.a.content};b.number(first/max_visible+1);b.ch('/');b.number((count+max_visible-1)/max_visible);c.text(page,274,23,8,White,42);}
}
