void App::sync_crystal_presentation(){
 const auto&e=game.original_events;
 if(e.id!=OriginalEventId::CrystalSphere||!e.crystal_item_count)return;
 const bool fresh=crystal_presentation.observe(game,map_clock_ms);
 // The same board stays behind its reward overlay and terminal screen.
 // Those screens own their own cursor; painting a backdrop cannot move it.
 if(game.phase!=Phase::OriginalEventChoice||e.selection!=OriginalEventSelection::Crystal)return;
 if(fresh||cursor<0||cursor>=CrystalCells||e.crystal_clear[cursor]){
  const int next=crystal_layout::nearest(e,fresh?60:cursor<0?60:cursor);
  if(next>=0)cursor=next;
 }
}
void App::crystal_click(int at){
 if(game.phase!=Phase::OriginalEventChoice||game.original_events.selection!=OriginalEventSelection::Crystal||at<0||at>=CrystalCells)return;
 sync_crystal_presentation();
 if(map_clock_ms-crystal_presentation.start<500)return;
 cursor=at;original_event_crystal_click(game,at%11,at/11);crystal_presentation.observe(game,map_clock_ms);
 const int next=crystal_layout::nearest(game.original_events,at);if(next>=0)cursor=next;
}
bool App::crystal_input(Key key){
 sync_crystal_presentation();const auto&e=game.original_events;
 if(key==Key::Confirm){crystal_click(cursor);return true;}
 if(key==Key::DrawPile||key==Key::DiscardPile||key==Key::EndTurn){
  original_event_crystal_tool(game,key==Key::DiscardPile||(key==Key::EndTurn&&e.crystal_tool!=2));return true;
 }
 const int dx=key==Key::Left?-1:key==Key::Right?1:0,dy=key==Key::Up?-1:key==Key::Down?1:0;
 if(!dx&&!dy)return false;
 for(int x=cursor%11+dx,y=cursor/11+dy;x>=0&&x<11&&y>=0&&y<11;x+=dx,y+=dy){
  if(crystal_layout::exists(x,y)&&!e.crystal_clear[y*11+x]){cursor=y*11+x;break;}
 }return true;
}
