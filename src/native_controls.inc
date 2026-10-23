// Original component artwork preserves its aspect ratio, including the
// authored transparent margins. Callers own the larger readable hit target.
static void native_control(Canvas&c,const char*kind,int x,int y,int w,int h,const char*label,bool enabled,bool selected){
 const auto visual=c.control_visual(label,x,y,w,h,ButtonFamily::Action,selected,enabled);
 const int original_w=w;w=mx(1,w*visual.scale/4096);x+=(original_w-w)/2;
 const int image=c.a.content.world_image("common",kind);const unsigned opacity=255;
 Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(c.brightness)*visual.shade));
 if(image>=0){
  const auto info=c.a.content.image(image);const int height=info.w?int(w*info.h/info.w):h,top=y+(h-height)/2;
  Canvas shadow=layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.picture(image,x+2,top+2,w,height,64);
  layer.picture(image,x,top,w,height,opacity);
  char key[48]{};TextBuffer b{key,sizeof key,0};b.text(kind);b.text("_OUTLINE");
  if(visual.hover&&enabled){int outline=c.a.content.world_image("common",key);if(outline>=0){Canvas glow=layer;glow.tint_red=240;glow.tint_green=180;glow.tint_blue=0;glow.picture(outline,x,top,w,height,unsigned(visual.hover)*255/4096,presentation::Add);}}
 }else layer.ui(U_RewardSkip,x,y,w,h,opacity);
 int label_x=x+3,label_w=w-6;
 if(image>=0&&(kind[0]=='B'||kind[0]=='C')){int icon=c.a.content.world_image("common",kind[0]=='B'?"BACK_ARROW":"CONFIRM_TICK");
  if(icon>=0){layer.picture(icon,x+10,y+(h-15)/2,15,15,opacity);label_x=x+28;label_w=w-36;}}
 layer.centered(label,label_x+1,y+(h-12)/2+1,label_w,12,Ink);
 layer.centered(label,label_x,y+(h-12)/2,label_w,12,enabled?rgb(255,246,226):Muted);
}
static void native_proceed(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){
 const auto visual=c.control_visual(label,x,y,w,h,ButtonFamily::Action,selected,enabled);
 const int normal=c.a.content.world_image("common","PROCEED"),outline=c.a.content.world_image("common","PROCEED_OUTLINE");
 if(normal<0){native_control(c,"PROCEED",x,y,w,h,label,enabled,selected);return;}
 const int width=mx(1,w*visual.scale/4096),height=mx(1,width*192/393),left=x+(w-width)/2,top=mn(y+(h-height)/2,c.clip_h-height);
 Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(c.brightness)*visual.shade));
 Canvas shadow=layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.picture(normal,left+2,top+2,width,height,64);
 // This is the original atlas arrow, with its authored transparent margins.
 // The outlined skin already contains the scene's yellow self_modulate.
 if(outline>=0&&visual.hover&&enabled)layer.picture(outline,left,top,width,height,unsigned(visual.hover)*255/4096,presentation::Add);
 layer.picture(normal,left,top,width,height);
 const int text_y=top+(height-12)/2;
 layer.centered(label,left+1,text_y+1,width,12,rgb(87,16,8));
 layer.centered(label,left,text_y,width,12,enabled?rgb(255,246,226):Muted);
}
static void native_back(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){native_control(c,"BACK",x,y,w,h,label,enabled,selected);}
static void native_confirm(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){native_control(c,"CONFIRM",x,y,w,h,label,enabled,selected);}
static void native_end_turn(Canvas&c,bool enabled){
 auto&a=c.a;const auto visual=c.control_visual("END_TURN",254,173,66,31,ButtonFamily::EndTurn,false,enabled);
 const int shift=(visual.y+2048)/4096,normal=a.content.ui_image(U_EndTurnNative),hover=a.content.ui_image(U_EndTurnHover);
 bool playable=false;for(unsigned i=0;i<a.game.hand.count;i++)if(can_play_card(a.game,a.game.hand.ids[i])){playable=true;break;}
 const uint16_t encounter=a.game.enemy_world.active?uint16_t(256+unsigned(a.game.enemy_world.encounter)):a.game.encounter;
 const auto glow=a.end_turn_glow.observe({a.game.seed,a.game.floor,a.game.world.combat_count,encounter},enabled&&!playable,a.map_clock_ms);
 const int glow_image=a.content.ui_image(U_EndTurnGlow);
 const auto draw_glow=[&](int alpha,int scale,unsigned red,unsigned green){
  if(!alpha||glow_image<0)return;
  // All three original TextureRects have the same 512x256 bounds and
  // (256,128) pivot. Keep their shared center through the 320px adaptation.
  const int w=(78*scale+2048)/4096,h=(39*scale+2048)/4096;
  Canvas light=c;light.tint_red=uint8_t(div255(unsigned(c.tint_red)*red));
  light.tint_green=uint8_t(div255(unsigned(c.tint_green)*green));
  light.picture(glow_image,247+(78-w)/2,169+shift+(39-h)/2,w,h,unsigned(alpha)*255/4096,presentation::Add);
 };
 draw_glow(glow.pulse_alpha,glow.pulse_scale,64,223);
 Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(c.brightness)*visual.shade));
 if(normal>=0){layer.picture(normal,247,169+shift,78,39);
  if(visual.hover&&hover>=0)layer.picture(hover,247,169+shift,78,39,unsigned(visual.hover)*255/4096);
 }else layer.picture(A_EndTurn,254,174+shift,64,29);
 draw_glow(glow.base_alpha,glow.base_scale,0,212);
 const uint16_t cream=rgb(255,246,226),focus=playable?rgb(255,79,67):rgb(0,255,255);
 uint16_t color=enabled?blend(cream,focus,unsigned(visual.hover)*255/4096):Muted;
 layer.centered("结束回合",255,183+shift,64,10,rgb(19,31,46));layer.centered("结束回合",254,182+shift,64,10,color);
 c.hit(254,173,66,31,HitKind::Command,(int)Key::EndTurn,enabled);
}
