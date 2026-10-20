static bool ui_ancient_event(OriginalEventId id){switch(id){case OriginalEventId::Neow:case OriginalEventId::Darv:case OriginalEventId::Nonupeipe:case OriginalEventId::Tanx:case OriginalEventId::Orobas:case OriginalEventId::Pael:case OriginalEventId::Tezcatara:case OriginalEventId::Vakuu:return true;default:return false;}}
static const char* native_event_title(const App&a){const char*text=a.content.world_title("events",original_event_key(a.game.original_events.id));return *text?text:original_event_key(a.game.original_events.id);}
static void native_event_option(const App&a,int option,bool description,char*out,size_t size){
 const auto&e=a.game.original_events;if(!size)return;out[0]=0;
 if(ui_ancient_event(e.id)&&option>=0&&option<e.offer_count&&e.offers[option]<WorldRelicKinds){
  if(description)a.relic_description(RelicId(e.offers[option]),out,size);else{TextBuffer b{out,size,0};b.text(a.relic_title(RelicId(e.offers[option])));}return;
 }
 if(e.id==OriginalEventId::FakeMerchant&&option>=0&&option<6){
  if(e.offers[option]>=WorldRelicKinds){TextBuffer b{out,size,0};b.text("售罄");return;}
  if(description){a.relic_description(RelicId(e.offers[option]),out,size);return;}
  TextBuffer b{out,size,0};b.text(a.relic_title(RelicId(e.offers[option])));b.text(" · ");b.number(e.choice_prices[option]);b.text(" 金币");return;
 }
 a.original_event_text(option,description,out,size);
 if(!*out&&!description){TextBuffer b{out,size,0};b.text(e.finished?"继续前进":"继续");}
}
static void native_event_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const auto&e=s.original_events;
 c.picture(a.content.world_image("events",original_event_key(e.id)),0,22,320,218);
 for(int x=122;x<320;x++)c.dim(x,22,1,218,unsigned(mn(178,(x-122)*5)));
 c.centered(native_event_title(a),131,27,182,16,Gold);
 char text[8192]{};a.original_event_text(-1,true,text,sizeof text);int lines=c.paragraph(text,140,53,164,0,0,10);
 a.scroll=mn(mx(0,a.scroll),mx(0,lines-5));c.paragraph(text,140,53,164,5,a.scroll,10);
 c.text("全文 I",141,119,10,Gold);c.action(134,115,83,20,UiAction::ReadEvent);
 c.text("↑",271,119,10,a.scroll?Gold:Muted);c.hit(263,115,24,20,HitKind::Scroll,-1,a.scroll>0);
 c.text("↓",298,119,10,a.scroll+5<lines?Gold:Muted);c.hit(290,115,24,20,HitKind::Scroll,1,a.scroll+5<lines);
 const int count=original_event_option_count(s);a.cursor=mn(mx(0,a.cursor),mx(0,count-1));int first=a.cursor/3*3;
 for(int i=first;i<mn(count,first+3);i++){int y=137+(i-first)*33;bool enabled=original_event_option_enabled(s,i),selected=a.cursor==i;
  c.ui(U_EventButton,130,y,186,32,enabled?255:115);if(selected)c.ui(U_EventOutline,130,y,186,32,170);
  bool icon=(ui_ancient_event(e.id)||e.id==OriginalEventId::FakeMerchant)&&i<e.offer_count&&e.offers[i]<WorldRelicKinds;
  int tx=icon?163:140,tw=icon?141:164;if(icon)c.picture(a.content.relic_image(e.offers[i]),135,y+4,24,24,enabled?255:100);
  char label[256]{},description[2048]{};native_event_option(a,i,false,label,sizeof label);native_event_option(a,i,true,description,sizeof description);
  c.text(label,tx,y+1,10,enabled?Gold:Muted,tw);c.paragraph(description,tx,y+15,tw,1,0,8,enabled?White:Muted);
  c.hit(130,y,186,32,HitKind::Choice,i,enabled);
 }
 if(count){char focused[4096]{};native_event_option(a,a.cursor,true,focused,sizeof focused);c.dim(4,148,123,90,195);c.paragraph(focused,10,151,111,6,0,10,White);}
 if(count>3){char page[32]{};TextBuffer b{page,sizeof page,0};b.number(first+1);b.text("–");b.number(mn(first+3,count));b.text(" / ");b.number(count);c.text(page,8,132,10,Gold,116);}
}
