// Original component artwork preserves its aspect ratio, including the
// authored transparent margins. Callers own the larger readable hit target.
static void native_control(Canvas&c,const char*kind,int x,int y,int w,int h,const char*label,bool enabled,bool selected,bool cross=false){
 const auto visual=c.control_visual(label,x,y,w,h,kind[0]=='B'?ButtonFamily::Back:ButtonFamily::Action,selected,enabled);
 const int image=c.a.content.world_image("common",kind);const unsigned opacity=255;
 int image_h=h;
 if(image>=0){const auto info=c.a.content.image(image);
  if(info.w&&info.h){const int fitted=mn(w,h*info.w/info.h);
   const int scaled=mx(1,fitted*visual.scale/4096);x+=(w-scaled)/2;w=scaled;image_h=mx(1,w*info.h/info.w);}}
 Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(c.brightness)*visual.shade));
 if(image>=0){
  const int height=image_h,top=y+(h-height)/2;
  Canvas shadow=layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.picture(image,x+2,top+2,w,height,64);
  layer.picture(image,x,top,w,height,opacity);
  char key[48]{};TextBuffer b{key,sizeof key,0};b.text(kind);b.text("_OUTLINE");
  if(visual.hover&&enabled){int outline=c.a.content.world_image("common",key);if(outline>=0){Canvas glow=layer;glow.tint_red=240;glow.tint_green=180;glow.tint_blue=0;glow.picture(outline,x,top,w,height,unsigned(visual.hover)*255/4096,presentation::Add);}}
 }else layer.ui(U_RewardSkip,x,y,w,h,opacity);
 int label_x=x+3,label_w=w-6;
 if(image>=0&&(kind[0]=='B'||kind[0]=='C')){int icon=cross&&c.a.content.version()>=Ui6ContentVersion?c.a.content.ui_image(I6_CloseX):c.a.content.world_image("common",kind[0]=='B'?"BACK_ARROW":"CONFIRM_TICK");
  if(icon>=0){
   if(!*label){
    // The character-select ConfirmButton contains an 80px cream tick on
    // its 267px image, and no text node. Keep the glyph centered in the
    // calculator target while bounding it to the available height.
    const int size=mn(h,mx(1,w*80/267));Canvas glyph=layer;
    glyph.tint_green=uint8_t(div255(unsigned(glyph.tint_green)*246));glyph.tint_blue=uint8_t(div255(unsigned(glyph.tint_blue)*226));
    glyph.picture(icon,x+(w-size)/2,y+(h-size)/2,size,size,opacity);return;
   }
   layer.picture(icon,x+10,y+(h-15)/2,15,15,opacity);label_x=x+28;label_w=w-36;}}
 layer.centered(label,label_x+1,y+(h-12)/2+1,label_w,12,Ink);
 layer.centered(label,label_x,y+(h-12)/2,label_w,12,enabled?rgb(255,246,226):Muted);
}
static void native_proceed(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){
 const auto visual=c.control_visual(label,x,y,w,h,ButtonFamily::Action,selected,enabled);
 const int normal=c.a.content.world_image("common","PROCEED"),outline=c.a.content.world_image("common","PROCEED_OUTLINE");
 if(normal<0){native_control(c,"PROCEED",x,y,w,h,label,enabled,selected);return;}
 const int fitted=mn(w,h*393/192),width=mx(1,fitted*visual.scale/4096),height=mx(1,width*192/393),left=x+(w-width)/2,top=mn(y+(h-height)/2,c.clip_h-height);
 Canvas layer=c;layer.brightness=uint8_t(div255(unsigned(c.brightness)*visual.shade));
 Canvas shadow=layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.picture(normal,left+2,top+2,width,height,64);
 // This is the original atlas arrow, with its authored transparent margins.
 // The outlined skin already contains the scene's yellow self_modulate.
 if(outline>=0&&visual.hover&&enabled)layer.picture(outline,left,top,width,height,unsigned(visual.hover)*255/4096,presentation::Add);
 layer.picture(normal,left,top,width,height);
 const int text_y=top+(height-12)/2;
 const int label_x=left+width*57/393,label_w=width*240/393;
 layer.text_fitted(label,label_x+1,text_y+1,label_w,12,rgb(87,16,8),8,true);
 layer.text_fitted(label,label_x,text_y,label_w,12,enabled?rgb(255,246,226):Muted,8,true);
}
static void native_back(Canvas&c,int x,int y,int w,int h,const char*,bool enabled=true,bool selected=false){native_control(c,"BACK",x,y,w,h,"",enabled,selected);}
// NBackButton positions its Control separately from its larger child sprite.
// NGame FourByThree is 1680x1260. Source Image/Outline bounds are 267x150
// and 273x156; KeepAspectCentered fits the complete 300x202 AtlasTexture
// (including its margin), not just its 267x160 region. The 80px icon retains
// its independent Control origin. The hit rectangle is a different node.
static void native_original_back(Canvas&c,uint32_t opened=UINT32_MAX){
 auto&a=c.a;const unsigned age=a.map_clock_ms-opened;
 const int entry=opened==UINT32_MAX||age>=350?0:-34*(4096-room_curve::back_out(age*4096/350))/4096;
 const auto visual=c.control_visual("ORIGINAL_BACK",-8+entry,173,38,21,ButtonFamily::Back,false,true);
 const int pivot_x=-4+entry,pivot_y=180,scale=int(visual.scale);
 const auto draw=[&](Canvas&layer,int id,int x,int y,int w,int h,unsigned alpha,unsigned mode){
  if(id>=0)layer.picture(id,pivot_x+(x+entry-pivot_x)*scale/4096,pivot_y+(y-pivot_y)*scale/4096,
                       mx(1,w*scale/4096),mx(1,h*scale/4096),alpha,mode);
 };
 const int back=a.content.world_image("common","BACK"),outline=a.content.world_image("common","BACK_OUTLINE");
 Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
 draw(shadow,back,-5,172,42,29,64,presentation::Normal);
 if(visual.hover)draw(c,outline,-8,170,44,30,unsigned(visual.hover)*255/4096,presentation::Add);
 Canvas face=c;face.brightness=uint8_t(div255(unsigned(face.brightness)*visual.shade));
 draw(face,back,-7,170,42,29,255,presentation::Normal);
 draw(face,a.content.world_image("common","BACK_ARROW"),5,175,15,15,255,presentation::Normal);
 c.action(-8+entry,173,38,21,UiAction::Back);
}
static void native_close(Canvas&c,int x,int y,int w,int h,bool selected=false){native_control(c,"BACK",x,y,w,h,"",true,selected,true);}
static void native_confirm(Canvas&c,int x,int y,int w,int h,const char*label,bool enabled=true,bool selected=false){native_control(c,"CONFIRM",x,y,w,h,label,enabled,selected);}
static void native_original_confirm(Canvas&c,UiAction action,uint32_t opened=UINT32_MAX){
 auto&a=c.a;const unsigned age=a.map_clock_ms-opened;
 const int entry=opened==UINT32_MAX||age>=350?0:34*(4096-room_curve::back_out(age*4096/350))/4096;
 // NConfirmButton derives (1520,906) from the actual 1680x1260 viewport.
 // Its child textures use the same full AtlasTexture fitting as Back.
 const auto visual=c.control_visual("ORIGINAL_CONFIRM",290+entry,173,38,21,ButtonFamily::Action,false,true);
 const int pivot_x=324+entry,pivot_y=180,scale=int(visual.scale);
 const auto draw=[&](Canvas&layer,int id,int x,int y,int w,int h,unsigned alpha,unsigned mode){
  if(id>=0)layer.picture(id,pivot_x+(x+entry-pivot_x)*scale/4096,pivot_y+(y-pivot_y)*scale/4096,
                       mx(1,w*scale/4096),mx(1,h*scale/4096),alpha,mode);
 };
 const int face=a.content.world_image("common","CONFIRM"),outline=a.content.world_image("common","CONFIRM_OUTLINE");
 Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
 draw(shadow,face,286,172,42,29,64,presentation::Normal);
 if(visual.hover)draw(c,outline,283,170,44,30,unsigned(visual.hover)*255/4096,presentation::Add);
 Canvas button=c;button.brightness=uint8_t(div255(unsigned(c.brightness)*visual.shade));
 draw(button,face,284,170,42,29,255,presentation::Normal);
 button.tint_green=uint8_t(div255(unsigned(button.tint_green)*246));button.tint_blue=uint8_t(div255(unsigned(button.tint_blue)*226));
 draw(button,a.content.world_image("common","CONFIRM_TICK"),296,175,15,15,255,presentation::Normal);
 c.action(290+entry,173,38,21,action);
}
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
