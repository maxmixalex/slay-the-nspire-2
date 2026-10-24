static const char* combat_orb_key(unsigned kind){
 static const char*const keys[]={"DARK_ORB","LIGHTNING_ORB","FROST_ORB","PLASMA_ORB","GLASS_ORB"};
 return kind<unsigned(DOrb::Count)?keys[kind]:"EMPTY_SLOT";
}
static void combat_orbs(Canvas&c){
 auto&a=c.a;const auto&s=a.game;const int slots=health_bar::bounded(a.combat_status_fact(CombatStatus::OrbSlots),0,MaxOrbs);
 const int count=health_bar::bounded(a.combat_status_fact(CombatStatus::OrbCount),0,slots);const uint32_t now=a.animation*33u;
 auto&motion=a.orb_presentation;motion.layout(unsigned(slots),now);
 const auto camera=s.enemy_world.active?encounter_camera(s.enemy_world.encounter):EncounterCamera{};
 const auto layout=a.creature_layout.right?a.creature_layout:combat_layout_for(s);const auto root=player_formation_point(s,camera.zoom);
 const auto player=a.combat_player_pose();const auto anchors=player_anchors(s.character);
 const int radius=mx(5,combat_project_extent(camera,layout,30)),base_size=mx(9,combat_project_extent(camera,layout,90));
 for(int i=0;i<slots;++i){
  const int kind=i<count?a.combat_status_fact(CombatStatus::OrbKind,i):255;motion.model(unsigned(i),unsigned(kind),now);
  const auto position=motion.position(unsigned(i),now);const int xx=anchors.intent_x*1024+position.x,yy=(anchors.intent_y+195)*1024+position.y;
  const int x=s.enemy_world.active?combat_project_x(camera,layout,root.x+int(int64_t(xx)*camera.zoom/1024),false):player.x+xx*4/(21*1024);
  const int y=s.enemy_world.active?combat_project_y(camera,layout,root.y+int(int64_t(yy)*camera.zoom/1024)):player.feet+yy*4/(21*1024);
  if(i<count){
   const unsigned age=now-motion.items[i].born;
   const int fade=age>=500?4096:int(health_bar::expo_out(age*2));
   const int scale=age>=500?4096:4096+(4096-fade)*CombatCardMotion::sine(int(age)*40960/1500-1024)/1024;
   const int size=base_size*scale/4096;
   if(size>0){Canvas orb=c;if(kind==int(DOrb::Glass)&&!a.combat_status_fact(CombatStatus::OrbPassive,i)){orb.tint_red=0;orb.tint_green=orb.tint_blue=133;}
    orb.ui(orb_image(unsigned(kind)),x-size/2,y-size/2,size,size);}
   if(kind!=int(DOrb::Plasma)&&age>=100){
    const int passive=a.combat_status_fact(CombatStatus::OrbPassive,i),evoke=a.combat_status_fact(CombatStatus::OrbEvoke,i);
    const int label_x=x+2,label_y=y+(kind==int(DOrb::Dark)?-9:-4);
    for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)c.num(passive,label_x+dx,label_y+dy,8,Ink);
    c.num(passive,label_x,label_y,8,White);
    if(kind==int(DOrb::Dark)){
     for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)c.num(evoke,label_x+dx,label_y+10+dy,8,Ink);
     c.num(evoke,label_x,label_y+10,8,rgb(94,255,255));
    }
   }
  }else{
   const unsigned age=now-motion.items[i].born;const int size=age>=250?radius*2:int(radius*2*age/250);
   if(size>0&&a.content.version()>=HistoryUi8ContentVersion)c.ui(I8_EmptyOrb,x-size/2,y-size/2,size,size);
   else if(size>2)c.box(x-size/2+1,y-size/2+1,size-2,size-2,Muted);
  }
  c.hit(x-radius,y-radius,radius*2,radius*2,HitKind::Orb,i);
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
  a.content.world_title("static_hover_tips","ENERGY_COUNT"),a.content.world_title("static_hover_tips","STAR_COUNT"),a.content.english()).format(source);
 if(!*title)title=a.content.ui_text(index<count?orb_name(kind):"空槽位");
 const int lines=c.paragraph(body,0,0,132,0,0,10),shown=mn(lines,11),height=29+shown*13;
 const int x=health_bar::bounded(hit->x+hit->w+4,3,173),y=health_bar::bounded(hit->y-height/2,24,mx(24,235-height));
 const int skin=a.content.world_image("common","HOVER_TIP");
 if(skin>=0){Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.nine_image(skin,x+2,y+2,144,height,28,22,46,16,64);c.nine_image(skin,x,y,144,height,28,22,46,16);}
 else{c.rect(x,y,144,height,Panel);c.box(x,y,144,height,Gold);}
 c.text(title,x+6,y+4,10,Gold,132);c.paragraph(body,x+6,y+23,132,shown,0,10,White);
 if(lines>shown)c.text("…",x+125,y+height-13,10,Gold);
}
