static bool ui_ancient_event(OriginalEventId id){switch(id){case OriginalEventId::Neow:case OriginalEventId::Darv:case OriginalEventId::Nonupeipe:case OriginalEventId::Tanx:case OriginalEventId::Orobas:case OriginalEventId::Pael:case OriginalEventId::Tezcatara:case OriginalEventId::Vakuu:return true;default:return false;}}
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
  TextBuffer b{out,size,0,&a.content};b.text(a.relic_title(RelicId(e.offers[option])));b.text(" · ");b.number(e.choice_prices[option]);b.text(" 金币");return;
 }
 if(e.id==OriginalEventId::FakeMerchant&&option==6){if(!description){TextBuffer b{out,size,0,&a.content};b.text("离开");}return;}
 a.original_event_text(option,description,out,size);
 if(!*out&&!description){TextBuffer b{out,size,0,&a.content};b.text(e.finished?"继续前进":"继续");}
}
static void native_ancient_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 c.centered(native_event_title(a),14,27,292,16,Gold);
 char key[128]{};TextBuffer k{key,sizeof key,0,&c.a.content};k.text(original_event_key(e.id));k.text(".epithet");
 c.centered(a.content.world_string("ancients",key),22,46,276,10,White);
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);
 const int lines=c.paragraph(text,35,61,250,0,0,10);
 a.scroll=mn(mx(0,a.scroll),mx(0,lines-2));
 if(*text){c.dim(27,58,266,31,120);c.paragraph(text,35,61,250,2,a.scroll,10);}
 c.text(a.content.english()?"Read":"全文",141,93,10,Gold);c.action(130,89,72,20,UiAction::ReadEvent);
 c.text("↑",265,93,10,a.scroll?Gold:Muted);c.hit(254,89,26,20,HitKind::Scroll,-1,a.scroll>0);
 c.text("↓",293,93,10,a.scroll+2<lines?Gold:Muted);c.hit(282,89,26,20,HitKind::Scroll,1,a.scroll+2<lines);
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));
 // The original ancient scene is a centered VBox of three 1000x100 rows.
 // Keep that hierarchy while enlarging type and targets for 320x240.
 for(int i=0;i<count&&i<3;i++){
  const int y=e.finished?196:110+i*43;bool enabled=original_event_option_enabled(s,i),selected=a.cursor==i;
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
  if(icon)c.picture(a.relic_face_image(e.offers[i]),22,y+6,28,28);
  char label[256]{},description[4096]{};native_event_option(a,i,false,label,sizeof label);native_event_option(a,i,true,description,sizeof description);
  int tx=icon?60:26,tw=icon?234:268;
  Canvas label_canvas=c;if(!enabled)label_canvas.opacity=uint8_t(div255(label_canvas.opacity*179));
  if(e.finished)label_canvas.centered(label,26,y+12,268,12,enabled?Gold:Red);
  else{
   label_canvas.text(label,tx,y+2,10,enabled?Gold:Red,tw);
   int rows=label_canvas.paragraph(description,tx,y+14,tw,2,0,10,White);
   if(rows>2){c.dim(283,y+26,14,12,180);c.text("…",285,y+26,10,enabled?Gold:Muted,12);}
  }
  c.hit(14,y,292,40,HitKind::Choice,i,enabled);
 }
}
static void native_fake_price(Canvas&c,int amount,int x,int y,bool enabled){
 char text[24]{};TextBuffer b{text,sizeof text,0,&c.a.content};b.number(amount);int left=x-(c.width(text,10)+13)/2;
 c.picture(A_Coin,left,y+2,10,10,enabled?255:135);c.text(text,left+12,y,10,enabled?Gold:Red);
}
static void native_fake_merchant_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);c.dim(0,22,320,218,100);
 c.picture(a.content.world_image("events","FAKE_MERCHANT_RUG"),0,38,320,180);
 c.centered(native_event_title(a),6,25,308,16,Gold);
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));
 // Authored relic origins in the 1747x978 rug: two, three, then one.
 // Icons and prices are enlarged independently so every stock slot is legible.
 static const int positions[6][2]={{869,218},{1144,288},{737,516},{1010,583},{1345,498},{823,847}};
 for(int i=0;i<6;i++){
  const int x=positions[i][0]*320/1747,y=38+positions[i][1]*180/978;
  const bool stocked=i<e.offer_count&&e.offers[i]<WorldRelicKinds,enabled=original_event_option_enabled(s,i);
  // A 36px width keeps the staggered top-right and middle-right targets
  // disjoint after the icon/type enlargement; their authored centers remain.
  if(a.cursor==i)c.box(x-18,y-17,36,46,stocked?Gold:Muted);
  if(stocked){c.picture(a.relic_face_image(e.offers[i]),x-14,y-14,28,28,enabled?255:125);native_fake_price(c,e.choice_prices[i],x,y+15,enabled);}
  else{c.dim(x-20,y-15,41,40,100);c.centered("售罄",x-20,y-3,41,10,Muted);}
  c.hit(x-18,y-17,36,46,HitKind::Choice,i,enabled);
 }
 char title[256]{},description[4096]{};
 if(a.cursor<6){
  if(a.cursor<e.offer_count&&e.offers[a.cursor]<WorldRelicKinds){TextBuffer b{title,sizeof title,0,&c.a.content};b.text(a.relic_title(RelicId(e.offers[a.cursor])));}
  else{TextBuffer b{title,sizeof title,0,&c.a.content};b.text("售罄");}
  native_event_option(a,a.cursor,true,description,sizeof description);
 }else{native_event_option(a,6,false,title,sizeof title);a.original_event_text(-1,true,description,sizeof description);}
 c.dim(4,59,106,140,185);c.paragraph(title,9,62,96,2,0,10,Gold);
 int lines=c.paragraph(description,9,91,96,0,0,10);a.scroll=mn(mx(0,a.scroll),mx(0,lines-6));
 c.paragraph(description,9,91,96,6,a.scroll,10,White);
 c.text(a.content.english()?"Read":"全文",10,181,10,Gold);c.action(4,176,66,22,UiAction::ReadEvent);
 c.text("↑",77,181,10,a.scroll?Gold:Muted);c.hit(71,176,19,22,HitKind::Scroll,-1,a.scroll>0);
 c.text("↓",97,181,10,a.scroll+6<lines?Gold:Muted);c.hit(91,176,19,22,HitKind::Scroll,1,a.scroll+6<lines);
 char leave[128]{};native_event_option(a,6,false,leave,sizeof leave);
 c.ui(U_RewardSkip,219,208,96,29);if(a.cursor==6)c.box(219,208,96,29,Gold);
 c.centered(leave,225,214,84,12,Gold);c.hit(218,205,98,34,HitKind::Choice,6,original_event_option_enabled(s,6));
}
static void native_event_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;
 if(ui_ancient_event(e.id)){native_ancient_screen(c);return;}
 if(e.id==OriginalEventId::FakeMerchant&&!e.finished){native_fake_merchant_screen(c);return;}
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 // default_event_layout.tscn places a centered story above one options VBox
 // on the right of the illustration. No extra dark column or duplicate
 // selected-description box belongs on top of the event artwork.
 c.fitted(native_event_title(a),131,30,182,19,16,12,Gold,true,true,false,true);
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);
 const auto story=c.fitted(text,137,54,173,65,10,10,White,true,false,false,true);
 if(story.truncated)c.text("…",298,106,10,Gold,12);
 c.action(132,51,183,68,UiAction::ReadEvent);
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));int first=a.cursor/3*3;
 for(int i=first;i<mn(count,first+3);i++){const int y=123+(i-first)*38;const bool enabled=original_event_option_enabled(s,i),selected=a.cursor==i;
  // NEventOptionButton: a shadow plus a blue additive focus outline, and a
  // single rich label with a gold/red bold title followed by its description.
  Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.ui(U_EventOutline,130,y+1,186,37,128);
  Canvas button=c;button.brightness=uint8_t(div255(button.brightness*(enabled?(selected?255:229):166)));
  button.ui(U_EventButton,130,y,186,37,230);
  if(selected&&enabled){Canvas outline=c;outline.tint_red=uint8_t(div255(outline.tint_red*120));outline.tint_blue=uint8_t(div255(outline.tint_blue*254));
   outline.picture(a.content.ui_image(U_EventOutline),129,y-1,188,39,192,presentation::Add);}
  char label[256]{},description[2048]{},combined[2432]{};native_event_option(a,i,false,label,sizeof label);native_event_option(a,i,true,description,sizeof description);
  TextBuffer body{combined,sizeof combined,0,&a.content};
  if(*label){body.text(enabled?"[gold][b]":"[red][b]");body.text(label);body.text(enabled?"[/b][/gold]":"[/b][/red]");if(*description)body.ch('\n');}body.text(description);
  Canvas words=c;if(!enabled)words.opacity=uint8_t(div255(words.opacity*179));
  words.paragraph_fitted(combined,140,y+3,164,32,10,White,8);
  c.hit(130,y,186,37,HitKind::Choice,i,enabled);
 }
 if(count>3){char page[32]{};TextBuffer b{page,sizeof page,0,&c.a.content};b.number(first/3+1);b.ch('/');b.number((count+2)/3);c.text(page,285,23,8,White,30);}
}
