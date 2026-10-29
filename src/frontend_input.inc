enum FrontMenu {FrontStart,FrontContinue,FrontAbandon,FrontCompendium,FrontSettings,FrontQuit};
static FrontMenu front_item(const App&a,int row){
 static const FrontMenu fresh[]={FrontStart,FrontSettings,FrontCompendium,FrontQuit};
 static const FrontMenu saved[]={FrontContinue,FrontAbandon,FrontSettings,FrontCompendium,FrontQuit};
 return row>=0&&row<a.title_menu_count()?(a.has_save?saved[row]:fresh[row]):FrontQuit;
}
static bool resumable_run(const State&s){return s.phase!=Phase::Dead&&s.phase!=Phase::Complete&&s.phase!=Phase::Error;}
// The title already holds a validated, read-only saved run for its Continue
// summary. Its ordinary idle frames may prepare that shop's image bytes;
// the active simulation is still restored only by the real Continue action.
static const State* title_merchant_stock(const App&a){
 if(!a.title||!a.has_save||a.debug_open||a.character_departure.active()||
  a.character_select||a.profiles_open||a.library||a.compendium_open||
  a.statistics_open||a.history_open||a.settings_open||a.help||
  a.frontend_confirm||a.run_mode.screen!=RunMenu::Closed||
  a.auxiliary.screen!=AuxiliaryScreen::None)return nullptr;
 const State*probe=state_workspace::saved_probe(&a);
 return probe&&probe->phase==Phase::Shop?probe:nullptr;
}
static SavedRunIdentity saved_run_identity(const State&s){
 SavedRunIdentity out{};out.seed=s.seed;out.character=s.character;
 out.mode=s.run_options.mode;out.modifiers=s.run_options.modifiers;out.daily_day=s.run_options.daily_day;
 out.world=s.world.active;out.practice=s.practice;
 if(out.world){out.run_seed=s.world.run_seed;out.ascension=s.world.ascension;out.region=s.world.rooms[0].region;}
 return out;
}
static bool same_run_identity(const SavedRunIdentity&a,const SavedRunIdentity&b){
 return a.seed==b.seed&&a.character==b.character&&a.world==b.world&&a.practice==b.practice&&a.mode==b.mode&&a.modifiers==b.modifiers&&a.daily_day==b.daily_day&&
  (!a.world||(a.run_seed==b.run_seed&&a.ascension==b.ascension&&a.region==b.region));
}
static void remember_saved(App&a,const State&s){
 a.has_save=resumable_run(s);
 a.saved_character=s.character;a.saved_seed=s.seed;a.saved_world=s.world.active;a.saved_practice=s.practice;
 a.saved_floor=s.world.active?s.world.total_floor:s.floor;a.saved_hp=s.hp;a.saved_max_hp=s.max_hp;a.saved_gold=s.gold;
 a.saved_run_seed=s.world.active?s.world.run_seed:0;a.saved_ascension=s.world.active?s.world.ascension:0;
 a.saved_region=s.world.active?s.world.rooms[0].region:0;
 a.saved_current_region=s.world.active?s.world.rooms[s.world.act_index].region:0;
 a.saved_at=progress_matches(a.progress.current,s)&&a.progress.current_saved_revision==s.revision?a.progress.current_saved_at:0;
}
static void forget_saved(App&a){
 state_workspace::invalidate_saved_probe(&a);
 a.has_save=false;a.saved_character=Character::Ironclad;a.saved_seed=a.saved_at=0;
 a.saved_floor=a.saved_hp=a.saved_max_hp=a.saved_gold=0;a.saved_run_seed=0;a.saved_ascension=a.saved_region=a.saved_current_region=0;
 a.saved_world=a.saved_practice=false;
}
static State* read_saved(App&a){
 // The host decodes once into scratch storage. Failed reads cannot touch the
 // visible simulation or sever its association with a previously saved run.
 State*probe=state_workspace::begin_saved_read(&a);
 if(!probe)return nullptr;
 const bool valid=a.callbacks.load&&a.callbacks.load(*probe);
 if(!state_workspace::finish_saved_read(&a,valid)){a.has_save=false;return nullptr;}
 remember_saved(a,*probe);return probe;
}
static bool terminal_probe(State&s){
 if(resumable_run(s)){player_kill(s,true);resolve_pending(s);}
 return validate(s)&&(s.phase==Phase::Dead||s.phase==Phase::Complete);
}
void App::forget_run_checkpoint(){run_save_linked=false;confirmed_run={};}
bool App::refresh_save(){
 const bool existing=read_saved(*this)!=nullptr;
 if(ftue_profile_pending){
  ftue_profile_pending=false;
  // A valid old run is an existing profile even without UI history. Only
  // the new empty profile gets the source default EnableFtues=true.
  if(!existing){progress.tutorial_flags=FtueFresh;progress_dirty=true;}
 }
 return existing&&has_save;
}
bool App::load_saved_run(){
 const State*probe=read_saved(*this);if(!probe||!has_save)return false;
 game=*probe;run_save_linked=true;reset_presentation();reset_map_act_intro(true);begin_progress(true);return true;
}
static bool frontend_continue_saved_run(App&a){
 if(!a.load_saved_run())return false;
 // NMainMenu uses the saved character's same 0.8s transition as Embark.
 // Loading the snapshot must not invoke finish_selected_run: that would
 // reset resumed progress and replace its existing room presentation.
 auto&m=a.character_departure;m.phase=1;m.character=uint8_t(a.game.character);
 m.started=a.map_clock_ms;m.reveal_ms=0;m.continuing=true;
 const int mask=a.content.ui_image(I12_CharacterTransitionFirst+(a.game.character==Character::Silent?1:a.game.character==Character::Necrobinder?2:0));
 a.content.prepare_images(&mask,1);a.pointer_cancel();a.hit_count=0;return true;
}
bool App::save_current_run(){
 if(!callbacks.save||!callbacks.save(game))return false;
 if(progress_matches(progress.current,game)){
  progress.current_saved_at=callbacks.wall_clock?callbacks.wall_clock():0;
  progress.current_saved_revision=game.revision;progress_dirty=true;
 }
 remember_saved(*this,game);run_save_linked=has_save;sync_progress();return persist_progress();
}
bool App::finish_saved_run(){
 // Starting a run does not associate it with another run already on disk.
 // Only Continue or a successful save establishes that ordinary lifecycle.
 if(!run_save_linked)return true;
 State*probe=read_saved(*this);if(!probe)return false;
 if(!has_save||!same_run_identity(saved_run_identity(game),saved_run_identity(*probe))){forget_run_checkpoint();return true;}
 if(callbacks.clear_save){if(!callbacks.clear_save())return false;}
 else{
  // Older hosts can persist a terminal checkpoint instead of removing it.
  // Execute forced death on the probe, and adopt it only after a real save.
  *probe=game;
  if(!callbacks.save||!terminal_probe(*probe)||!callbacks.save(*probe))return false;
  game=*probe;
 }
 forget_saved(*this);forget_run_checkpoint();return true;
}
void App::return_to_title(){
 persist_progress();
 title=true;paused=library=character_select=help=settings_open=compendium_open=statistics_open=history_open=false;frontend_confirm=frontend_choice=0;confirmed_run={};menu=0;
 map_preview.open=false;reset_presentation();refresh_save();
 profiles_open=profile_pending=false;bestiary.open=false;
 run_mode={};
}
void App::accept_frontend_confirm(){
 const unsigned action=frontend_confirm;if(!action)return;
 const SavedRunIdentity confirmed=confirmed_run;
 frontend_confirm=frontend_choice=0;confirmed_run={};pointer_cancel();
 if(action==5){if(profiles_open)request_profile_change(profile_target,true);return;}
 if(action==4){
  UiPreferences defaults{};fast_mode=defaults.fast_mode;show_card_indices=defaults.show_card_indices;show_run_timer=defaults.show_run_timer;long_press=defaults.long_press;skip_intro_logo=defaults.skip_intro_logo;end_turn_hold={};
  text_effects=defaults.text_effects;keyboard_only=defaults.keyboard_only;phobia_mode=defaults.phobia_mode;screen_shake=defaults.screen_shake;bestiary_actions_preferred=defaults.bestiary_actions_preferred;
  settings_language_open=false;invalidate_hand_tiles();persist_preferences();return;
 }
 if(action==3){cancel_map_travel();persist_progress();quit=true;return;}
 if(action==1){
  if(!title)return;
  State*probe=read_saved(*this);if(!probe){message("Could not load the saved run. Please try again.");return;}
  if(!has_save||!same_run_identity(confirmed,saved_run_identity(*probe))){menu=0;message("存档已变更，请重新确认");return;}
  if(callbacks.clear_save){if(!callbacks.clear_save()){message("Could not delete the saved run. Please try again.");return;}}
  else if(!callbacks.save||!terminal_probe(*probe)||!callbacks.save(*probe)){message("Could not save the completed run. Please try again.");return;}
  finish_progress(true,probe);forget_run_checkpoint();forget_saved(*this);menu=0;return;
 }
 if(action==2&&paused&&resumable_run(game)){
  // RunManager.Abandon guarantees a forced player death; death prevention
  // must not turn the requested abandon into a surviving run.
  if(!finish_saved_run()){message("Could not delete the saved run. Please try again.");return;}
  if(resumable_run(game)){player_kill(game,true);resolve_pending(game);}finish_progress(true);paused=false;reset_presentation();
 }
}
bool App::frontend_input(Key key){
 if(frontend_confirm){
  if(key==Key::Back){frontend_confirm=frontend_choice=0;confirmed_run={};pointer_cancel();}
  else if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down)frontend_choice^=1;
  else if(key==Key::Confirm){if(frontend_choice)accept_frontend_confirm();else{frontend_confirm=0;confirmed_run={};pointer_cancel();}}
  return true;
 }
 if(help){if(key==Key::Back||key==Key::Confirm)help=false;return true;}
 if(settings_input(key))return true;
 if(profiles_input(key))return true;
 if(progress_navigation_input(key))return true;
 if(run_modes_input(key))return true;
 if(library||character_select||map_preview.open||detail_card>=0||detail_item>=0||status_actor>=0)return false;
 if(title){
  const int count=title_menu_count();if(menu<0||menu>=count)menu=0;
  if(key==Key::Up||key==Key::Left)menu=(menu+count-1)%count;
  else if(key==Key::Down||key==Key::Right)menu=(menu+1)%count;
  else if(key==Key::Back){open_profiles();}
  else if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;}
  else if(key==Key::Load){
   if(!frontend_continue_saved_run(*this)){refresh_save();message("没有有效的存档");}
  }else if(key==Key::Confirm){switch(front_item(*this,menu)){
   case FrontStart:open_run_menu(RunMenu::Singleplayer);break;
   case FrontContinue:if(!frontend_continue_saved_run(*this)){refresh_save();menu=0;message("没有有效的存档");}break;
   case FrontAbandon:
    if(refresh_save()){confirmed_run=saved_run_identity(*state_workspace::saved_probe(this));frontend_confirm=1;frontend_choice=0;}
    else{menu=0;message("Could not load the saved run. Please try again.");}break;
   case FrontCompendium:open_compendium();break;
   case FrontSettings:settings_open=true;settings_language_open=false;settings_top=settings_tab=settings_item=0;break;
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
  case 1:settings_open=true;settings_language_open=false;settings_top=settings_tab=settings_item=0;break;
  case 2:open_compendium();break;
  case 3:if(resumable_run(game)){frontend_confirm=2;frontend_choice=0;}break;
  case 4:
   if(!resumable_run(game)){if(finish_saved_run())return_to_title();else message("Could not delete the saved run. Please try again.");}
   else if(save_current_run())return_to_title();else message("Could not save the run. Please try again.");break;
 }}else if(key==Key::Quit){frontend_confirm=3;frontend_choice=0;}
 else if(key==Key::Collection)return false;
 return true;
}
