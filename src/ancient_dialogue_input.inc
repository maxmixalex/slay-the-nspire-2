static void ancient_line_key(const App&a,const char*speaker,unsigned sequence,unsigned line,bool repeating,const char*kind,char*out,size_t capacity){
 TextBuffer key{out,capacity,0};key.text(original_event_key(a.game.original_events.id));key.text(".talk.");key.text(speaker);key.ch('.');key.number(int(sequence));key.ch('-');key.number(int(line));if(repeating)key.ch('r');key.ch('.');key.text(kind);
}
static unsigned ancient_dialogue_load(App&a,const char*speaker,unsigned sequence,bool repeating){
 auto&d=a.ancient_dialogue;d.count=d.character_lines=0;
 for(unsigned line=0;line<AncientDialoguePresentation::Capacity;++line){
  char key[AncientDialoguePresentation::KeyCapacity]{};ancient_line_key(a,speaker,sequence,line,repeating,"ancient",key,sizeof key);
  if(!*a.content.world_string("ancients",key)){
   ancient_line_key(a,speaker,sequence,line,repeating,"char",key,sizeof key);
   if(!*a.content.world_string("ancients",key))break;d.character_lines|=uint8_t(1u<<line);
  }
  TextBuffer copy{d.keys[line],sizeof d.keys[line],0};copy.text(key);++d.count;
 }
 return d.count;
}
bool App::ancient_dialogue_ready()const{
 return !title&&!paused&&!help&&!settings_open&&!frontend_confirm&&!panel&&!library&&!character_select&&
  !map_preview.open&&!map_window.visible&&!event_reader&&detail_card<0&&detail_item<0&&status_actor<0&&
  deck_candidate<0&&game.phase==Phase::Event&&game.original_events.active&&ui_ancient_event(game.original_events.id);
}
void App::sync_ancient_dialogue(){
 const auto&e=game.original_events;if(!e.active||!ui_ancient_event(e.id))return;
 auto&d=ancient_dialogue;const uint32_t revision=native_event_revision(e);
 const bool new_room=!d.bound||d.seed!=game.world.run_seed||d.serial!=progress.current.serial||
  d.id!=unsigned(e.id)||d.act!=game.world.act_index||d.point!=game.world.current_point;
 if(!new_room){
  if(d.revision!=revision||d.finished!=bool(e.finished)){
   // NEventRoom.RefreshEventState clears the conversation before displaying
   // its resulting description/options. It does not manufacture a reply.
   d.count=d.current=d.character_lines=0;d.measured=false;d.scroll=0;d.start=map_clock_ms;
   d.revision=revision;d.finished=e.finished;
  }
  if(d.english!=content.english()){d.english=content.english();d.measured=false;}
  return;
 }
 d={};d.bound=true;d.seed=game.world.run_seed;d.serial=progress.current.serial;d.id=unsigned(e.id);
 d.act=game.world.act_index;d.point=game.world.current_point;d.revision=revision;d.finished=e.finished;
 d.start=map_clock_ms;d.english=content.english();
 d.random^=uint32_t(d.seed)^uint32_t(d.seed>>32)^d.serial*0x9e3779b9u^d.id*0x85ebca6bu;
 if(e.finished)return;
 unsigned total_visits=0,character_visits=0;
 d.history_complete=progress_loaded&&progress.current.serial&&!progress.current.partial&&
  progress.next_serial<=unsigned(progress.count)+2;
 const auto observe=[&](const UiRunHistory&r,bool current){
  if(!r.serial)return;if(r.partial)d.history_complete=false;
  for(unsigned i=0;i<r.floor_count&&i<UiHistoryFloors;++i){const auto&floor=r.floors[i];
   if(floor.event!=unsigned(e.id)||(current&&floor.act==d.act&&floor.point==d.point))continue;
   ++total_visits;if(r.character==unsigned(game.character))++character_visits;
  }
 };
 for(unsigned i=0;i<progress.count&&i<UiHistoryCapacity;++i)if(progress.history[i].serial!=progress.current.serial)observe(progress.history[i],false);
 if(progress.current.run_seed==game.world.run_seed)observe(progress.current,true);
 const char*character=character_definitions[unsigned(game.character)].key;
 // All eight locked models define character sequences at visits 0, 1 and 4.
 // Their localization's r suffix is the source repeating flag. With missing
 // historical records, recorded counts are only lower bounds: do not claim
 // an exact numbered visit or invent a character conversation.
 if(!total_visits){ancient_dialogue_load(*this,"firstVisitEver",0,false);return;}
 if(d.history_complete&&(character_visits==0||character_visits==1||character_visits==4)){
  const unsigned sequence=character_visits==4?2:character_visits;
  if(ancient_dialogue_load(*this,character,sequence,sequence==1))return;
 }
 struct Candidate{uint8_t sequence;bool character;};Candidate candidates[9]{};unsigned count=0;
 char key[AncientDialoguePresentation::KeyCapacity]{};
 if(character_visits>=1){ancient_line_key(*this,character,1,0,true,"ancient",key,sizeof key);
  if(*content.world_string("ancients",key))candidates[count++]={1,true};
  else{ancient_line_key(*this,character,1,0,true,"char",key,sizeof key);if(*content.world_string("ancients",key))candidates[count++]={1,true};}
 }
 // Tezcatara explicitly excludes Defect from any-character dialogue.
 if(e.id!=OriginalEventId::Tezcatara||game.character!=Character::Defect)for(unsigned i=0;i<8;++i){
  ancient_line_key(*this,"ANY",i,0,true,"ancient",key,sizeof key);if(!*content.world_string("ancients",key))break;
  candidates[count++]={uint8_t(i),false};
 }
 if(count){const auto selected=candidates[d.choose(count)];if(ancient_dialogue_load(*this,selected.character?character:"ANY",selected.sequence,true))return;}
 // Explicit historyless fallback, also used for a legacy profile whose
 // character-specific visit count is unavailable.
 ancient_dialogue_load(*this,"firstVisitEver",0,false);
}
void App::ancient_dialogue_text(unsigned line,char*out,size_t capacity)const{
 if(!capacity)return;out[0]=0;
 if(line<ancient_dialogue.count)localized_text("ancients",ancient_dialogue.keys[line],out,capacity,int(game.original_events.id));
}
void App::next_ancient_dialogue(){
 if(!ancient_dialogue_ready())return;sync_ancient_dialogue();auto&d=ancient_dialogue;
 if(d.last())return;++d.current;d.measured=false;d.scroll=0;pointer_cancel();
}
bool App::scroll_ancient_dialogue(int rows){
 if(!rows||!ancient_dialogue_ready())return false;sync_ancient_dialogue();auto&d=ancient_dialogue;
 d.scroll=int16_t(mn(int(d.max_scroll),mx(0,int(d.scroll)-rows*14)));return true;
}
bool App::ancient_dialogue_input(Key key){
 if(!ancient_dialogue_ready())return false;sync_ancient_dialogue();auto&d=ancient_dialogue;
 if(key==Key::Up||key==Key::Down){scroll_ancient_dialogue(key==Key::Up?-1:1);return true;}
 if(!d.last()&&(key==Key::Confirm||key==Key::EndTurn)){next_ancient_dialogue();return true;}
 if(!d.last()&&(key==Key::Left||key==Key::Right))return true;
 // Source enables pointer buttons on the last line, then keyboard focus at
 // 0.8s. The content still completes its one-second ExpoOut slide.
 if(key==Key::Confirm&&!pointer.dispatching&&map_clock_ms-d.start<800)return true;
 return false;
}
