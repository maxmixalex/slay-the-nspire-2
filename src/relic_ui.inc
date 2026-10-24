// Original-derived relic art and panels, adapted to the calculator's resolution.
static void relic_progress(const State&s,RelicMask id,char*out,size_t size,const Content*locale=nullptr){
    if(!size)return;out[0]=0;TextBuffer b{out,size,0,locale};unsigned limit=relic_counter_limit(id);
    if(!limit)return;
    if(id==CentennialPuzzle){b.text(relic_counter(s,id)?"本场已触发":"本场尚未触发");return;}
    b.text(relic_counter_is_turn_scoped(id)?"本回合 ":"累计 ");b.number(relic_counter(s,id));b.text(" / ");b.number(limit);
}
static void relic_badge(Canvas&c,RelicMask id,int x,int y){
    unsigned limit=relic_counter_limit(id);if(!limit)return;
    if(relic_counter_is_turn_scoped(id)&&!combat_phase(c.a.game.phase))return;
    unsigned value=relic_counter(c.a.game,id);
    bool active=c.a.relic_flash_ticks>0&&(c.a.relic_flash_mask&id);
    if(active)value=limit;
    c.rect(x,y,13,13,Panel);c.num(value,x+1,y-1,10,active?Green:Gold);
}
static bool ui_relic_flash(const App&a,RelicId id){return a.relic_flash_ticks>0&&(a.game.world_relics.active?a.world_relic_flash==id:id<RelicKinds&&(a.relic_flash_mask&relic_definitions[id].id));}
static unsigned ui_relic_opacity(const State&s,unsigned position){return s.world_relics.active&&position<s.world_relics.count&&((s.world_relics.entries[position].flags&WorldRelicMelted)||world_relic_disabled(s,position))?95:255;}
static void ui_relic_badge(Canvas&c,unsigned position,int x,int y){
 const auto&s=c.a.game;RelicId id=ui_owned_relic_id(s,position);
 if(!s.world_relics.active){if(id<RelicKinds)relic_badge(c,relic_definitions[id].id,x,y);return;}
 // The core owns the model-specific counter; presentation never treats an
 // arbitrary internal relic value as a visible counter.
 int value=0;if(!world_relic_display(s,position,value))return;
 c.rect(x,y,18,13,Panel);if(value<100)c.num(value,x+1,y-1,10,ui_relic_flash(c.a,id)?Green:Gold);else c.text("99+",x,y-1,10,Gold);
}
static void ui_relic_progress(const State&s,unsigned position,char*out,size_t size,const Content*locale=nullptr){
 if(!size)return;out[0]=0;RelicId id=ui_owned_relic_id(s,position);if(id==NoWorldRelic)return;
 if(!s.world_relics.active){if(id<RelicKinds)relic_progress(s,relic_definitions[id].id,out,size,locale);return;}
 const auto&r=s.world_relics.entries[position];TextBuffer b{out,size,0,locale};
 if(r.flags&WorldRelicMelted){b.text("已融化");return;}if(world_relic_disabled(s,position))b.text("已用尽");
 int value=0;if(world_relic_display(s,position,value)){if(b.n)b.text(" · ");b.text("计数：");b.number(value);}if(r.flags&WorldRelicWax)b.text(" · 蜡制遗物");
}
static void owned_relic_strip(Canvas&c){
 auto&a=c.a;auto&s=a.game;const unsigned count=ui_owned_relic_count(s);
 for(unsigned i=0;i<count;++i){
  const int x=hud_relic_layout::x(i),y=hud_relic_layout::y(i),cell=hud_relic_layout::Size;
  const bool interactive=!a.paused&&!a.help&&!a.settings_open&&!a.frontend_confirm&&a.detail_card<0&&a.detail_item<0&&a.status_actor<0&&!a.event_reader;
  const bool focused=interactive&&(a.pointer.visible?(a.pointer.x>=x&&a.pointer.x<x+cell&&a.pointer.y>=y&&a.pointer.y<y+cell):a.hud_focus==HudRelicFocusFirst+int(i));
  const RelicId id=ui_owned_relic_id(s,i);const uint16_t identity=s.world_relics.active?s.world_relics.entries[i].serial:uint16_t(id+1);
  const int scale=a.hud_relic_motion.observe(i,identity,focused,a.map_clock_ms),size=(hud_relic_layout::Icon*scale+2048)/4096;
  const int left=hud_relic_layout::center_x(i)-size/2,top=hud_relic_layout::center_y(i)-size/2;
  if(!a.reward_pickup_hidden(RewardPickupKind::Relic,i)){
   if(id<WorldRelicKinds){int outline=collection_assets::RelicOutline[id];
    if(id==WorldRelicYummyCookie)outline=collection_assets::CookieOutline[unsigned(s.character)<5?unsigned(s.character):0];
    if(outline>=0){Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.ui(outline,left,top,size,size,128);}}
   Canvas icon=c;if(s.world_relics.active&&world_relic_disabled(s,i))icon.brightness=128;
   icon.picture(a.relic_face_image(id),left,top,size,size);
   int value=0;bool counter=s.world_relics.active?world_relic_display(s,i,value):false;
   if(!s.world_relics.active&&id<RelicKinds){const auto legacy=relic_definitions[id].id;
    counter=relic_counter_limit(legacy)&&(!relic_counter_is_turn_scoped(legacy)||combat_phase(s.phase));value=int(relic_counter(s,legacy));}
   if(counter){char amount[16]{};TextBuffer text{amount,sizeof amount,0};text.number(value);const int tx=x+cell-c.width(amount,8),ty=y+cell-8;
    for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)c.text(amount,tx+dx,ty+dy,8,Ink);
    c.text(amount,tx,ty,8,rgb(255,246,226));}
   if(ui_relic_flash(a,id))c.picture(a.relic_face_image(id),left-1,top-1,size+2,size+2,100,presentation::Add);
  }
  c.hit(x,y,cell,cell,HitKind::OwnedRelic,int(i));
 }
}
static void owned_relic_panel(Canvas&c){
 auto&a=c.a;auto&s=a.game;const int count=ui_owned_relic_count(s);a.scroll=mn(mx(0,a.scroll),mx(0,count-1));int first=a.scroll/18*18;
 c.centered("遗物",5,28,310,16,Gold);
 for(int i=first;i<mn(count,first+18);i++){int j=i-first,x=20+j%6*49,y=55+j/6*35;RelicId id=ui_owned_relic_id(s,i);
  if(i==a.scroll){c.rect(x-4,y-2,38,33,rgb(75,58,37));c.box(x-4,y-2,38,33,Gold);}
  c.picture(a.relic_face_image(id),x,y,25,25,ui_relic_opacity(s,i));ui_relic_badge(c,i,x+20,y+16);c.hit(x-5,y-3,46,34,HitKind::Relic,i);
 }
 if(count){RelicId id=ui_owned_relic_id(s,a.scroll);c.centered(a.relic_title(id,a.scroll),15,161,290,12,Gold);
  char progress[128]{},description[2048]{};ui_relic_progress(s,a.scroll,progress,sizeof progress,&a.content);a.relic_description(id,description,sizeof description,a.scroll);
  if(*progress)c.centered(progress,15,180,290,10,Green);c.paragraph(description,20,*progress?197:182,280,*progress?1:2,0,10,White);
 }else c.centered("没有遗物",0,106,320,12,Muted);
 c.text("‹",15,216,16,Gold);c.hit(7,211,35,24,HitKind::Scroll,-18,first>0);c.text("›",292,216,16,Gold);c.hit(278,211,35,24,HitKind::Scroll,18,first+18<count);
 char page[64]{};TextBuffer b{page,sizeof page,0,&a.content};b.number(first/18+1);b.text(" / ");b.number(mx(1,(count+17)/18));c.centered(page,41,219,236,10,Muted);
}
static void owned_relic_tooltip(Canvas&c){
 auto&a=c.a;if(a.event_reader||a.pointer.held||a.title||a.paused||a.help||a.settings_open||a.frontend_confirm||(a.panel&&a.panel!=3)||a.library||a.character_select||a.map_preview.open||a.status_actor>=0||a.detail_card>=0||a.detail_item>=0||a.deck_candidate>=0||choice_phase(a.game.phase))return;
 const HitRegion*hit=a.pointer.visible?a.hit_at(a.pointer.x,a.pointer.y):nullptr;
 if(!a.pointer.visible&&a.hud_focus>=HudRelicFocusFirst)for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::OwnedRelic&&a.hits[i].value==a.hud_focus-HudRelicFocusFirst){hit=&a.hits[i];break;}
 if(!hit||hit->kind!=HitKind::OwnedRelic||hit->value<0||hit->value>=int(ui_owned_relic_count(a.game)))return;
 RelicId id=ui_owned_relic_id(a.game,hit->value);int x=mn(mx(4,hit->x-3),317-SourceHoverWidth),y=hit->y+hit->h+2;
 char progress[128]{},description[2048]{};ui_relic_progress(a.game,hit->value,progress,sizeof progress,&a.content);a.relic_description(id,description,sizeof description,hit->value);
 char body[2304]{};TextBuffer text{body,sizeof body,0};if(*progress){text.text("[green]");text.text(progress);text.text("[/green]\n");}text.text(description);
 source_hover_box(c,a.relic_title(id,hit->value),body,x,y);
}
#include "world_treasure_ui.inc"
static void treasure_screen(Canvas&c){
    if(c.a.game.world.active){native_treasure_screen(c);return;}
    auto&a=c.a;auto&s=a.game;c.picture(A_ChestBackground,0,20);
    if(!s.treasure_state){
        c.picture(A_Chest,128,91);c.centered("发现宝藏",0,37,320,20,Gold);
        c.button("打开宝箱",35,198,250,true,28);c.hit(35,198,250,28,HitKind::Choice,0);return;
    }
    c.dim(0,22,320,218,180);c.ui(U_RewardBanner,34,29,252,48);
    const auto*d=relic_definition(s.treasure_relic);bool taken=s.treasure_state==2;
    const RelicId world_id=s.world.active?s.world.treasure_relic:NoWorldRelic;char description[2048]{};if(world_id!=NoWorldRelic)a.relic_description(world_id,description,sizeof description);
    c.centered(taken?"已领取":"宝藏",47,40,226,20,Gold);
    unsigned opacity=taken?120:220+(a.animation%16<8?a.animation%8:15-a.animation%16)*4;
    c.picture(A_MapHalo,118,69,84,84,opacity);
    int image=world_id!=NoWorldRelic?a.relic_face_image(world_id):d?a.relic_face_image((int)(d-relic_definitions)):A_Coin;
    c.picture(image,138,88,44,44,taken?135:255);
    c.centered(world_id!=NoWorldRelic?a.relic_title(world_id):d?d->name:"50 金币",13,143,294,16,Gold);
    c.paragraph(world_id!=NoWorldRelic?description:d?d->description:"已获得全部可用遗物，改为获得 [gold]50[/gold] 金币。",31,165,258,2,0,12,White);
    c.hit(119,77,82,77,HitKind::Choice,0,!taken);
    c.button(taken?"已领取":"领取",7,209,99,a.cursor==0&&!taken,27);
    c.hit(4,207,105,31,HitKind::Choice,0,!taken);
    c.button("详情",112,211,91,false,24);c.hit(109,207,96,31,HitKind::Command,(int)Key::Detail,world_id!=NoWorldRelic||d!=nullptr);
    c.ui(U_RewardSkip,214,210,100,27);c.centered(taken?"前进 ›":"跳过 ›",214,214,100,12,a.cursor==1?Gold:White);
    c.hit(208,207,112,31,HitKind::Choice,1);
}
