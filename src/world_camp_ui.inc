unsigned App::camp_options(uint8_t*out,unsigned capacity)const{
 unsigned count=0;for(unsigned i=0;i<world_rest_option_count(game);++i)if(world_rest_option_visible(game,i)){if(out&&count<capacity)out[count]=uint8_t(i);++count;}return count;
}
static unsigned camp_selected(const App&a,const uint8_t*options,unsigned count){for(unsigned i=0;i<count;++i)if(options[i]==a.cursor)return i;return 0;}
void App::camp_page(int direction){
 if(!native_room_input_available(*this,Phase::Camp))return;uint8_t options[MaxWorldRestOptions];const unsigned count=camp_options(options,MaxWorldRestOptions);if(!count)return;
 const int page=int(camp_selected(*this,options,count)/4),last=int((count-1)/4);cursor=options[unsigned(mn(mx(0,page+direction),last))*4];pointer_cancel();
}
bool App::camp_input(Key key){
 if(!native_room_input_available(*this,Phase::Camp))return false;
 uint8_t options[MaxWorldRestOptions];const unsigned count=camp_options(options,MaxWorldRestOptions);unsigned selected=camp_selected(*this,options,count);
 if(key==Key::Left||key==Key::Up){if(count)cursor=options[(selected+count-1)%count];return true;}
 if(key==Key::Right||key==Key::Down){if(count)cursor=options[(selected+1)%count];return true;}
 if(key==Key::EndTurn){if(world_rest_proceed(game))proceed_map_window();return true;}
 if(key==Key::Confirm){
  if(!count){if(world_rest_proceed(game))proceed_map_window();return true;}cursor=options[selected];if(world_rest_choose(game,unsigned(cursor))){
   choice_selected_count=0;choice_revision=game.revision;scroll=0;pointer_cancel();
   uint8_t remaining[MaxWorldRestOptions];if(game.phase==Phase::Camp){const unsigned n=camp_options(remaining,MaxWorldRestOptions);cursor=n?remaining[0]:0;}else cursor=0;
  }else message("此选项当前不可用");return true;
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
 int image=a.content.world_image("rest_background",key);if(image>=0){c.picture(image,WorldRoomOriginX,WorldRoomOriginY,WorldRoomWidth,WorldRoomHeight);camp_actors(c,dark);return;}
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
static void native_camp_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;uint8_t options[MaxWorldRestOptions];const unsigned count=a.camp_options(options,MaxWorldRestOptions);
 const unsigned selected=camp_selected(a,options,count),first=selected/4*4,shown=mn(4,int(count-first));if(count)a.cursor=options[selected];camp_scene(c,count==0);
 auto&motion=a.room_presentation.rest;uint32_t mask=0;for(unsigned i=0;i<count;++i)mask|=1u<<options[i];motion.observe(mask,a.map_clock_ms);
 if(count){
  // RestSiteRoom keeps Header, ChoicesContainer and Description on the same
  // centered axis. Preserve that authored hierarchy at the room camera scale.
  const char*prompt=a.content.world_string("rest_site_ui","PROMPT");c.centered(*prompt?prompt:"休息片刻……",59,52,202,12,White);
  const int step=71,start=(320-int(shown)*50-int(shown-1)*21)/2;
  const bool available=native_room_input_available(a,Phase::Camp);int focused=-1;
  Canvas choices=c;choices.opacity=uint8_t(div255(choices.opacity*motion.alpha(a.map_clock_ms)));
  choices.brightness=uint8_t(div255(choices.brightness*motion.alpha(a.map_clock_ms)));
  for(unsigned i=0;i<shown;++i){const unsigned slot=options[first+i];const char*key=world_rest_option_key(s,slot);const bool enabled=world_rest_option_enabled(s,slot);const int x=start+int(i)*step;
   const bool hover=available&&(a.pointer.visible?motion.mouse_ready(a.map_clock_ms)&&a.pointer.x>=x-6&&a.pointer.x<x+56&&a.pointer.y>=71&&a.pointer.y<123:a.cursor==int(slot));
   const bool pressed=hover&&enabled&&a.pointer.held&&a.pointer.captured.kind==HitKind::Choice&&a.pointer.captured.value==int(slot)&&a.pointer.context==a.input_context();
   const auto sample=motion.buttons[slot].observe(pressed?2:hover?1:0,a.map_clock_ms);if(hover)focused=int(slot);
   const int w=(50*sample.scale+2048)/4096,h=(33*sample.scale+2048)/4096;
   const unsigned outline_scale=room_curve::multiply(unsigned(sample.scale),unsigned(sample.outline));
   const int ow=int((50*outline_scale+2048)/4096),oh=int((35*outline_scale+2048)/4096);
   Canvas outline=choices;outline.tint_red=255;outline.tint_green=179;outline.tint_blue=0;outline.opacity=uint8_t(div255(outline.opacity*192));
   outline.picture(a.content.ui_image(U_RestOutline),x+25-ow/2,91-oh/2,ow,oh,255,presentation::Add);
   camp_icon(choices,a.content.world_image("rest",key),x+(50-w)/2,76+(33-h)/2,w,h,enabled,unsigned(sample.value));
   char title[96]{};TextBuffer b{title,sizeof title,0,&c.a.content};b.text("OPTION_");b.text(key);b.text(".name");const char*name=a.content.world_string("rest_site_ui",title);
   choices.centered(*name?name:key,x-7,110+(sample.label*2+9*2048)/(9*4096),64,10,rgb(212,182,91));
   c.hit(x-6,71,62,52,HitKind::Choice,int(slot),enabled&&motion.mouse_ready(a.map_clock_ms));
  }
  motion.focus(focused,a.map_clock_ms);const unsigned description_alpha=motion.description_alpha(a.map_clock_ms);
  if(description_alpha){const unsigned slot=unsigned(motion.description);const auto option=WorldRestOption(s.world.rest_options[slot]);const bool enabled=world_rest_option_enabled(s,slot);
  char key[128]{},description[1024]{};TextBuffer b{key,sizeof key,0,&c.a.content};b.text("OPTION_");b.text(world_rest_option_key(s,slot));
  b.text(!enabled&&(option==WorldRestOption::Smith||option==WorldRestOption::Cook)?".descriptionDisabled":".description");a.localized_text("rest_site_ui",key,description,sizeof description,int(option));
  // Rich spans select their own colors. Tint the whole shadow pass so gold
  // words do not get a second displaced gold copy beneath the real label.
  // The source RichTextLabel is horizontally centered but top-aligned. The
  // shared card fitter centers both axes, so give it only the actual text
  // height rather than moving this description down inside the 393px box.
  auto metrics=[&](const RichChar&ch,int size){const auto g=a.content.glyph_metrics(ch.codepoint,size,rich_font_style(ch));
   return fitted_text::Metric{g.advance,mx(int(g.w),int(g.advance))};};
  const auto fit=fitted_text::plan(description,166,78,10,8,metrics);const int text_height=mn(78,fit.rows*fit.step+2);
  Canvas description_layer=c;description_layer.opacity=uint8_t(div255(description_layer.opacity*description_alpha));
  Canvas shadow=description_layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
  shadow.opacity=uint8_t(div255(shadow.opacity*128));
  shadow.paragraph_fitted(description,78,127,166,text_height,fit.size,White,fit.size,true);
  description_layer.paragraph_fitted(description,77,126,166,text_height,fit.size,rgb(255,246,226),fit.size,true);
  }
 }
 if(count>4){
  c.text("‹",12,212,16,Gold);c.action(2,204,36,34,UiAction::PreviousCampPage,first>0);c.text("›",117,212,16,Gold);c.action(105,204,36,34,UiAction::NextCampPage,first+shown<count);
  char page[32]{};TextBuffer b{page,sizeof page,0,&c.a.content};b.number(first/4+1);b.text(" / ");b.number((count+3)/4);c.centered(page,40,216,63,10,Muted);
 }
 if(world_rest_can_proceed(s)){
  const char*text=a.content.world_string("gameplay_ui","PROCEED_BUTTON");native_proceed(c,211,210,103,28,*text?text:"前进");c.action(207,205,111,33,UiAction::LeaveCamp);
 }
}
