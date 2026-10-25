// NBestiary preserves its mode preference when its submenu closes. All
// previews are detached UI state; neither hotkeys nor scrolling save a run.
static void bestiary_close(App&a){
 if(!a.bestiary.open)return;
 a.bestiary_actions_preferred=a.bestiary.actions;a.persist_preferences();
 ++a.bestiary.input_serial;a.bestiary.open=false;a.physical_keys_cancel();a.pointer_cancel();a.hit_count=0;
}
int App::bestiary_shortcut(unsigned command)const{
 if(!bestiary.open)return -1;
 const auto input=InputCommand(command);
 if(input==InputCommand::Cancel)return 10;
 if(input==InputCommand::Confirm)return 9;
 if(input==InputCommand::Select)return 11;
 if(!bestiary.actions){
  if(input==InputCommand::Deck)return 12;if(input==InputCommand::Exhaust)return 13;return -1;
 }
 if(!keyboard_only&&command>=unsigned(InputCommand::Card1)&&command<unsigned(InputCommand::Card10)){
  const unsigned move=command-unsigned(InputCommand::Card1);return move<bestiary_move_count()?int(move):-1;
 }
 if(keyboard_only){
  const InputCommand shortcuts[]={InputCommand::Deck,InputCommand::Exhaust,InputCommand::Discard,InputCommand::Draw,InputCommand::TopPanel};
  for(unsigned i=0;i<5;++i)if(input==shortcuts[i])return i<bestiary_move_count()?int(i):-1;
 }
 return -1;
}
void App::bestiary_shortcut(unsigned action,bool pressed){
 if(!bestiary.open)return;
 if(pressed){if(action<9&&action<bestiary_move_count())bestiary.moves[action].press(true,map_clock_ms);return;}
 if(action<9){bestiary_play(*this,action);return;}
 switch(action){
 case 9:bestiary_toggle(*this);break;
 case 10:bestiary_close(*this);break;
 case 11:bestiary_input(Key::Confirm);break;
 case 12:bestiary_input(Key::Inspect);break;
 case 13:bestiary_input(Key::ExhaustPile);break;
 default:break;
 }
}
bool App::bestiary_pointer(const HitRegion&hit){
 if(!bestiary.open)return false;
 switch(hit.kind){
 case HitKind::BestiaryEntry:bestiary_select(*this,hit.value);bestiary.focus=0;break;
 case HitKind::BestiaryMove:bestiary_play(*this,unsigned(hit.value));bestiary.focus=1;break;
 case HitKind::BestiaryFilter:bestiary_choose_filter(*this,unsigned(hit.value));bestiary.focus=1;break;
 case HitKind::BestiaryMode:bestiary_toggle(*this);break;
 case HitKind::UiAction:if(hit.value==int(UiAction::Back))bestiary_close(*this);break;
 case HitKind::Close:bestiary_close(*this);break;
 default:break;
 }return true;
}
void App::bestiary_scroll(int rows){if(bestiary.open)bestiary.wheel(rows);}
bool App::bestiary_input(Key key){
 auto&b=bestiary;if(!b.open)return false;
 if(key==Key::Back){bestiary_close(*this);return true;}
 if(key==Key::Quit){quit=true;return true;}
 if(key==Key::EndTurn){bestiary_toggle(*this);return true;}
 const int direction=key==Key::Up?-1:key==Key::Down?1:0;
 if(direction){
  if(!b.focus){int row=b.focused;for(int i=0;i<b.row_count;++i){row=(row+direction+b.row_count)%b.row_count;
    if(bestiary_known(*this,b.rows[row])){b.focused=int16_t(row);b.reveal(row);break;}}
  }else if(b.actions){const unsigned count=bestiary_move_count();if(count)b.action=uint8_t((b.action+int(count)+direction)%int(count));}
  else{int filter=b.filter;for(unsigned i=0;i<6;++i){filter=(filter+direction+6)%6;if(bestiary_filter_enabled(*this,unsigned(filter))){bestiary_choose_filter(*this,unsigned(filter));break;}}}
  return true;
 }
 if(key==Key::Left){b.focus=1;return true;}if(key==Key::Right){b.focus=0;return true;}
 if(key==Key::Confirm){if(!b.focus)bestiary_select(*this,b.focused);else if(b.actions)bestiary_play(*this,b.action);return true;}
 // Original directional-controller action bindings, in their source order.
 if(b.actions){const Key hotkeys[]={Key::Inspect,Key::ExhaustPile,Key::DiscardPile,Key::DrawPile};
  for(unsigned i=0;i<sizeof hotkeys/sizeof hotkeys[0];++i)if(key==hotkeys[i]){bestiary_play(*this,i);return true;}
 }else if(key==Key::Inspect||key==Key::ExhaustPile){
  const int step=key==Key::Inspect?-1:1;int filter=b.filter;for(unsigned i=0;i<6;++i){filter=(filter+step+6)%6;if(bestiary_filter_enabled(*this,unsigned(filter))){bestiary_choose_filter(*this,unsigned(filter));break;}}
 }
 return true;
}
