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
static bool ui_relic_flash(const App&a,RelicId id,int position=-1){
 if(a.game.world_relics.active&&position>=0&&unsigned(position)<a.game.world_relics.count&&
    a.hud_relic_motion.flashing(unsigned(position),a.game.world_relics.entries[position].serial,a.map_clock_ms))return true;
 return !a.game.world_relics.active&&a.relic_flash_ticks>0&&id<RelicKinds&&(a.relic_flash_mask&relic_definitions[id].id);
}
static void relic_inventory_flash(Canvas&c,RelicId id,int x,int y,unsigned age){
 if(age>=1050)return;
 // The inventory owns relic_inventory_flash_vfx, not NRelicFlashVfx above
 // a creature. Its256px icon emits three750ms particles,150ms apart.
 // Cache the exact power_flash shader result at the native source extent;
 // neither its RGB squaring nor texture decode runs for every particle.
 constexpr int Side=49;
 struct Tile {const Content*owner=nullptr;uint32_t generation=0;int image=-1;uint16_t pixels[Side*Side]{};uint8_t alpha[Side*Side]{};};
 static Tile tiles[4];static unsigned next=0;
 const int image=c.a.relic_face_image(id,int(c.a.game.character));Tile*tile=nullptr;
 for(auto&v:tiles)if(v.owner==&c.a.content&&v.generation==c.a.content.generation()&&v.image==image){tile=&v;break;}
 if(!tile){tile=&tiles[next++%4];tile->owner=&c.a.content;tile->generation=c.a.content.generation();tile->image=image;
  for(unsigned i=0;i<Side*Side;++i){tile->pixels[i]=0;tile->alpha[i]=0;}
  Canvas raster{c.a,tile->pixels,Side,Side,Side,tile->alpha};raster.picture(image,0,0,Side,Side);
  for(unsigned i=0;i<Side*Side;++i){const unsigned color=tile->pixels[i],r=color>>11,g=color>>5&63,b=color&31;
   tile->pixels[i]=uint16_t((mn(31,int(2*r*r/31))<<11)|(mn(63,int(2*g*g/63))<<5)|mn(31,int(2*b*b/31)));
   tile->alpha[i]=uint8_t(div255(unsigned(tile->alpha[i])*tile->alpha[i]));}
 }
 const Picture picture{reinterpret_cast<const uint8_t*>(tile->pixels),tile->alpha,nullptr,Side,Side,1};
 for(unsigned i=0;i<3;++i){const unsigned delay=i*150u;if(age<delay||age-delay>=750u)continue;const unsigned t=age-delay;
  const unsigned q=t*4096u/750u,smooth=(q*q/4096u)*(12288u-2u*q)/4096u;
  const unsigned scale=2495u+3051u*smooth/4096u;
  const unsigned ramp=t<91u?t*255u/91u:t<450u?255u:(750u-t)*255u/300u;
  const unsigned alpha=div255(ramp*32u);const int size=int(Side*scale/4096u);
  c.picture(picture,x-size/2,y-size/2,size,size,alpha,presentation::Add);
 }
}
static unsigned ui_relic_opacity(const State&s,unsigned position){return s.world_relics.active&&position<s.world_relics.count&&((s.world_relics.entries[position].flags&WorldRelicMelted)||world_relic_disabled(s,position))?95:255;}
static void ui_relic_badge(Canvas&c,unsigned position,int x,int y){
 const auto&s=c.a.game;RelicId id=ui_owned_relic_id(s,position);
 if(!s.world_relics.active){if(id<RelicKinds)relic_badge(c,relic_definitions[id].id,x,y);return;}
 // The core owns the model-specific counter; presentation never treats an
 // arbitrary internal relic value as a visible counter.
 int value=0;if(!world_relic_display(s,position,value))return;
 c.rect(x,y,18,13,Panel);if(value<100)c.num(value,x+1,y-1,10,ui_relic_flash(c.a,id,int(position))?Green:Gold);else c.text("99+",x,y-1,10,Gold);
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
 a.hud_relic_motion.expire(a.map_clock_ms);
 for(unsigned i=0;i<count;++i){
  const int x=hud_relic_layout::x(i),y=hud_relic_layout::y(i),cell=hud_relic_layout::Size;
  const bool interactive=!a.paused&&!a.help&&!a.settings_open&&!a.frontend_confirm&&a.detail_card<0&&a.detail_item<0&&a.status_actor<0&&!a.event_reader;
  const bool focused=interactive&&(a.pointer.visible?(a.pointer.x>=x&&a.pointer.x<x+cell&&a.pointer.y>=y&&a.pointer.y<y+cell):a.hud_focus==HudRelicFocusFirst+int(i));
  const RelicId id=ui_owned_relic_id(s,i);const uint16_t identity=s.world_relics.active?s.world_relics.entries[i].serial:uint16_t(id+1);
  const int scale=a.hud_relic_motion.observe(i,identity,focused,a.map_clock_ms),size=(hud_relic_layout::Icon*scale+2048)/4096;
  const int left=hud_relic_layout::center_x(i)-size/2;int top=hud_relic_layout::center_y(i)-size/2;unsigned alpha=255;
  const auto&holder=a.hud_relic_motion.holders[i];
  if(holder.entering&&int32_t(a.map_clock_ms-holder.acquired)<350){
   const unsigned age=int32_t(a.map_clock_ms-holder.acquired)<0?0:a.map_clock_ms-holder.acquired;
   top+=(8*(4096-RewardPresentation::back_out(age*4096u/350u)))/4096;alpha=age<100u?age*255u/100u:255u;
  }
  if(!a.reward_pickup_hidden(RewardPickupKind::Relic,i)){
   if(id<WorldRelicKinds){int outline=collection_assets::RelicOutline[id];
    if(id==WorldRelicYummyCookie)outline=collection_assets::CookieOutline[unsigned(s.character)<5?unsigned(s.character):0];
    if(outline>=0){Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.ui(outline,left,top,size,size,128);}}
   Canvas icon=c;if(s.world_relics.active&&world_relic_disabled(s,i))icon.brightness=128;
   icon.picture(a.relic_face_image(id,int(s.character)),left,top,size,size,alpha);
   int value=0;bool counter=s.world_relics.active?world_relic_display(s,i,value):false;
   if(!s.world_relics.active&&id<RelicKinds){const auto legacy=relic_definitions[id].id;
    counter=relic_counter_limit(legacy)&&(!relic_counter_is_turn_scoped(legacy)||combat_phase(s.phase));value=int(relic_counter(s,legacy));}
   if(counter){char amount[16]{};TextBuffer text{amount,sizeof amount,0};text.number(value);const int tx=x+cell-c.width(amount,8),ty=y+cell-8;
    for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)c.text(amount,tx+dx,ty+dy,8,Ink);
    c.text(amount,tx,ty,8,rgb(255,246,226));}
   if(!s.world_relics.active&&ui_relic_flash(a,id,int(i)))c.picture(a.relic_face_image(id,int(s.character)),left-1,top-1,size+2,size+2,100,presentation::Add);
  }
  c.hit(x,y,cell,cell,HitKind::OwnedRelic,int(i));
 }
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
    int image=world_id!=NoWorldRelic?a.relic_face_image(world_id,taken?int(s.character):-1):d?a.relic_face_image((int)(d-relic_definitions),taken?int(s.character):-1):A_Coin;
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
