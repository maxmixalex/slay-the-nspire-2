#pragma once
#include "app.hpp"
namespace spire {
// UI observation is scoped to this exact State and restores the previous
// binding when nested input handlers finish. It cannot alter save bytes/RNG.
struct UiCoreObserver {
 App& app;CoreObserver previous;bool was_combat;
 static void receive(void* context,const State&s,Log log){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_core(s,log);}
 static void receive_animation(void* context,const State&s,int slot,const char*trigger){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_enemy_animation(s,slot,trigger);}
 explicit UiCoreObserver(App&a):app(a),previous(set_core_observer({&a,receive,receive_animation})),was_combat(combat_phase(a.game.phase)){app.sync_enemy_feedback();}
 ~UiCoreObserver(){
  set_core_observer(previous);app.sync_enemy_feedback();
  if(app.game.enemy_world.active&&combat_phase(app.game.phase)&&int32_t(app.enemy_visual_clock-app.animation)>app.busy)app.busy=int(app.enemy_visual_clock-app.animation);
  if(was_combat&&app.game.enemy_world.active&&!combat_phase(app.game.phase)&&app.game.phase!=Phase::Dead){
   bool living=false;for(int i=0;i<app.game.enemy_count;i++)living|=app.game.enemies[i].hp>0;
   if(!living)for(int i=0;i<app.game.enemy_count;i++){const auto&m=app.enemy_presentation[i];if(m.dead&&!m.loop&&int32_t(m.until-app.combat_linger_until)>0)app.combat_linger_until=m.until;}
  }
 }
 UiCoreObserver(const UiCoreObserver&)=delete;
 UiCoreObserver& operator=(const UiCoreObserver&)=delete;
};
}
