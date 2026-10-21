// The room's gold and relic are owned by the run controller. These helpers
// only select original visual states and route real player actions to it.
static bool native_room_input_available(const App&a,Phase phase){
 return a.game.world.active&&a.game.world.room_open&&a.game.phase==phase&&
  a.game.world.room_type==unsigned(phase==Phase::Camp?WorldRoom::Rest:WorldRoom::Treasure)&&!a.title&&!a.paused&&!a.help&&!a.panel&&!a.library&&!a.character_select&&
  !a.map_preview.open&&!a.event_reader&&!a.frontend_confirm&&!a.settings_open&&a.detail_card<0&&a.detail_item<0&&a.status_actor<0&&a.deck_candidate<0;
}
static const char* treasure_text(const App&a,const char*key,const char*fallback){const char*t=a.content.world_string("gameplay_ui",key);return *t?t:fallback;}
static void treasure_key(const State&s,const char*action,char*out,size_t size){TextBuffer key{out,size,0};key.text("ACT");key.number(s.world.act_index+1);if(action){key.ch('.');key.text(action);}}
// App advances at 33ms. Clamp before multiplication so a long-open room or
// an unsigned tick wrap cannot restart a completed visual transition.
static uint32_t treasure_elapsed(uint32_t ticks,uint32_t duration){return ticks>=(duration+32)/33?duration:ticks*33;}
static uint16_t treasure_linear(uint16_t from,uint16_t to,uint32_t ticks,uint32_t duration){
 const uint32_t elapsed=treasure_elapsed(ticks,duration);
 return uint16_t((uint32_t(from)*(duration-elapsed)+uint32_t(to)*elapsed+duration/2)/duration);
}
static unsigned treasure_alpha8(uint16_t alpha){return (unsigned(alpha)+128)/257;}
static uint16_t treasure_expo_out(uint32_t ticks){
 // Godot EXPO_OUT: 1.001 * (1 - 2^(-10*t/d)), with an exact endpoint.
 // Samples of the original 400ms tween at this renderer's 33ms clock, in
 // 0..65535 alpha. Constant data needs neither ARM libm nor initialization.
 static const uint16_t values[]={0,28570,44698,53801,58940,61841,63478,64403,64924,65219,65385,65479,65532};
 return ticks<sizeof values/sizeof values[0]?values[ticks]:65535;
}
static uint16_t treasure_chest_alpha(const App&a,uint32_t now){
 if(a.treasure_presentation>=3)return treasure_linear(a.treasure_chest_from,65535,now-a.treasure_close_tick,300);
 if(a.treasure_presentation==2){
  const uint32_t elapsed=treasure_elapsed(now-a.treasure_open_tick,400);
  // The target is exactly 0.5, before quantizing to the RGB565 canvas alpha.
  return uint16_t((65535u*(800-elapsed)+400)/800);
 }
 if(a.treasure_presentation==1)return 65535;
 return a.game.treasure_state==1&&a.game.world.treasure_relic!=NoWorldRelic?32768:65535;
}
static uint16_t treasure_banner_alpha(const App&a,uint32_t now){
 if(a.treasure_presentation>=3)return uint16_t((uint32_t(a.treasure_banner_from)*(65535-treasure_expo_out(now-a.treasure_close_tick))+32767)/65535);
 if(a.treasure_presentation)return treasure_expo_out(now-a.treasure_banner_tick);
 return a.game.treasure_state==1&&a.game.world.treasure_relic!=NoWorldRelic?65535:0;
}
static int treasure_banner_position(const App&a,uint32_t now){
 if(a.treasure_presentation>=3)return a.treasure_banner_y;
 if(!a.treasure_presentation)return 0;
 // NCommonBanner's 50px hide offset, projected from 1080px to the 218px
 // room, with BACK_OUT (s=1.70158). AnimateOut kills this position tween.
 static const int8_t offsets[]={10,7,4,2,0,0,-1,-1,-1,-1,0,0,0};
 const uint32_t ticks=now-a.treasure_banner_tick;return ticks<sizeof offsets?offsets[ticks]:0;
}
static void treasure_remember_room(App&a){a.treasure_room_seed=world_run_seed(a.game);a.treasure_room_act=a.game.world.act_index;a.treasure_room_point=a.game.world.current_point;}
static void treasure_begin_close(App&a,uint32_t when,uint16_t chest,uint16_t banner,int y){
 const bool opening=a.treasure_presentation==2;
 a.treasure_chest_from=chest;a.treasure_banner_from=banner;a.treasure_banner_y=int8_t(y);a.treasure_close_tick=when;
 a.treasure_presentation=opening?3:4;treasure_remember_room(a);
 // Keep open_tick: AnimIn restores opacity without restarting Spine.
}
void App::reset_treasure_presentation(){
 treasure_presentation=0;treasure_open_tick=treasure_banner_tick=treasure_close_tick=0;
 treasure_chest_from=65535;treasure_banner_from=0;treasure_banner_y=0;
 treasure_room_seed=0;treasure_room_act=255;treasure_room_point=0xffff;
}
unsigned App::treasure_chest_opacity()const{return treasure_alpha8(treasure_chest_alpha(*this,animation));}
unsigned App::treasure_banner_opacity()const{return treasure_alpha8(treasure_banner_alpha(*this,animation));}
int App::treasure_banner_offset()const{return treasure_banner_position(*this,animation);}
unsigned App::treasure_collection_opacity()const{
 // AnimOut explicitly sets the collection to white. In single player the
 // awarded holder is already invisible, so its artwork must not fade out.
 if(treasure_presentation>=3)return treasure_alpha8(treasure_linear(65535,0,animation-treasure_close_tick,300));
 if(treasure_presentation==2)return treasure_alpha8(treasure_linear(0,65535,animation-treasure_open_tick,400));
 if(treasure_presentation==1)return 0;
 return game.treasure_state==1&&game.world.treasure_relic!=NoWorldRelic?255:0;
}
void App::sync_treasure_presentation(){
 if(treasure_presentation&&(!game.world.active||!game.world.room_open||game.world.room_type!=unsigned(WorldRoom::Treasure)||
    treasure_room_seed!=world_run_seed(game)||treasure_room_act!=game.world.act_index||treasure_room_point!=game.world.current_point)){
  reset_treasure_presentation();
 }
 // Extra room rewards may temporarily open a selection before the chest's
 // own relic collection. Start its animation only after that selection ends.
 if(treasure_presentation==1&&game.world.active&&game.phase==Phase::Treasure&&game.treasure_state){treasure_presentation=2;treasure_open_tick=animation;}
 if(treasure_presentation==2){
  uint32_t close=animation;bool completed=game.treasure_state==2;
  // The empty collection completes after 1s. This only closes its visuals;
  // the controller still owns the empty chest and its available Proceed.
  if(!completed&&game.world.treasure_relic==NoWorldRelic&&uint32_t(animation-treasure_open_tick)>=31){completed=true;close=treasure_open_tick+31;}
  if(completed)treasure_begin_close(*this,close,treasure_chest_alpha(*this,close),treasure_banner_alpha(*this,close),treasure_banner_position(*this,close));
 }
}
bool App::treasure_can_proceed()const{
 if(!native_room_input_available(*this,Phase::Treasure))return false;
 if(game.treasure_state!=1||game.world.treasure_relic==NoWorldRelic||!treasure_presentation)return true;
 // NTreasureRoom enables Skip 2.5 seconds after opening the relic selection.
 // This delay never enters the save or prevents access to its pending choice.
 return treasure_presentation==2&&uint32_t(animation-treasure_open_tick)>=76;
}
void App::treasure_action(UiAction action){
 if(!native_room_input_available(*this,Phase::Treasure))return;sync_treasure_presentation();
 if(action==UiAction::TreasureOpen){
  if(!game.treasure_state&&open_treasure(game)){reset_treasure_presentation();treasure_presentation=1;treasure_banner_tick=animation;treasure_remember_room(*this);sync_treasure_presentation();cursor=game.world.treasure_relic==NoWorldRelic?1:0;choice_selected_count=0;choice_revision=game.revision;pointer_cancel();}
 }else if(action==UiAction::TreasureTake){
  // Capture before acquisition changes treasure_state, including a restored
  // offer. Original RelicPickingFinished need not await its acquired relic's
  // subsequent deck choice (for example Dolly's Mirror).
  const uint16_t chest=treasure_chest_alpha(*this,animation),banner=treasure_banner_alpha(*this,animation);const int y=treasure_banner_position(*this,animation);
  if(game.treasure_state==1&&game.world.treasure_relic!=NoWorldRelic&&take_treasure(game)){treasure_begin_close(*this,animation,chest,banner,y);cursor=1;choice_selected_count=0;choice_revision=game.revision;pointer_cancel();}
 }else if(action==UiAction::TreasureLeave&&treasure_can_proceed()&&leave_treasure(game))reset_presentation();
}
bool App::treasure_input(Key key){
 if(!native_room_input_available(*this,Phase::Treasure))return false;sync_treasure_presentation();
 const bool take=game.treasure_state==1&&game.world.treasure_relic!=NoWorldRelic;
 if(game.treasure_state&&!take)cursor=1;
 if(key==Key::Left||key==Key::Up){cursor=!game.treasure_state||take?0:1;return true;}
 if(key==Key::Right||key==Key::Down){cursor=1;return true;}
 if(key==Key::Confirm){treasure_action(cursor==1?UiAction::TreasureLeave:!game.treasure_state?UiAction::TreasureOpen:UiAction::TreasureTake);return true;}
 if(key==Key::EndTurn){treasure_action(UiAction::TreasureLeave);return true;}
 if(key==Key::Detail){if(game.treasure_state&&game.world.treasure_relic!=NoWorldRelic){detail_item=game.world.treasure_relic;detail_owned=-1;detail_scroll=0;pointer_cancel();}return true;}
 return false;
}
static bool treasure_clip(Canvas&c,const char*action,uint32_t elapsed,uint32_t duration,unsigned opacity){
 char model[24]{};TextBuffer name{model,sizeof model,0};name.text("treasure.ACT");name.number(c.a.game.world.act_index+1);const int id=c.a.content.world_clip(model,action);if(id<0)return false;
 const auto clip=c.a.content.clip(id);if(!clip.count||!duration)return false;
 // The directory stores integer frame_ms; source-duration normalization keeps
 // a 23-frame opening from cutting its 1133ms source cycle short at 1127ms.
 const unsigned frame=mn(int(clip.count)-1,int(uint64_t(elapsed)*clip.count/duration));
 c.picture(int(clip.first)+int(frame),WorldRoomOriginX,WorldRoomOriginY,WorldRoomWidth,WorldRoomHeight,opacity);return true;
}
static void treasure_scene(Canvas&c){
 auto&a=c.a;const auto&s=a.game;char key[48]{};
 const unsigned opacity=a.treasure_chest_opacity();
 c.rect(0,22,320,218,rgb(0,0,0));
 if(!s.treasure_state){
  treasure_key(s,a.cursor==0?"CLOSED_OUTLINE":"CLOSED",key,sizeof key);int image=a.content.world_image("treasure",key);
  if(image>=0){c.picture(image,WorldRoomOriginX,WorldRoomOriginY,WorldRoomWidth,WorldRoomHeight);return;}
 }else{
  const uint32_t elapsed=treasure_elapsed(a.animation-a.treasure_open_tick,WorldTreasureOpeningDurationMs+WorldTreasureShineDurationMs);
  if(a.treasure_presentation==2||a.treasure_presentation==3){
   if(elapsed<WorldTreasureOpeningDurationMs&&treasure_clip(c,"OPENING",elapsed,WorldTreasureOpeningDurationMs,opacity))return;
   if(elapsed>=WorldTreasureOpeningDurationMs&&elapsed<WorldTreasureOpeningDurationMs+WorldTreasureShineDurationMs&&
      treasure_clip(c,"SHINE",elapsed-WorldTreasureOpeningDurationMs,WorldTreasureShineDurationMs,opacity))return;
  }
  treasure_key(s,"OPEN",key,sizeof key);int image=a.content.world_image("treasure",key);
  if(image>=0){c.picture(image,WorldRoomOriginX,WorldRoomOriginY,WorldRoomWidth,WorldRoomHeight,opacity);return;}
 }
 // Historical fixtures lack the appended source scene assets. Keep their
 // existing composition without manufacturing an opening animation.
 c.picture(A_ChestBackground,0,22,320,218,opacity);c.picture(A_Chest,128,91,0,0,opacity);
}
static void native_treasure_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;a.sync_treasure_presentation();
 const bool offering=s.treasure_state==1&&s.world.treasure_relic!=NoWorldRelic;
 if(s.treasure_state&&!offering)a.cursor=1;treasure_scene(c);
 const unsigned banner_alpha=a.treasure_banner_opacity();
 if(banner_alpha){Canvas banner=c;banner.opacity=uint8_t(div255(c.opacity*banner_alpha));const int y=a.treasure_banner_offset();banner.ui(U_RewardBanner,34,29+y,252,48);banner.centered(treasure_text(a,"TREASURE_BANNER","宝藏"),47,40+y,226,20,Gold);}
 if(!s.treasure_state){
  // Projection of the original centered 800x500 Chest control at 1920x1080.
  c.action(87,96,162,101,UiAction::TreasureOpen);
  c.text("打开 Enter",13,216,10,White);
 }else if(offering){
  Canvas collection=c;collection.opacity=uint8_t(div255(c.opacity*a.treasure_collection_opacity()));
  collection.picture(a.relic_face_image(s.world.treasure_relic),138,88,44,44);c.action(119,77,82,77,UiAction::TreasureTake);
  collection.centered(a.relic_title(s.world.treasure_relic),13,143,294,16,Gold);
  char description[2048]{};a.relic_description(s.world.treasure_relic,description,sizeof description);collection.paragraph(description,31,165,258,2,0,12,White);
  // The readable calculator controls remain usable while the collection
  // fades. Chaotic holder delay/motion is a separate, unimplemented effect.
  c.text("领取 Enter",8,216,10,White);c.button("详情 I",112,211,91,false,24);c.hit(109,207,96,31,HitKind::Command,int(Key::Detail));
 }else{
  if(s.treasure_state==1&&a.treasure_collection_opacity()){
   Canvas empty=c;const unsigned self_alpha=treasure_alpha8(treasure_linear(0,65535,a.animation-a.treasure_open_tick,250));
   empty.opacity=uint8_t(div255(div255(c.opacity*a.treasure_collection_opacity())*self_alpha));empty.centered(treasure_text(a,"TREASURE_EMPTY","空的……"),21,65,278,16,White);
  }
  owned_relic_strip(c);
 }
 const bool enabled=a.treasure_can_proceed();native_proceed(c,212,210,102,28,treasure_text(a,offering?"SKIP_REWARDS_BUTTON":"PROCEED_BUTTON",offering?"跳过":"前进"),enabled,a.cursor==1);
 c.action(208,205,112,33,UiAction::TreasureLeave,enabled);
}
