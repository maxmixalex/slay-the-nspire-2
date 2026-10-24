static uint16_t crystal_word(const Picture&p,unsigned at){
 const uint8_t*b=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
 return uint16_t(b[0]|unsigned(b[1])<<8);
}
static unsigned crystal_noise(const Picture&p,unsigned radius,unsigned angle,unsigned fraction){
 const unsigned row=radius*256,one=crystal_word(p,row+angle),two=crystal_word(p,row+((angle+1)&255));
 return (one*(256-fraction)+two*fraction)>>8;
}
// Software shader: no trigonometry, power functions, allocations or texture
// decompression in the pixel loop. Offline tables preserve the source's two
// independent TIME rates; changing cells affects only coverage and borders.
static void crystal_sphere_layer(Canvas&c,bool fog,crystal_shader::Phase phase){
 auto&a=c.a;const auto&motion=a.crystal_presentation;const uint32_t now=a.map_clock_ms;
 const int x=fog?crystal_layout::FogX:crystal_layout::SphereX,y=fog?crystal_layout::FogY:crystal_layout::SphereY;
 const int w=fog?crystal_layout::FogW:crystal_layout::SphereW,h=fog?crystal_layout::FogH:crystal_layout::SphereH;
 const auto polar=a.content.image(a.content.ui_image(I8_CrystalPolar));
 const auto n1=a.content.image(a.content.ui_image(I8_CrystalNoiseOne)),n2=a.content.image(a.content.ui_image(I8_CrystalNoiseTwo));
 const auto inside=a.content.image(a.content.ui_image(fog?I8_CrystalInner:I8_CrystalOuterInside));
 const auto outside=fog?inside:a.content.image(a.content.ui_image(I8_CrystalOuterOutside));
 if(!polar.pixels||!n1.pixels||!n2.pixels||!inside.pixels||!outside.pixels)return;
 if(polar.format>3||n1.format>3||n2.format>3||inside.format>3||outside.format>3||polar.w!=160||polar.h!=160||n1.w!=256||n1.h!=240||n2.w!=256||n2.h!=240||inside.w!=256||inside.h!=240||outside.w!=256||outside.h!=240)return;
 const unsigned phase1=phase.one,phase2=phase.two;
 uint8_t alpha[CrystalCells];for(unsigned i=0;i<CrystalCells;++i)alpha[i]=uint8_t(fog?motion.cell_alpha(i,now):255);
 static_assert(crystal_layout::SphereW==158&&crystal_layout::SphereH==157&&crystal_layout::FogW==159&&crystal_layout::FogH==160,"Update exact source coordinate tables with the shader rectangles");
 const auto*horizontal=fog?crystal_shader::FogX.pixels:crystal_shader::OuterX.pixels;
 const auto*vertical=fog?crystal_shader::FogY.pixels:crystal_shader::OuterY.pixels;
 for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){
  const auto&column=horizontal[xx];const auto&row=vertical[yy];
  const int cx=column.cell,cy=row.cell;const bool valid=crystal_layout::exists(cx,cy);
  if(fog&&!valid)continue;
  const unsigned packed=crystal_word(polar,unsigned(row.sample)*160+column.sample),radius=packed>>8,angle=packed&255;
  const unsigned one=crystal_noise(n1,radius,(angle+(phase1>>8))&255,phase1&255);
  const unsigned two=crystal_noise(n2,radius,(angle+(phase2>>8))&255,phase2&255);
  const unsigned at=radius*256+((one*two)>>24);const Picture&table=valid?inside:outside;
  unsigned opacity=table.alpha?table.alpha[at]:255;
  uint16_t color=crystal_word(table,at);
  if(fog){
   const unsigned cell=unsigned(cy*11+cx),coverage=alpha[cell];unsigned border=0;
   const auto edge=[&](int nx,int ny,unsigned weight){if(nx<0||nx>=11||ny<0||ny>=11||!weight)return;
    const int difference=int(coverage)-int(alpha[ny*11+nx]);border=unsigned(mx(int(border),int(div255(unsigned(difference<0?-difference:difference)*weight))));};
   edge(cx-1,cy,column.low);edge(cx+1,cy,column.high);edge(cx,cy-1,row.low);edge(cx,cy+1,row.high);
   opacity=div255(opacity*unsigned(mx(coverage,border)));if(!opacity)continue;
   color=uint16_t((mn(31,(color>>11)+int(border*28/255))<<11)|(mn(63,((color>>5)&63)+int(border*56/255))<<5)|mn(31,(color&31)+int(border*31/255)));
  }
  c.paint((y+yy)*c.pitch+x+xx,color,opacity);
 }
}
static void crystal_dialogue(Canvas&c){
 auto&a=c.a;const auto&m=a.crystal_presentation;const uint32_t age=a.map_clock_ms-m.dialogue_start;
 const unsigned alpha=m.dialogue_alpha(a.map_clock_ms);if(!alpha)return;
 const int y=46-15*(int(room_curve::Q)-room_curve::back_out(room_curve::linear(age,500)))/int(room_curve::Q);
 Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*alpha));
 const auto p=a.content.image(a.content.ui_image(I8_CrystalDialogue));
 const unsigned scale=1365+2731*room_curve::expo_out(age,500)/room_curve::Q;
 const int width=p.w*int(scale)/4096,height=p.h*int(scale)/4096;
 Canvas shadow=layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
 shadow.ui(I8_CrystalDialogue,m.dialogue_x-8-width/2,y-5-height/2+1,width,height,64);
 layer.ui(I8_CrystalDialogue,m.dialogue_x-8-width/2,y-5-height/2,width,height);
 char key[96]{},text[512]{};TextBuffer b{key,sizeof key,0};b.text("CRYSTAL_SPHERE.banter.");
 b.text(m.dialogue==1?"START.":m.dialogue==2?"REVEAL_GOOD.":m.dialogue==3?"REVEAL_BAD.":"END.");b.number(m.line);
 a.localized_text("events",key,text,sizeof text);layer.opacity=uint8_t(div255(layer.opacity*191));
 layer.text_visible_characters=room_rich_characters(text)*room_curve::linear(age,400)/room_curve::Q;
 layer.fitted(text,m.dialogue_x-35,y-13,54,25,6,6,rgb(0,0,0),true,true,true,false);
}
static bool crystal_backdrop(Canvas&layer){
 auto&a=layer.a;const auto&e=a.game.original_events;auto&motion=a.crystal_presentation;
 const bool resources=a.content.version()>=HistoryUi8ContentVersion;
 const auto phase=resources?crystal_shader::phase(a.map_clock_ms):crystal_shader::Phase{};
 if(resources)layer.ui(I8_CrystalBackground,0,0,320,240);
 else layer.picture(a.content.world_image("crystal","CRYSTAL_SPHERE_MINIGAME_BG"),-94,3,507,238);
 if(resources)crystal_sphere_layer(layer,false,phase);
 if(a.map_clock_ms-motion.start>=500)for(unsigned i=0;i<e.crystal_item_count;++i){
  const auto&item=e.crystal_items[i];if(!item.w||!item.h)continue;
  const int x=crystal_layout::cell_edge(item.x),y=crystal_layout::cell_top(item.y)-1;
  const int base_w=crystal_layout::cell_edge(item.x+item.w)-x,base_h=crystal_layout::cell_top(item.y+item.h)-y-1;
  const unsigned scale=motion.item_scale(i,a.map_clock_ms);const int w=base_w*int(scale)/1024,h=base_h*int(scale)/1024;
  Canvas icon=layer;icon.brightness=uint8_t(motion.item_brightness(i,a.map_clock_ms));
  icon.picture(a.content.world_image("crystal",crystal_item_image(item)),x+(base_w-w)/2,y+(base_h-h)/2,w,h);
 }
 if(resources){crystal_sphere_layer(layer,true,phase);layer.ui(I8_CrystalOutline,56,71,134,134);}
 return resources;
}
static void crystal_screen(Canvas&c){
 auto&a=c.a;const auto&e=a.game.original_events;a.sync_crystal_presentation();auto&motion=a.crystal_presentation;
 a.hit_count=0;Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*motion.screen_alpha(a.map_clock_ms)));
 const bool resources=crystal_backdrop(layer);
 int hovered=-1;const bool enabled=!e.finished&&e.crystal_charges&&a.map_clock_ms-motion.start>=500;
 for(int y=0;y<11;++y)for(int x=0;x<11;++x){if(!crystal_layout::exists(x,y))continue;const int at=y*11+x;
  const int xx=crystal_layout::cell_edge(x),yy=crystal_layout::cell_top(y),w=crystal_layout::cell_edge(x+1)-xx,h=crystal_layout::cell_top(y+1)-yy;
  if(!resources&&!e.crystal_clear[at])layer.rect(xx,yy,w,h,rgb(64,46,94));
  if(enabled&&!e.crystal_clear[at]){
   if(a.pointer.visible?(a.pointer.x>=xx&&a.pointer.x<xx+w&&a.pointer.y>=yy&&a.pointer.y<yy+h):a.cursor==at)hovered=at;
   c.hit(xx,yy,w,h,HitKind::CrystalCell,at,true);
  }
 }
 motion.focus(hovered,e.crystal_tool,a.map_clock_ms);
 for(unsigned at=0;at<CrystalCells;++at){const unsigned alpha=motion.hover_alpha(at,a.map_clock_ms);if(!alpha||e.crystal_clear[at])continue;
  const int x=crystal_layout::cell_edge(at%11),y=crystal_layout::cell_top(at/11);
  layer.picture(a.content.world_image("crystal","CRYSTAL_BALL_SINGLE_SQUARE_UI"),x,y,crystal_layout::cell_edge(at%11+1)-x,crystal_layout::cell_top(at/11+1)-y,alpha);
 }
 char text[2048]{};
 if(!e.finished){
  a.localized_text("events","CRYSTAL_SPHERE.minigame.divinationsRemain",text,sizeof text);layer.text(text,12,223,6,rgb(255,246,226),220);
  for(int index=0;index<2;++index){const int tool=index?1:2,x=240,y=65+index*29,w=75,h=25;
   const bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h;
   Canvas button=layer;button.font_face=5;button.brightness=hover?255:204;
   button.picture(a.content.world_image("crystal","DIVINE_BUTTON"),x,y,w,h);
   if(e.crystal_tool==tool)layer.picture(a.content.world_image("crystal","DIVINE_BUTTON_OUTLINE"),x,y,w,h);
   layer.picture(a.content.world_image("crystal",tool==1?"SMALL_DIVINATION_ICON":"BIG_DIVINATION_ICON"),x+8,y+4,16,16);
   a.localized_text("events",tool==1?"CRYSTAL_SPHERE.button.DIVINATION_LABEL_SMALL":"CRYSTAL_SPHERE.button.DIVINATION_LABEL_BIG",text,sizeof text);
   button.fitted(text,x+25,y+4,45,17,8,6,rgb(255,246,226),true,false,true,false);
   c.action(x,y,w,h,tool==1?UiAction::CrystalSmall:UiAction::CrystalLarge,enabled);
  }
  Canvas instructions=layer;instructions.tint_red=4;instructions.tint_green=0;instructions.tint_blue=40;
  instructions.nine_ui(I7_TinyPanel,240,128,75,89,2,2,2,2,192);
  a.localized_text("events","CRYSTAL_SPHERE.minigame.instructions.title",text,sizeof text);
  layer.fitted(text,244,132,67,12,6,6,rgb(254,209,0),true,true,true,false);
  a.localized_text("events","CRYSTAL_SPHERE.minigame.instructions.description",text,sizeof text);
  layer.fitted(text,245,146,65,68,6,6,rgb(255,246,226),true,false,false,false);
 }else{
  a.cursor=0;
  const char*proceed=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
  native_proceed(layer,*proceed?proceed:"前进",true);native_proceed_choice(c,0);
 }
 if(resources)crystal_dialogue(layer);
}
