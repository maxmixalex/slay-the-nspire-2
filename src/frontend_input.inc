enum FrontMenu {FrontStart,FrontContinue,FrontAbandon,FrontCompendium,FrontSettings,FrontQuit};
static FrontMenu front_item(const App&a,int row){
 static const FrontMenu fresh[]={FrontStart,FrontCompendium,FrontSettings,FrontQuit};
 static const FrontMenu saved[]={FrontContinue,FrontAbandon,FrontCompendium,FrontSettings,FrontQuit};
 return row>=0&&row<a.title_menu_count()?(a.has_save?saved[row]:fresh[row]):FrontQuit;
}
static bool resumable_run(const State&s){return s.phase!=Phase::Dead&&s.phase!=Phase::Complete&&s.phase!=Phase::Error;}
// Aggregate initialization keeps this freestanding storage in static data;
// a global default-constructor entry is unsupported by the Ndless loader.
static State frontend_saved_probe{};
static SaveData frontend_saved_bytes{};
static bool same_checkpoint(SaveCheckpoint a,SaveCheckpoint b){return a.size&&a.size==b.size&&a.hash==b.hash;}
static bool checkpoint_of(const State&s,SaveCheckpoint&out){
 // Canonical encoding excludes C++ padding and includes every saved field.
 // This detects ordinary replacement, including progress within one seed;
 // it is not a collision-proof identity or an atomic storage transaction.
 if(!save(s,frontend_saved_bytes))return false;
 uint64_t hash=14695981039346656037ULL;
 for(size_t i=0;i<frontend_saved_bytes.size;i++){hash^=frontend_saved_bytes.bytes[i];hash*=1099511628211ULL;}
 out={hash,uint32_t(frontend_saved_bytes.size)};return true;
}
static void remember_saved(App&a,const State&s,SaveCheckpoint checkpoint){
 a.has_save=resumable_run(s);a.saved_checkpoint=a.has_save?checkpoint:SaveCheckpoint{};
 a.saved_character=s.character;a.saved_seed=s.seed;a.saved_world=s.world.active;a.saved_practice=s.practice;
 a.saved_floor=s.world.active?s.world.total_floor:s.floor;a.saved_hp=s.hp;a.saved_max_hp=s.max_hp;
 a.saved_run_seed=s.world.active?s.world.run_seed:0;a.saved_ascension=s.world.active?s.world.ascension:0;
 a.saved_region=s.world.active?s.world.rooms[0].region:0;
}
static void forget_saved(App&a){
 a.has_save=false;a.saved_checkpoint={};a.saved_character=Character::Ironclad;a.saved_seed=0;
 a.saved_floor=a.saved_hp=a.saved_max_hp=0;a.saved_run_seed=0;a.saved_ascension=a.saved_region=0;
 a.saved_world=a.saved_practice=false;
}
static bool read_saved(App&a,SaveCheckpoint&checkpoint){
 // Failed probes cannot touch the visible simulation. Preserve the last
 // known checkpoint as evidence that a later failed read is not absence.
 if(!a.callbacks.load||!a.callbacks.load(frontend_saved_probe)||!checkpoint_of(frontend_saved_probe,checkpoint)){
  a.has_save=false;return false;
 }
 remember_saved(a,frontend_saved_probe,checkpoint);return true;
}
static bool same_run_identity(const State&a,const State&b){
 if(a.seed!=b.seed||a.character!=b.character||bool(a.world.active)!=bool(b.world.active)||bool(a.practice)!=bool(b.practice))return false;
 return !a.world.active||(a.world.run_seed==b.world.run_seed&&a.world.ascension==b.world.ascension&&a.world.rooms[0].region==b.world.rooms[0].region);
}
static bool terminal_probe(State&s){
 if(resumable_run(s)){player_kill(s,true);resolve_pending(s);}
 return validate(s)&&(s.phase==Phase::Dead||s.phase==Phase::Complete);
}
void App::forget_run_checkpoint(){run_checkpoint={};confirmed_checkpoint={};}
bool App::refresh_save(){SaveCheckpoint checkpoint{};return read_saved(*this,checkpoint)&&has_save;}
bool App::load_saved_run(){
 SaveCheckpoint checkpoint{};
 if(!read_saved(*this,checkpoint)||!has_save)return false;
 game=frontend_saved_probe;run_checkpoint=checkpoint;reset_presentation();return true;
}
bool App::save_current_run(){
 SaveCheckpoint checkpoint{};
 if(!callbacks.save||!checkpoint_of(game,checkpoint)||!callbacks.save(game))return false;
 remember_saved(*this,game,checkpoint);run_checkpoint=has_save?checkpoint:SaveCheckpoint{};return true;
}
bool App::finish_saved_run(){
 const bool expected=saved_checkpoint.size||run_checkpoint.size;SaveCheckpoint checkpoint{};
 if(!read_saved(*this,checkpoint))return !expected;
 // Different expeditions can coexist with the visible run. Matching input
 // parameters alone never authorize deletion of an unassociated checkpoint.
 if(!has_save||!same_run_identity(game,frontend_saved_probe)){forget_run_checkpoint();return true;}
 if(!same_checkpoint(run_checkpoint,checkpoint))return false;
 if(callbacks.clear_save){if(!callbacks.clear_save())return false;}
 else{
  // Older hosts can persist a terminal checkpoint instead of removing it.
  // Execute forced death on the probe, and adopt it only after a real save.
  frontend_saved_probe=game;
  if(!callbacks.save||!terminal_probe(frontend_saved_probe)||!callbacks.save(frontend_saved_probe))return false;
  game=frontend_saved_probe;
 }
 forget_saved(*this);forget_run_checkpoint();return true;
}
void App::return_to_title(){
 title=true;paused=library=character_select=help=settings_open=false;frontend_confirm=frontend_choice=0;confirmed_checkpoint={};menu=0;
 map_preview.open=false;reset_presentation();refresh_save();
}
void App::accept_frontend_confirm(){
 const unsigned action=frontend_confirm;if(!action)return;
 const SaveCheckpoint confirmed=confirmed_checkpoint;
 frontend_confirm=frontend_choice=0;confirmed_checkpoint={};pointer_cancel();
 if(action==3){quit=true;return;}
 if(action==1){
  if(!title||!confirmed.size)return;
  SaveCheckpoint checkpoint{};
  if(!read_saved(*this,checkpoint)){message("无法读取存档，请重试");return;}
  if(!has_save||!same_checkpoint(confirmed,checkpoint)){menu=0;message("存档已变更，请重新确认");return;}
  if(callbacks.clear_save){if(!callbacks.clear_save()){message("无法删除存档，请重试");return;}}
  else if(!callbacks.save||!terminal_probe(frontend_saved_probe)||!callbacks.save(frontend_saved_probe)){message("无法保存终局，请重试");return;}
  if(same_checkpoint(run_checkpoint,checkpoint))forget_run_checkpoint();
  forget_saved(*this);
  refresh_save();menu=0;return;
 }
 if(action==2&&paused&&resumable_run(game)){
  // RunManager.Abandon guarantees a forced player death; death prevention
  // must not turn the requested abandon into a surviving run.
  if(!finish_saved_run()){message("无法删除存档，请重试");return;}
  if(resumable_run(game)){player_kill(game,true);resolve_pending(game);}paused=false;reset_presentation();
 }
}
bool App::frontend_input(Key key){
 if(frontend_confirm){
  if(key==Key::Back){frontend_confirm=frontend_choice=0;confirmed_checkpoint={};pointer_cancel();}
  else if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down)frontend_choice^=1;
  else if(key==Key::Confirm){if(frontend_choice)accept_frontend_confirm();else{frontend_confirm=0;confirmed_checkpoint={};pointer_cancel();}}
  return true;
 }
 if(help){if(key==Key::Back||key==Key::Confirm)help=false;return true;}
 if(settings_open){
  const int count=title?5:3;
  if(key==Key::Back){settings_open=false;pointer_cancel();}
  else if(key==Key::Up||key==Key::Left)settings_item=uint8_t((settings_item+count-1)%count);
  else if(key==Key::Down||key==Key::Right)settings_item=uint8_t((settings_item+1)%count);
  else if(key==Key::Confirm){
   const UiAction actions[]={UiAction::SettingsFastMode,UiAction::SettingsControls,UiAction::SettingsPractice,UiAction::SettingsReference,UiAction::SettingsBack};
   ui_action(actions[!title&&settings_item==2?4:settings_item]);
  }
  return true;
 }
 if(library||character_select||map_preview.open||detail_card>=0||detail_item>=0||status_actor>=0)return false;
 if(title){
  const int count=title_menu_count();if(menu<0||menu>=count)menu=0;
  if(key==Key::Up||key==Key::Left)menu=(menu+count-1)%count;
  else if(key==Key::Down||key==Key::Right)menu=(menu+1)%count;
  else if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;}
  else if(key==Key::Load){
   if(load_saved_run())title=false;else{refresh_save();message("没有有效的存档");}
  }else if(key==Key::Confirm){switch(front_item(*this,menu)){
   case FrontStart:practice_entry=false;character_select=true;break;
   case FrontContinue:if(load_saved_run())title=false;else{refresh_save();menu=0;message("没有有效的存档");}break;
   case FrontAbandon:
    if(refresh_save()){confirmed_checkpoint=saved_checkpoint;frontend_confirm=1;frontend_choice=0;}
    else{menu=0;message("无法读取存档，请重试");}break;
   case FrontCompendium:library=true;library_page=library_selected=0;break;
   case FrontSettings:settings_open=true;settings_item=0;break;
   case FrontQuit:frontend_confirm=3;frontend_choice=0;break;
  }}else if(key==Key::Collection||key==Key::ReferenceMap)return false;
  return true;
 }
 if(!paused)return false;
 if(key==Key::Back){paused=false;return true;}
 if(key==Key::Up||key==Key::Left)menu=(menu+4)%5;
 else if(key==Key::Down||key==Key::Right)menu=(menu+1)%5;
 else if(key==Key::Confirm){switch(menu){
  case 0:paused=false;break;
  case 1:settings_open=true;settings_item=0;break;
  case 2:library=true;library_page=library_selected=0;break;
  case 3:if(resumable_run(game)){frontend_confirm=2;frontend_choice=0;}break;
  case 4:
   if(!resumable_run(game)){if(finish_saved_run())return_to_title();else message("无法删除存档，请重试");}
   else if(save_current_run())return_to_title();else message("保存失败，请重试");break;
 }}else if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;}
 else if(key==Key::Collection)return false;
 return true;
}
