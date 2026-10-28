unsigned App::camp_options(uint8_t*out,unsigned capacity)const{
 unsigned count=0;for(unsigned i=0;i<world_rest_option_count(game);++i)if(world_rest_option_visible(game,i)){if(out&&count<capacity)out[count]=uint8_t(i);++count;}return count;
}
void App::sync_camp_presentation(){
 const auto&w=game.world;if(!w.active||w.room_type!=unsigned(WorldRoom::Rest)||game.phase!=Phase::Camp)return;
 auto&m=room_presentation.rest;uint32_t next=0;for(unsigned i=0;i<w.rest_count;++i)if(world_rest_option_visible(game,i))next|=uint32_t(1)<<i;
 if(m.selection.pending&&!w.rest_pending){
  const auto&selection=m.selection;const bool completed=selection.slot>=0&&
   (w.rest_used&~selection.used&(uint32_t(1)<<unsigned(selection.slot)));
  if(completed){
   // HideChoices runs beside DoLocalPostSelectVfx. Only its completion
   // enables first Proceed/travel; the authoritative option is already done.
   unsigned wait=500;const auto option=WorldRestOption(selection.option);
   if(option==WorldRestOption::Heal){wait=fast_mode?1500:2500;m.heal_start=map_clock_ms;m.healing=true;}
   else if(option==WorldRestOption::Smith){
    wait=fast_mode?1000:2000;
    // NCardSmithVfx emits three weak short directional punches after its
    //250ms card reveal. Ordinary NCardUpgradeVfx does not shake the room.
    for(unsigned i=0;i<3;++i){const int degrees=170+int(ScreenShakePresentation::hash(map_clock_ms+i*0x9e3779b9u)%21u);
     screen_effect(ScreenShakePresentation::Punch,ScreenShakePresentation::Weak,300,unsigned(degrees*4096/360),250u+i*250u);}
   }
   else if(option==WorldRestOption::Lift||option==WorldRestOption::Clone)screen_effect(ScreenShakePresentation::Punch,ScreenShakePresentation::Strong);
   m.complete(map_clock_ms,wait);
  }else m.cancel(next,map_clock_ms); // canceled Smith/Cook keeps the room's existing controls
 }
 if(m.hiding&&!m.waiting(map_clock_ms)){
  const uint32_t appeared=m.post_start+m.post_wait;m.hiding=false;m.show(next,appeared,true);
 }else m.observe(next,map_clock_ms);
}
bool App::camp_proceed_ready()const{
 return !game.world.active||game.world.room_type!=unsigned(WorldRoom::Rest)||
  room_presentation.rest.proceed_ready(map_clock_ms);
}
static unsigned camp_selected(const App&a,const uint8_t*options,unsigned count){for(unsigned i=0;i<count;++i)if(options[i]==a.cursor)return i;return 0;}
void App::camp_page(int direction){
 if(!native_room_input_available(*this,Phase::Camp)||room_presentation.rest.choices_locked())return;uint8_t options[MaxWorldRestOptions];const unsigned count=camp_options(options,MaxWorldRestOptions);if(!count)return;
 const int page=int(camp_selected(*this,options,count)/4),last=int((count-1)/4);cursor=options[unsigned(mn(mx(0,page+direction),last))*4];pointer_cancel();
}
bool App::camp_input(Key key){
 if(!native_room_input_available(*this,Phase::Camp))return false;
 if(content.has_ftue_assets()&&!seen_ftue(FtueId::RestSite)&&map_clock_ms-ftue.room_since<500)return true;
 sync_camp_presentation();auto&motion=room_presentation.rest;
 uint8_t options[MaxWorldRestOptions];const unsigned count=camp_options(options,MaxWorldRestOptions);unsigned selected=camp_selected(*this,options,count);
 if(key==Key::EndTurn){if(camp_proceed_ready()&&world_rest_proceed(game))proceed_map_window();return true;}
 if(motion.choices_locked())return key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down||key==Key::Confirm;
 if(key==Key::Left||key==Key::Up){if(count)cursor=options[(selected+count-1)%count];return true;}
 if(key==Key::Right||key==Key::Down){if(count)cursor=options[(selected+1)%count];return true;}
 if(key==Key::Confirm){
  if(!count){if(camp_proceed_ready()&&world_rest_proceed(game))proceed_map_window();return true;}cursor=options[selected];
  if(!world_rest_option_enabled(game,unsigned(cursor))){message("此选项当前不可用");return true;}
  uint32_t enabled=0;for(unsigned i=0;i<game.world.rest_count;++i)if(world_rest_option_enabled(game,i))enabled|=uint32_t(1)<<i;
  motion.select(unsigned(cursor),game.world.rest_options[cursor],enabled,game.world.rest_used,world_rest_can_proceed(game));
  if(world_rest_choose(game,unsigned(cursor))){
   choice_selected_count=0;choice_revision=game.revision;scroll=0;pointer_cancel();
   uint8_t remaining[MaxWorldRestOptions];if(game.phase==Phase::Camp){const unsigned n=camp_options(remaining,MaxWorldRestOptions);cursor=n?remaining[0]:0;}else cursor=0;
  }else{motion.cancel(motion.mask,map_clock_ms);message("此选项当前不可用");}sync_camp_presentation();return true;
 }
 return false;
}
static void camp_actors(Canvas&c,bool dark){
 const auto&a=c.a;if(a.game.world.act_index>=3)return;
 for(const auto&actor:WorldRestActors){
  if(actor.character!=unsigned(a.game.character))continue;
  char model[64]{};TextBuffer key{model,sizeof model,0,&c.a.content};key.text("rest_actor.");key.text(actor.key);key.text(".ACT");key.number(a.game.world.act_index+1);key.text(dark?".DARK":".LIT");
  int ids[3]{-1,-1,-1};bool complete=actor.planes>0&&actor.planes<=3;
  for(unsigned plane=0;complete&&plane<actor.planes;++plane){char action[3]{'P',char('0'+plane),0};ids[plane]=a.content.world_clip(model,action);complete=ids[plane]>=0&&a.content.clip(ids[plane]).count;}
  if(!complete)continue;
  // All planes share the original room camera, period and phase. Preserve
  // Defect's additive glow between his body planes. Regent's authored scene
  // overrides its skeleton's multiply slot with the normal MIX material.
  for(unsigned plane=0;plane<actor.planes;++plane){const auto clip=a.content.clip(ids[plane]);
   const unsigned frame=world_rest_actor_frame(uint64_t(a.animation)*33,clip.count,actor.duration_ms);
   c.picture(int(clip.first)+int(frame),WorldRoomOriginX,WorldRoomOriginY,WorldRoomWidth,WorldRoomHeight,255,actor.blend[plane]);
  }
 }
}
static void camp_scene(Canvas&c,bool dark){
 auto&a=c.a;char region[24]{},key[32]{};world_region_image_key(a.game,region,sizeof region);TextBuffer b{key,sizeof key,0,&c.a.content};b.text(region);
 if(dark)b.text("_DARK");
 int image=a.content.world_image("rest_background",key);if(image>=0){
  c.picture(image,WorldRoomOriginX,WorldRoomOriginY,WorldRoomWidth,WorldRoomHeight);camp_actors(c,dark);
  if(!dark){
   // Same room camera as the baked per-act background. The hive's fire is
   // slightly to the left; all other authored fire pivots project to161,179.
   const bool hive=a.game.world.act_index==1;const int x=hive?156:161,y=hive?174:179;
   const unsigned flicker=24+unsigned(room_curve::sine_turn((a.map_clock_ms%850)*4096/850)+4096)*20/8192;
   room_vfx_stamp(c,I7_Glow,x-23,y-26,46,38,flicker,0xff9a44);
   room_vfx_cinders(c,x,y-11,8,a.map_clock_ms,0x52455354u);
  }
  return;
 }
 // Preserve the old private fixtures' complete presentation. The native act
 // backgrounds do not receive misregistered first-act figures or fire layers;
 // original per-act rest actors use their separate registered resources above.
 c.picture(A_CampBackground,0,22,320,218);c.clip(character_camp_animation(a.game.character),188,61,a.animation*33,false);
 if(a.game.character==Character::Necrobinder)c.clip(AnimOstyCamp,247,100,a.animation*33,false);
 if(!dark){c.picture(a.content.ui_image(U_CampFlames)+int((a.animation*33/125)%160),130,76,46,58);c.picture(A_CampFire,128,111,55,34);}
}
static void camp_icon(Canvas&c,int image,int x,int y,int w,int h,bool enabled,unsigned value){
 small_material_icon(c,image,x,y,w,h,enabled?1000:0,enabled?value:600);
}
static void native_camp_option_picture(Canvas&choices,unsigned slot,int x,bool enabled,const RoomUi7Presentation::Rest::Button::Sample&sample){
 auto&a=choices.a;const auto&s=a.game;const char*key=world_rest_option_key(s,slot);
 // Apply KeepAspectCentered in source units, including packed padding, and
 // round only after BOTH transforms. Rounding47.238 to47 before the.9
 // outline scale produced42px instead of43; focused art also lost a pixel
 // on both axes. The common source pivot125px projects to24px, not23.
 constexpr int holder_w=47,center_x=24;
 const auto extent=[](unsigned source_q8,unsigned scale){return int((source_q8*scale+524288u)/1048576u);};
 const int w=extent(11301,unsigned(sample.scale)),h=extent(8052,unsigned(sample.scale));
   const unsigned outline_scale=room_curve::multiply(unsigned(sample.scale),unsigned(sample.outline));
   // Icon:256x169 inside250x153; outline:256x173 inside248x173.
   // Q8 sizes include the80x57 and86x61 encoded transparent canvases.
   const int ow=extent(12093,outline_scale),oh=extent(8578,outline_scale);
   Canvas outline=choices;outline.tint_red=255;outline.tint_green=179;outline.tint_blue=0;outline.opacity=uint8_t(div255(outline.opacity*192));
   outline.picture(a.content.ui_image(U_RestOutline),x+center_x-ow/2,82-oh/2,ow,oh,255,presentation::Add);
   camp_icon(choices,a.content.world_image("rest",key),x+center_x-w/2,82-h/2,w,h,enabled,unsigned(sample.value));
   char title[96]{};TextBuffer b{title,sizeof title,0,&a.content};b.text("OPTION_");b.text(key);b.text(".name");const char*name=a.content.world_string("rest_site_ui",title);
   Canvas label=choices;label.font_face=a.content.original_label_fonts()?5:0;
   const int label_y=97+(sample.label*4+21*2048)/(21*4096);
   Canvas label_rim=label;label_rim.opacity=uint8_t(div255(label_rim.opacity*128));
   label_rim.centered(*name?name:key,x-4,label_y+1,holder_w+10,10,0);
   label.centered(*name?name:key,x-5,label_y,holder_w+10,10,rgb(212,182,91));
}
static void native_camp_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;a.sync_camp_presentation();auto&motion=a.room_presentation.rest;
 uint8_t options[MaxWorldRestOptions];unsigned count=0;for(unsigned i=0;i<s.world.rest_count;++i)if(motion.mask&(uint32_t(1)<<i))options[count++]=uint8_t(i);
 unsigned selected=camp_selected(a,options,count);if(motion.choices_locked())for(unsigned i=0;i<count;++i)if(options[i]==motion.selection.slot)selected=i;
 const unsigned first=selected/4*4,shown=mn(4,int(count-first));if(count&&!motion.choices_locked())a.cursor=options[selected];camp_scene(c,a.camp_options(nullptr,0)==0);
 if(motion.healing){const uint32_t age=a.map_clock_ms-motion.heal_start;
  if(age<4000){room_vfx_rest_tone(c,age);room_vfx_smoke(c,160,202,320,age,12,0x4845414cu);}
  else motion.healing=false;
 }
 if(a.content.has_ftue_assets()&&!a.seen_ftue(FtueId::RestSite)&&a.map_clock_ms-a.ftue.room_since<500)return;
 Canvas screen=c;screen.opacity=uint8_t(div255(screen.opacity*motion.screen_alpha(a.map_clock_ms)));
 if(count&&screen.opacity){
  // ChoicesScreen is a full UI Control: its centered offsets use4/21,
  // independently of the background camera. Source36/28px labels project
  // to7/5px; the former12/10px labels displaced the whole room hierarchy.
  Canvas heading=screen;heading.font_face=a.content.original_label_fonts()?5:0;heading.sparse_scene_sizes=true;
  const char*prompt=a.content.world_string("rest_site_ui","PROMPT");const char*header=*prompt?prompt:"休息片刻……";
  const int header_size=a.content.font_size(10,heading.font_face,false,true),header_y=46+mx(0,(15-header_size-2)/2);
  Canvas rim=heading;rim.opacity=uint8_t(div255(rim.opacity*64));
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)rim.centered(header,65+dx,header_y+dy,190,header_size,0);
  heading.centered(header,65,header_y,190,header_size,rgb(255,246,226));
  constexpr int icon_w=47,step=66,choice_y=66,choice_h=31;
  const int start=(320-int(shown)*icon_w-int(shown-1)*19)/2;
  const bool available=native_room_input_available(a,Phase::Camp)&&!motion.choices_locked();int focused=-1;
  Canvas choices=screen;choices.opacity=uint8_t(div255(choices.opacity*motion.alpha(a.map_clock_ms)));
  choices.brightness=uint8_t(div255(choices.brightness*motion.alpha(a.map_clock_ms)));
  for(unsigned i=0;i<shown;++i){const unsigned slot=options[first+i];const bool enabled=motion.choices_locked()?bool(motion.selection.enabled&(uint32_t(1)<<slot)):world_rest_option_enabled(s,slot);const int x=start+int(i)*step;
   const bool hover=available&&(a.pointer.visible?motion.mouse_ready(a.map_clock_ms)&&a.pointer.x>=x&&a.pointer.x<x+icon_w&&a.pointer.y>=choice_y&&a.pointer.y<choice_y+choice_h:a.cursor==int(slot));
   const bool pressed=hover&&enabled&&a.pointer.held&&a.pointer.captured.kind==HitKind::Choice&&a.pointer.captured.value==int(slot)&&a.pointer.context==a.input_context();
   const auto sample=motion.buttons[slot].observe(pressed?2:hover?1:0,a.map_clock_ms);if(hover)focused=int(slot);
   native_camp_option_picture(choices,slot,x,enabled,sample);
   if(!motion.choices_locked())c.hit(x,choice_y,icon_w,choice_h,HitKind::Choice,int(slot),enabled&&motion.mouse_ready(a.map_clock_ms));
  }
  if(!motion.choices_locked())motion.focus(focused,a.map_clock_ms);const unsigned description_alpha=motion.description_alpha(a.map_clock_ms);
  if(description_alpha){const unsigned slot=unsigned(motion.description);const auto option=WorldRestOption(s.world.rest_options[slot]);const bool enabled=motion.choices_locked()||world_rest_option_enabled(s,slot);
  char key[128]{},description[1024]{};TextBuffer b{key,sizeof key,0,&c.a.content};b.text("OPTION_");b.text(world_rest_option_key(s,slot));
  b.text(!enabled&&(option==WorldRestOption::Smith||option==WorldRestOption::Cook)?".descriptionDisabled":".description");a.localized_text("rest_site_ui",key,description,sizeof description,int(option));
  // Rich spans select their own colors. Tint the whole shadow pass so gold
  // words do not get a second displaced gold copy beneath the real label.
  // The source RichTextLabel is horizontally centered but top-aligned. The
  // shared card fitter centers both axes, so give it only the actual text
  // height rather than moving this description down inside the 393px box.
  Canvas description_layer=screen;description_layer.bold_face=5;description_layer.opacity=uint8_t(div255(description_layer.opacity*description_alpha));
  const int body_size=a.content.font_size(10);
  auto metrics=[&](const RichChar&ch,int size){return description_layer.text_metric(ch,size);};
  const auto fit=fitted_text::plan(description,236,75,body_size,body_size,metrics);const int text_height=mn(75,fit.rows*fit.step+2);
  Canvas shadow=description_layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
  shadow.opacity=uint8_t(div255(shadow.opacity*128));
  shadow.paragraph_fitted(description,43,117,236,text_height,fit.size,White,fit.size,true);
  description_layer.paragraph_fitted(description,42,116,236,text_height,fit.size,rgb(255,246,226),fit.size,true);
  }
 }
 if(count>4&&!motion.choices_locked()){
  c.text("‹",12,212,16,Gold);c.action(2,204,36,34,UiAction::PreviousCampPage,first>0);c.text("›",117,212,16,Gold);c.action(105,204,36,34,UiAction::NextCampPage,first+shown<count);
  char page[32]{};TextBuffer b{page,sizeof page,0,&c.a.content};b.number(first/4+1);b.text(" / ");b.number((count+3)/4);c.centered(page,40,216,63,10,Muted);
 }
 const bool can_proceed=world_rest_can_proceed(s)&&a.camp_proceed_ready();
 const char*text=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
 native_proceed(c,*text?text:"前进",can_proceed);native_proceed_action(c,UiAction::LeaveCamp,can_proceed);
}
