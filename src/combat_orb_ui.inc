static const char* combat_orb_key(unsigned kind){
 static const char*const keys[]={"DARK_ORB","LIGHTNING_ORB","FROST_ORB","PLASMA_ORB","GLASS_ORB"};
 return kind<unsigned(DOrb::Count)?keys[kind]:"EMPTY_SLOT";
}
static void combat_orb_visual(Canvas&c,unsigned kind,const CombatOrbPoint&point,uint32_t born,int passive,int evoke){
 const uint32_t now=c.a.animation*33u;const unsigned age=int32_t(now-born)<0?0u:now-born;
 const int x=point.x,y=point.y;
 if(kind<unsigned(DOrb::Count)){
  const int fade=age>=500?4096:int(health_bar::expo_out(age*2));
  const int scale=age>=500?4096:4096+(4096-fade)*CombatCardMotion::sine(int(age)*40960/1500-1024)/1024;
  const int size=point.size*scale/4096;
  if(size>0){Canvas orb=c;if(kind==unsigned(DOrb::Glass)&&!passive){orb.tint_red=0;orb.tint_green=orb.tint_blue=133;}
   orb.ui(orb_image(kind),x-size/2,y-size/2,size,size);}
  if(kind!=unsigned(DOrb::Plasma)&&age>=100){
   const int label_x=x+2,label_y=y+(kind==unsigned(DOrb::Dark)?-9:-4);
   for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)c.num(passive,label_x+dx,label_y+dy,8,Ink);
   c.num(passive,label_x,label_y,8,White);
   if(kind==unsigned(DOrb::Dark)){
    for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)c.num(evoke,label_x+dx,label_y+10+dy,8,Ink);
    c.num(evoke,label_x,label_y+10,8,rgb(94,255,255));
   }
  }
 }else{
  const int size=age>=250?point.radius*2:int(point.radius*2*age/250);
  if(size>0&&c.a.content.version()>=HistoryUi8ContentVersion)c.ui(I8_EmptyOrb,x-size/2,y-size/2,size,size);
  else if(size>2)c.box(x-size/2+1,y-size/2+1,size-2,size-2,Muted);
 }
}
static void combat_orbs(Canvas&c){
 auto&a=c.a;const uint32_t now=a.animation*33u;a.sync_orb_presentation();auto&motion=a.orb_presentation;
 for(const auto&ghost:motion.ghosts)if(ghost.kind<unsigned(DOrb::Count)&&int32_t(now-ghost.at)>=0&&now-ghost.at<250u){
  Canvas fading=c;fading.opacity=uint8_t(div255(fading.opacity*(250u-(now-ghost.at))*255u/250u));
  combat_orb_visual(fading,ghost.kind,combat_orb_point(a,ghost.point),ghost.born,ghost.passive,ghost.evoke);
 }
 for(unsigned i=0;i<motion.slots;++i){
  const auto point=combat_orb_point(a,motion.position(i,now));
  combat_orb_visual(c,motion.items[i].kind,point,motion.items[i].born,a.combat_status_fact(CombatStatus::OrbPassive,int(i)),a.combat_status_fact(CombatStatus::OrbEvoke,int(i)));
  c.hit(point.x-point.radius,point.y-point.radius,point.radius*2,point.radius*2,HitKind::Orb,int(i));
 }
}
struct CombatOrbTipContext {const App*a;int slot;};
static LocalizedValue combat_orb_tip_value(const void*opaque,const char*name){
 const auto&context=*static_cast<const CombatOrbTipContext*>(opaque);
 if(wt_equal(name,"Passive"))return wt_number(context.a->combat_status_fact(CombatStatus::OrbPassive,context.slot));
 if(wt_equal(name,"Evoke"))return wt_number(context.a->combat_status_fact(CombatStatus::OrbEvoke,context.slot));
 if(wt_equal(name,"energyPrefix"))return wt_number(1);return {};
}
static void orb_hover_tip(Canvas&c){
 auto&a=c.a;
 if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.title||a.library||a.paused||a.help||a.panel||
    a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||a.settings_open||a.frontend_confirm||(!combat_phase(a.game.phase)&&!a.choice_peeking))return;
 const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(!hit||hit->kind!=HitKind::Orb)return;
 const int slots=a.combat_status_fact(CombatStatus::OrbSlots),count=a.combat_status_fact(CombatStatus::OrbCount),index=hit->value;
 if(index<0||index>=slots)return;
 const unsigned kind=index<count?unsigned(a.combat_status_fact(CombatStatus::OrbKind,index)):255;
 const char*key=combat_orb_key(kind);const char*title=a.content.world_title("orbs",key);
 char path[80]{},body[2048]{};TextBuffer name{path,sizeof path,0};name.text(key);name.text(index<count?".smartDescription":".description");
 const char*source=a.content.world_string("orbs",path);
 if(!*source){path[0]=0;name.n=0;name.text(key);name.text(".description");source=a.content.world_string("orbs",path);}
 const CombatOrbTipContext context{&a,index};LocalizedTemplate(body,sizeof body,combat_orb_tip_value,&context,
  a.content.world_title("static_hover_tips","ENERGY_COUNT"),a.content.world_title("static_hover_tips","STAR_COUNT"),a.content.english()).for_language(a.content.language()).format(source);
 if(!*title)title=a.content.ui_text(index<count?orb_name(kind):"空槽位");
 // NOrb uses its bounds, the default 75% left/right threshold, and the
 // OrbModel's icon in the normal HoverTip header. The complete dynamic
 // Passive/Evoke body shares the same native 5px layout as other tips.
 const int x=hit->x>240?hit->x-SourceHoverWidth:hit->x+hit->w;
 source_hover_box(c,title,body,x,hit->y,SourceHoverWidth,index<count?a.content.ui_image(orb_image(kind)):-1);
}
