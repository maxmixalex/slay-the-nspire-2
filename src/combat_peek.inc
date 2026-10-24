// NPeekButton hides its registered selection controls and backstop while
// retaining the selection transaction. No gameplay command runs here.
bool App::combat_peek_context()const{
 return choice_phase(game.phase)&&(combat_phase(game.phase)||
  (game.phase==Phase::WorldRelicChoice&&combat_phase(Phase(game.world_relics.return_phase))));
}
bool App::combat_peek_available()const{
 return !title&&!paused&&!help&&!settings_open&&!frontend_confirm&&!panel&&!library&&!character_select&&!map_preview.open&&
  detail_card<0&&detail_item<0&&status_actor<0&&combat_peek_context()&&combat_choice_ready();
}
unsigned App::combat_peek_amount()const{
 const unsigned age=map_clock_ms-choice_peek_at,p=CombatCardMotion::cubic(age,250),target=choice_peeking?1024u:0u;
 return unsigned((choice_peek_from*(1024u-p)+target*p)/1024u);
}
void App::sync_combat_peek(){
 if(!combat_peek_context()||choice_peek_revision!=game.revision){
  choice_peeking=false;choice_peek_from=0;choice_peek_at=map_clock_ms;choice_peek_revision=game.revision;
 }
}
void App::toggle_combat_peek(){
 sync_combat_peek();if(!combat_peek_available())return;
 choice_peek_from=uint16_t(combat_peek_amount());choice_peek_at=map_clock_ms;choice_peeking=!choice_peeking;
 pointer_cancel();keyword_hover={};
}
bool App::combat_peek_input(Key key){
 sync_combat_peek();if(!choice_peeking||!combat_peek_available())return false;
 if(key==Key::Back){toggle_combat_peek();return true;}
 if(key==Key::Left||key==Key::Right){
  for(int step=1;step<=game.enemy_count;++step){const int slot=(target+(key==Key::Left?-step:step)+game.enemy_count)%game.enemy_count;
   if(combat_health(slot+1).hp>0){target=slot;break;}}return true;
 }
 if(key==Key::Detail){open_statuses(target>=0&&target<game.enemy_count?target+1:0);return true;}
 return key==Key::Up||key==Key::Down||key==Key::Confirm||key==Key::EndTurn;
}
