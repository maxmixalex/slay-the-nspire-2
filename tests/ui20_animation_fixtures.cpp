// Controlled review fixtures; actual production NSR/NSH codecs and core turns.
#define main ui14_device_fixtures_main
#include "ui14_device_fixtures.cpp"
#undef main
#include "debug_commands.hpp"
#include "keyword_hover.hpp"

static void begin(const char*character,const char*fight){
 app.callbacks={};char command[128];std::snprintf(command,sizeof command,"new %s 1104089578",character);CHECK(app.debug_command(command));
 if(fight){std::snprintf(command,sizeof command,"fight %s",fight);CHECK(app.debug_command(command));}
 CHECK(validate(app.game));
}
static void export_run(const char*stem){CHECK(save(app.game,original));input.assign(original.bytes,original.bytes+original.size);matching_progress(stem);}
int main(int argc,char**argv){
 CHECK(argc==3);output=argv[2];std::filesystem::create_directories(output);const auto art=read(argv[1]);CHECK(app.init(art.data(),art.size(),{}));
 begin("IRONCLAD","CEREMONIAL_BEAST_BOSS");CHECK(app.game.enemy_world.models[0].node==0);
 CHECK(end_turn(app.game)&&validate(app.game)&&app.game.enemy_world.models[0].node==1);
 after=app.game;CHECK(end_turn(after)&&validate(after)&&after.phase==Phase::Combat&&after.enemy_world.models[0].node==1);
 export_run("ui20-ceremonial-plow");

 begin("NECROBINDER","NIBBITS_WEAK");CHECK(app.debug_command("relic remove BOUND_PHYLACTERY"));app.game.enemy_world.models[0].node=0;app.game.expansion.osty_hp=app.game.expansion.osty_max_hp=1;
 CHECK(validate(app.game));after=app.game;CHECK(end_turn(after)&&validate(after));CHECK(after.expansion.osty_hp==0&&after.hp>0&&after.phase==Phase::Combat);
 export_run("ui20-osty-die");
 app.callbacks={};app.game=after;export_run("ui20-osty-dead-loop");
 begin("NECROBINDER","NIBBITS_WEAK");CHECK(app.debug_command("relic remove BOUND_PHYLACTERY"));app.game.enemy_world.models[0].node=0;app.game.hp=1;app.game.expansion.osty_hp=app.game.expansion.osty_max_hp=1;
 CHECK(validate(app.game));after=app.game;CHECK(end_turn(after)&&validate(after)&&after.phase==Phase::Dead&&after.expansion.osty_hp==0);
 export_run("ui20-osty-terminal-die");

 begin("SILENT",nullptr);CHECK(app.debug_command("room Shop"));CardKind candidate=CardKinds;
 for(unsigned k=0;k<CardKinds;++k){HoverTipList tips;collect_card_hover_tips(app,{CardKind(k),0},tips);
  bool only=tips.count>0;for(unsigned i=0;i<tips.count;++i)only&=tips.items[i].kind==HoverTipKind::Card;
  if(only&&card_rarity(CardKind(k))>=2&&card_rarity(CardKind(k))<=4){candidate=CardKind(k);break;}
 }
 CHECK(candidate<CardKinds);app.game.world.shop_cards[0]={candidate,0};app.game.world.shop_facts[0]={};CHECK(validate(app.game));
 std::printf("Card-only shop owner: slot0=%s\n",definitions[candidate].key);export_run("ui20-shop-card-only");

 begin("IRONCLAD","CEREMONIAL_BEAST_BOSS");CHECK(app.debug_command("win"));
 CHECK(validate(app.game));CHECK(app.game.phase==Phase::Reward||app.game.phase==Phase::WorldRelicChoice);
 export_run("ui20-boss-gold-card-reward");
 const char note[]="Controlled UI20 review fixtures, not natural runs or release save contents.\n"
  "Install one .sav.tns as state/nSpire2.sav.tns and matching .progress.tns as state/nSpire2.progress.tns on the isolated review profile, then Continue. Both use production codecs; every save and progress pair was loaded and round-tripped.\n"
  "ui20-ceremonial-plow: original console new IRONCLAD/fight CEREMONIAL_BEAST_BOSS then one actual end_turn to finish Stamp. Continue, wait for combat preparation, press End Turn (the normal E binding or button) once. This executes the live Plow move at native pixel density.\n"
  "ui20-osty-die: new NECROBINDER/fight NIBBITS_WEAK, ordinary console removal of BOUND_PHYLACTERY (prevents next-turn automatic revival), and explicitly supplied attacking move0 plus 1/1 Osty HP. Continue and press End Turn once; the real attack kills Osty while the owner survives. Hover Osty before ending the turn to inspect Die For You's original tooltip icon.\n"
  "ui20-osty-terminal-die: same live setup with owner also at1HP. Continue and End Turn once; actual enemy damage ends the run and must still start Osty's queued death animation at its visual impact.\n"
  "ui20-osty-dead-loop: the actual post-turn canonical result of ui20-osty-die; Continue to verify original persistent corpse motion while HP remains zero.\n"
  "ui20-shop-card-only: new SILENT/room Shop, with slot0 explicitly set to a real ordinary card whose related hover has only derived cards. Open the merchant and hover the first stock card; its preview must directly abut the owner, with no blank keyword-column gap. The exporter prints the selected owner key.\n"
  "ui20-boss-gold-card-reward: new IRONCLAD/fight CEREMONIAL_BEAST_BOSS then the production console win path. Continue and inspect the card reward row before entering card selection. It must use the original gold/rare reward icon.\n";
 write("README-ui20.txt",note,sizeof note-1);std::printf("UI20 production fixtures: %u checks passed\n",checks);return 0;
}
