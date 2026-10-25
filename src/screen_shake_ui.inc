static bool screen_combat_target(const App&a){
 return !a.title&&!a.character_select&&a.game.enemy_world.active&&
  (combat_phase(a.game.phase)||choice_phase(a.game.phase)||a.game.phase==Phase::Reward||int32_t(a.combat_linger_until-a.animation)>0);
}
uint32_t App::screen_motion_clock()const{return screen_combat_target(*this)?animation*33u:map_clock_ms;}
void App::sync_screen_motion(){
 // These are the actual NGame screen-shake targets. Capstones such as the
 // map and settings retain the live room beneath them, not a new target.
 uint32_t context=title?1u:character_select?2u:screen_combat_target(*this)?3u:
  game.world.active?4u+game.world.room_type:32u+unsigned(game.phase);
 if(!title&&!character_select)context=ScreenShakePresentation::hash(context^game.seed^uint32_t(game.floor+1)*0x91e10da5u);
 screen_motion.bind(context,screen_motion_clock());screen_motion.advance(screen_motion_clock());
}
void App::screen_effect(ScreenShakePresentation::Kind kind,unsigned strength,unsigned duration,unsigned angle,unsigned delay){
 sync_screen_motion();const uint32_t now=screen_motion_clock();screen_motion.add(now,now+delay,kind,strength,duration,angle);
}
void App::screen_effect_at(uint32_t tick,ScreenShakePresentation::Kind kind,unsigned strength,unsigned duration,unsigned angle){
 sync_screen_motion();const uint32_t now=screen_motion_clock();screen_motion.add(now,tick*33u,kind,strength,duration,angle);
}
void App::screen_scene_offset(int&x,int&y)const{screen_motion.sample(screen_motion_clock(),screen_shake,x,y);}
static void ui_architect_screen_effect(App&a,bool start){
 if(a.game.original_events.id!=OriginalEventId::TheArchitect)return;
 const auto plan=source_architect_shake(a.game.character,unsigned(a.game.original_events.values[0]),start,a.fast_mode);if(!plan.complete)return;
 for(unsigned i=0;i<plan.hits;++i)a.screen_effect(ScreenShakePresentation::Punch,
  i+1==plan.hits?ScreenShakePresentation::Strong:ScreenShakePresentation::Weak,i+1==plan.hits?800:300,0,plan.hit_at(i));
 if(plan.architect_at)a.screen_effect(ScreenShakePresentation::Punch,ScreenShakePresentation::Weak,800,0,plan.architect_at);
 a.screen_effect(ScreenShakePresentation::SequenceEnd,0,0,0,plan.complete);
 if(!start){a.sync_game_over_presentation();if(a.game_over_presentation.bound)a.game_over_presentation.opened=a.map_clock_ms+plan.complete;}
}
static void screen_translate(Canvas&c,int dx,int dy,int first_hit,int top){
 if(!dx&&!dy)return;
 const int first_y=dy>0?239:top,last_y=dy>0?top-1:240,step_y=dy>0?-1:1;
 const int first_x=dx>0?319:0,last_x=dx>0?-1:320,step_x=dx>0?-1:1;
 for(int y=first_y;y!=last_y;y+=step_y){const int sy=health_bar::bounded(y-dy,top,239);
  for(int x=first_x;x!=last_x;x+=step_x){const int sx=health_bar::bounded(x-dx,0,319);c.pixels[y*c.pitch+x]=c.pixels[sy*c.pitch+sx];}}
 for(int i=first_hit;i<c.a.hit_count;++i){c.a.hits[i].x+=dx;c.a.hits[i].y+=dy;}
}
static void screen_scene_shake(Canvas&c,int first_hit=0,int top=0){int x=0,y=0;c.a.screen_scene_offset(x,y);screen_translate(c,x,y,first_hit,top);}
