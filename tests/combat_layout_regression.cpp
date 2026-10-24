#include "combat_layout.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static State game,before;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"combat layout:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void fight(EncounterId encounter){
    new_run(game,0x6117,false);game.phase=Phase::Combat;game.turn=1;
    CHECK(encounter_setup(game,encounter));CHECK(validate(game));
}
static bool same(CreaturePose a,CreaturePose b){return a.x==b.x&&a.feet==b.feet&&a.width==b.width&&a.scale==b.scale&&a.intent==b.intent&&a.hp==b.hp&&a.status==b.status;}
static void authored_widths(){
    fight(EncounterId::NibbitsWeak);auto p=enemy_pose(game,0);
    CHECK(p.x==240&&p.feet==122&&p.width==45&&p.scale==296);
    CHECK(combat_player_x(game)==80);game.character=Character::Necrobinder;CHECK(combat_player_x(game)==55);
    fight(EncounterId::CorpseSlugsNormal);CHECK(!encounter_positions(game.enemy_world.encounter));
    // Source Bounds are 232 wide. The 960-wide half has three 232px
    // creatures, two 57px gaps and a 75px inset: 191, 480, 769 centers.
    CHECK(enemy_pose(game,0).x==191&&enemy_pose(game,1).x==240&&enemy_pose(game,2).x==288);
    CHECK(enemy_pose(game,0).feet==enemy_pose(game,1).feet&&enemy_pose(game,1).feet==enemy_pose(game,2).feet);
    const auto middle=enemy_pose(game,1),right=enemy_pose(game,2);
    damage_unpowered(game,0,9999);resolve_pending(game);
    CHECK(!game.enemies[0].hp&&same(middle,enemy_pose(game,1))&&same(right,enemy_pose(game,2)));
    fight(EncounterId::ScrollsOfBitingNormal);
    // Four 183px Bounds compress the gap to 26px. Only alternate creatures
    // move upward by the authored 43.2px; the encounter is still one row.
    const int xs[]={187,222,257,292};
    for(int i=0;i<4;++i){p=enemy_pose(game,i);CHECK(p.x==xs[i]&&p.feet==(i%2?115:122)&&p.scale==337);}
    fight(EncounterId::SlimesNormal);
    CHECK(encounter_camera(game.enemy_world.encounter).zoom==900);
    CHECK(enemy_pose(game,0).feet==127&&enemy_pose(game,1).feet==127);
    CHECK(enemy_pose(game,0).scale>enemy_pose(game,2).scale);
    fight(EncounterId::AeonglassBoss);p=enemy_pose(game,0);
    CHECK(p.x==240&&p.feet==128&&p.scale==963);
}
static void scene_slots_without_slots_override(){
    fight(EncounterId::BowlbugsNormal);
    CHECK(enemy_pose(game,0).x==183&&enemy_pose(game,1).x==228&&enemy_pose(game,2).x==277);
    fight(EncounterId::BowlbugsWeak);CHECK(enemy_pose(game,0).x==204&&enemy_pose(game,1).x==256);
    fight(EncounterId::KnightsElite);CHECK(enemy_pose(game,0).x==169&&enemy_pose(game,2).x==286);
    fight(EncounterId::SlumberingBeetleNormal);
    CHECK(enemy_pose(game,0).x==159&&enemy_pose(game,1).x==191&&enemy_pose(game,2).x==265);
    CHECK(enemy_pose(game,2).scale>enemy_pose(game,0).scale*2);
    fight(EncounterId::GremlinMercNormal);CHECK(enemy_pose(game,0).x==216);
    damage_unpowered(game,0,9999);resolve_pending(game);CHECK(game.phase==Phase::Combat&&game.enemy_count==2);
    int fat=-1,sneaky=-1;
    for(unsigned i=0;i<game.enemy_count;++i){if(game.enemy_world.models[i].kind==EnemyId::FatGremlin)fat=int(i);if(game.enemy_world.models[i].kind==EnemyId::SneakyGremlin)sneaky=int(i);}
    CHECK(fat>=0&&sneaky>=0&&fat!=sneaky);
    CHECK(enemy_pose(game,fat).x==260&&enemy_pose(game,sneaky).x==216);
    CHECK(enemy_pose(game,fat).feet<enemy_pose(game,sneaky).feet);
    CHECK(validate(game));
}
static void catalog_and_purity(){
    unsigned scene_count=0;
    for(unsigned id=1;id<EncounterKinds;++id){
        fight(EncounterId(id));before=game;
        if(encounter_positions(EncounterId(id)))++scene_count;
        for(int i=0;i<game.enemy_count;++i){const auto p=enemy_pose(game,i);
            CHECK(p.x>0&&p.x<320&&p.feet>30&&p.feet<155&&p.width>=22&&p.width<=80&&p.scale>=160&&p.scale<=1400);
            CHECK(same(p,enemy_pose(game,i)));
        }
        CHECK(!std::memcmp(&before,&game,sizeof game));
    }
    CHECK(scene_count==24);
    fight(EncounterId::OvicopterNormal);CHECK(combat_player_x(game)==63);
    const auto before_summon=enemy_pose(game,0);
    queue_extension(game,XOp::EnemySpawn,int(EnemyId::ToughEgg),8);resolve_pending(game);
    CHECK(same(before_summon,enemy_pose(game,0))&&enemy_pose(game,1).scale<before_summon.scale);
    fight(EncounterId::KaiserCrabBoss);CHECK(combat_player_x(game)==160);game.character=Character::Necrobinder;CHECK(combat_player_x(game)==141);
    game.enemy_world.active=0;CHECK(combat_player_x(game)==60&&same(enemy_pose(game,0),enemy_pose(game.enemy_count,0)));
}
int main(){authored_widths();scene_slots_without_slots_override();catalog_and_purity();std::printf("combat layout: %u checks passed\n",checks);}
