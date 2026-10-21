#pragma once
#include "app.hpp"
namespace spire {
// UI observation is scoped to this exact State and restores the previous
// binding when nested input handlers finish. It cannot alter save bytes/RNG.
struct UiCoreObserver {
 App& app;CoreObserver previous;bool was_combat;
 uint64_t room_seed;uint32_t window_generation;uint8_t room_act,room_point,room_open;
 static void receive(void* context,const State&s,Log log){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_core(s,log);}
 static void receive_animation(void* context,const State&s,int slot,const char*trigger){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_enemy_animation(s,slot,trigger);}
 explicit UiCoreObserver(App&a):app(a),previous(set_core_observer({&a,receive,receive_animation})),was_combat(combat_phase(a.game.phase)),
  room_seed(a.game.world.run_seed),window_generation(a.map_window_generation),room_act(a.game.world.act_index),room_point(a.game.world.current_point),room_open(a.game.world.room_open){app.sync_enemy_feedback();}
 ~UiCoreObserver(){
  set_core_observer(previous);app.sync_enemy_feedback();app.sync_turn_banner();app.sync_treasure_presentation();
  if(app.game.enemy_world.active&&combat_phase(app.game.phase)&&int32_t(app.enemy_visual_clock-app.animation)>app.busy)app.busy=int(app.enemy_visual_clock-app.animation);
  if(was_combat&&app.game.enemy_world.active&&!combat_phase(app.game.phase)&&app.game.phase!=Phase::Dead){
   bool living=false;for(int i=0;i<app.game.enemy_count;i++)living|=app.game.enemies[i].hp>0;
   if(!living)for(int i=0;i<app.game.enemy_count;i++){const auto&m=app.enemy_presentation[i];
    const uint32_t end=m.removing?m.hide_at:m.until;
    if(m.dead&&(m.removing||!m.loop)&&int32_t(end-app.combat_linger_until)>0)app.combat_linger_until=end;}
  }
  // Explicit ordinary Proceed keeps the same room, marked 2 by the
  // controller. Merely enabling travel (e.g. completing a rest option) does
  // not open the map. New acts, replacement runs and loads use their own
  // lifecycle instead of replaying an old room's open transition.
  if(window_generation==app.map_window_generation&&room_open==1&&app.game.world.room_open==2&&room_seed==app.game.world.run_seed&&
     room_act==app.game.world.act_index&&room_point==app.game.world.current_point)
   app.open_map_window();
  app.sync_map_window();
 }
 UiCoreObserver(const UiCoreObserver&)=delete;
 UiCoreObserver& operator=(const UiCoreObserver&)=delete;
};
}
