#pragma once
#include "app.hpp"
namespace spire {
// UI observation is scoped to this exact State and restores the previous
// binding when nested input handlers finish. It cannot alter save bytes/RNG.
struct UiCoreObserver {
 App& app;CoreObserver previous;bool was_combat;
 uint64_t room_seed;uint32_t window_generation;uint8_t room_act,room_point,room_open;
 RewardAcquisitionSnapshot acquisition;
 static void receive(void* context,const State&s,Log log){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_core(s,log);}
 static void receive_animation(void* context,const State&s,int slot,const char*trigger){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_enemy_animation(s,slot,trigger);}
 static void receive_effect(void* context,const State&s,Effect effect){auto*a=static_cast<App*>(context);if(&s==&a->game){a->observe_combat_effect(s,effect);a->observe_progress_effect(s);}}
 static void before_effect(void* context,const State&s,Effect effect){auto*a=static_cast<App*>(context);if(&s==&a->game)a->before_combat_effect(s,effect);}
 static void receive_damage(void*context,const State&s,const CoreDamageResult&result){auto*a=static_cast<App*>(context);if(&s==&a->game){a->observe_combat_damage(s,result);a->observe_progress_damage(s,result);}}
 static void receive_power_flash(void*context,const State&s,int receiver,const char*key){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_power_flash(s,receiver,key);}
 static void receive_deck_removing(void*context,const State&s,int index){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_deck_removing(s,index);}
 static void receive_enemy_spawned(void*context,const State&s,unsigned model,uint16_t identity){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_enemy_spawned(s,model,identity);}
 static void receive_run_fact(void*context,const State&s,unsigned kind,unsigned value){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_run_fact(s,kind,value);}
 static void receive_history(void*context,const State&s,const RunHistoryNotice&notice){auto*a=static_cast<App*>(context);if(&s==&a->game){a->observe_history(s,notice);a->observe_ftue_history(notice);a->observe_card_transform(s,notice);}}
 static void receive_orb(void*context,const State&s,const CoreOrbNotice&notice){auto*a=static_cast<App*>(context);if(&s==&a->game)a->observe_orb_activation(s,notice);}
 explicit UiCoreObserver(App&a):app(a),previous(set_core_observer({&a,receive,receive_animation,receive_effect,before_effect,receive_damage,receive_power_flash,receive_deck_removing,receive_enemy_spawned,receive_run_fact,receive_history,receive_orb})),was_combat(combat_phase(a.game.phase)),
  room_seed(a.game.world.run_seed),window_generation(a.map_window_generation),room_act(a.game.world.act_index),room_point(a.game.world.current_point),room_open(a.game.world.room_open),acquisition(a.reward_acquisition_snapshot()){app.sync_combat_feedback();app.sync_enemy_feedback();}
 ~UiCoreObserver(){
  set_core_observer(previous);app.finish_combat_feedback();app.sync_combat_feedback();app.sync_enemy_feedback();app.sync_turn_banner();app.sync_treasure_presentation();app.sync_camp_presentation();
  app.finish_reward_acquisition(acquisition);
  if(app.game.enemy_world.active&&combat_phase(app.game.phase)&&int32_t(app.enemy_visual_clock-app.animation)>app.busy)app.busy=int(app.enemy_visual_clock-app.animation);
  if(app.combat_feedback.active&&combat_phase(app.game.phase)&&int32_t(app.combat_feedback.end_at-app.animation)>app.busy)app.busy=int(app.combat_feedback.end_at-app.animation);
  // Explicit ordinary Proceed keeps the same room, marked 2 by the
  // controller. Merely enabling travel (e.g. completing a rest option) does
  // not open the map. New acts, replacement runs and loads use their own
  // lifecycle instead of replaying an old room's open transition.
  if(window_generation==app.map_window_generation&&room_open==1&&app.game.world.room_open==2&&room_seed==app.game.world.run_seed&&
     room_act==app.game.world.act_index&&room_point==app.game.world.current_point)
   app.open_map_window();
  app.sync_map_window();app.sync_progress();
 }
 UiCoreObserver(const UiCoreObserver&)=delete;
 UiCoreObserver& operator=(const UiCoreObserver&)=delete;
};
}
