enum FrontMenu {FrontStart,FrontContinue,FrontAbandon,FrontCompendium,FrontSettings,FrontQuit};
static FrontMenu front_item(const App&a,int row){
 static const FrontMenu fresh[]={FrontStart,FrontCompendium,FrontSettings,FrontQuit};
 static const FrontMenu saved[]={FrontContinue,FrontAbandon,FrontCompendium,FrontSettings,FrontQuit};
 return row>=0&&row<a.title_menu_count()?(a.has_save?saved[row]:fresh[row]):FrontQuit;
}
static bool resumable_run(const State&s){return s.phase!=Phase::Dead&&s.phase!=Phase::Complete&&s.phase!=Phase::Error;}
static State frontend_saved_probe;
bool App::refresh_save(){
 // The probe must not overwrite the visible run or any RNG stream. Static
 // workspace avoids putting a complete State on the calculator's stack.
 auto&probe=frontend_saved_probe;
 has_save=callbacks.load&&callbacks.load(probe)&&validate(probe)&&resumable_run(probe);
 if(has_save){saved_character=probe.character;saved_seed=probe.seed;saved_floor=probe.world.active?probe.world.total_floor:probe.floor;
  saved_hp=probe.hp;saved_max_hp=probe.max_hp;saved_ascension=probe.world.ascension;saved_world=probe.world.active;saved_practice=probe.practice;}
 return has_save;
}
bool App::load_saved_run(){
 auto&probe=frontend_saved_probe;
 if(!callbacks.load||!callbacks.load(probe)||!validate(probe)||!resumable_run(probe))return false;
 game=probe;reset_presentation();return true;
}
bool App::save_current_run(){
 if(!callbacks.save||!callbacks.save(game))return false;
 has_save=resumable_run(game);if(has_save){saved_character=game.character;saved_seed=game.seed;saved_floor=game.world.active?game.world.total_floor:game.floor;
  saved_hp=game.hp;saved_max_hp=game.max_hp;saved_ascension=game.world.ascension;saved_world=game.world.active;saved_practice=game.practice;}
 return true;
}
bool App::finish_saved_run(){
 refresh_save();
 // Practice and a separate saved expedition can coexist. Never erase the
 // latter merely because a different run reached its ending screen.
 if(!has_save||saved_seed!=game.seed||saved_character!=game.character||saved_world!=bool(game.world.active)||saved_practice!=bool(game.practice))return true;
 if(callbacks.clear_save){if(!callbacks.clear_save())return false;}
 else if(resumable_run(game)||!callbacks.save||!callbacks.save(game))return false;
 has_save=false;return true;
}
void App::return_to_title(){
 title=true;paused=library=character_select=help=settings_open=false;frontend_confirm=frontend_choice=0;menu=0;
 map_preview.open=false;reset_presentation();refresh_save();
}
void App::accept_frontend_confirm(){
 const unsigned action=frontend_confirm;if(!action)return;
 frontend_confirm=frontend_choice=0;pointer_cancel();
 if(action==3){quit=true;return;}
 if(action==1){
  if(!title||!has_save)return;
  if(!callbacks.clear_save||!callbacks.clear_save()){message("无法删除存档，请重试");return;}
  refresh_save();menu=0;return;
 }
 if(action==2&&paused&&resumable_run(game)){
  // RunManager.Abandon guarantees a forced player death; death prevention
  // must not turn the requested abandon into a surviving run.
  if(!finish_saved_run()){message("无法删除存档，请重试");return;}
  player_kill(game,true);resolve_pending(game);paused=false;reset_presentation();
 }
}
bool App::frontend_input(Key key){
 if(frontend_confirm){
  if(key==Key::Back){frontend_confirm=frontend_choice=0;pointer_cancel();}
  else if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down)frontend_choice^=1;
  else if(key==Key::Confirm){if(frontend_choice)accept_frontend_confirm();else{frontend_confirm=0;pointer_cancel();}}
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
   case FrontAbandon:frontend_confirm=1;frontend_choice=0;break;
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
