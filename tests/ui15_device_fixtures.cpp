// Extend the production-codec device exporter with a controlled Vantom scene.
// These supplied combat prerequisites are explicitly not a natural run.
#define main ui14_device_fixtures_main
#include "ui14_device_fixtures.cpp"
#undef main
#include "debug_commands.hpp"

static int strike_in_hand(const State&s){
 for(unsigned i=0;i<s.hand.count;++i)if(s.cards[s.hand.ids[i]].kind==Strike)return int(i);
 return -1;
}
static int beetle_juice(const State&s){
 for(unsigned i=0;i<potion_slot_count(s);++i)if(potion_at(s,i)==Potion::BeetleJuice)return int(i);
 return -1;
}
static void start_vantom(){
 app.callbacks={};CHECK(app.debug_command("new IRONCLAD 1104089578"));
 CHECK(app.debug_command("fight VANTOM_BOSS"));
 CHECK(app.game.phase==Phase::Combat&&app.game.enemy_count==1&&app.game.enemy_world.models[0].kind==EnemyId::Vantom);
}
int main(int argc,char**argv){
 const int result=ui14_device_fixtures_main(argc,argv);if(result)return result;
 start_vantom();
 // Reach Prepare through its complete ordinary turn sequence. Selecting a
 // node alone would omit the tail's actual charge and combat consequences.
 for(unsigned turn=0;turn<3;++turn){CHECK(app.game.enemy_world.models[0].node==turn);
  CHECK(end_turn(app.game)&&app.game.phase==Phase::Combat&&validate(app.game));}
 CHECK(app.game.enemy_world.models[0].node==3&&app.game.hp>0&&app.game.enemies[0].hp>0);
 after=app.game;CHECK(end_turn(after)&&after.phase==Phase::Combat&&validate(after));
 CHECK(after.enemy_world.models[0].node==0);
 CHECK(save(app.game,original));input.assign(original.bytes,original.bytes+original.size);
 matching_progress("ui15-vantom-prepare");

 start_vantom();
 const int hp=app.game.enemies[0].hp;
 const unsigned protection=unsigned(enemy_power_value(app.game,0,EnemyPower::Slippery));CHECK(protection==8);
 for(unsigned i=0;i<protection;++i){damage_unpowered(app.game,0,1);resolve_pending(app.game);}
 CHECK(!enemy_power_value(app.game,0,EnemyPower::Slippery)&&app.game.enemies[0].hp==hp-int(protection));
 CHECK(app.debug_command("potion BEETLE_JUICE"));
 CHECK(app.debug_command("power STRENGTH_POWER 999 0"));
 if(strike_in_hand(app.game)<0)CHECK(debug_card(app.game,Strike,DebugPile::Hand));
 CHECK(strike_in_hand(app.game)>=0&&beetle_juice(app.game)>=0&&validate(app.game));

 // Verify both instructed device actions through their ordinary core APIs.
 // The exported save contains the live pre-action enemy, never a dead pose.
 after=app.game;CHECK(play(after,strike_in_hand(after),0));
 CHECK(after.enemies[0].hp==0&&after.phase!=Phase::Error&&validate(after));
 after=app.game;CHECK(use_potion(after,beetle_juice(after),0));
 int shrunk=0;CHECK(enemy_visual_fact(after,0,"Shrink",shrunk)&&shrunk);
 CHECK(play(after,strike_in_hand(after),0));
 CHECK(after.enemies[0].hp==0&&after.phase!=Phase::Error&&validate(after));

 CHECK(save(app.game,original));input.assign(original.bytes,original.bytes+original.size);
 matching_progress("ui15-vantom-shrink-death");
 const char note[]="Controlled UI15 Vantom device fixture. Not a natural run or a release save.\n"
  "Setup uses console new IRONCLAD 1104089578, fight VANTOM_BOSS, eight real one-damage hits to consume the eight original Slippery layers, potion BEETLE_JUICE, and power STRENGTH_POWER 999 0. An ordinary Strike is added only if the dealt hand has none.\n"
  "For full-size death: Continue this save and play Strike on Vantom. For shrink plus death: restore the same pair, drink Beetle Juice on Vantom, allow its 750ms scale animation to finish, then play Strike. Both paths are independently verified through use_potion/play before export.\n"
  "The original death command removes Shrink and calls ScaleTo(1,.75), so a previously shrunk Vantom should grow back during the first750ms of death. Its2133ms explosion is therefore at full scale. Do not expect the small body to remain frozen after death.\n"
  "ui15-vantom-prepare is a separate live combat pair made by the same new/fight setup followed by three real end_turn calls. Continue it and press End Turn once for the original Prepare/debuff clip; its saved tail state comes from the actual prior three moves. The fourth turn is independently validated before export.\n"
  "The key death events are tail dissolve at33ms, both sprays at200ms, tail callback at1033ms, and the30-particle explosion at2133ms from the real death clip start. The calculator's33ms paint clock can sample immediately after those boundaries.\n"
  "Install ui15-vantom-shrink-death.sav.tns as state/nSpire2.sav.tns and its matching progress as state/nSpire2.progress.tns on the isolated measurement profile. Progress is marked partial with FTUE dismissed by the ordinary resume path. Read back both files.\n";
 write("README-vantom.txt",note,sizeof note-1);
 std::printf("UI15 device fixtures: %u checks passed; real full-size/shrunk death commands verified\n",checks);
}
