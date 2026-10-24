#include "combat_layout.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
using namespace spire;
static State game,before;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"combat layout:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void fight(EncounterId encounter){
    new_run(game,0x6117,false);game.phase=Phase::Combat;game.turn=1;
    CHECK(encounter_setup(game,encounter));CHECK(validate(game));
}
static bool same(CreaturePose a,CreaturePose b){
 return a.x==b.x&&a.feet==b.feet&&a.width==b.width&&a.scale==b.scale&&a.intent==b.intent&&a.hp==b.hp&&a.status==b.status&&
  a.body_left==b.body_left&&a.body_top==b.body_top&&a.body_width==b.body_width&&a.body_height==b.body_height&&a.center_x==b.center_x&&a.center_y==b.center_y&&a.intent_x==b.intent_x&&a.sprite_x==b.sprite_x&&a.sprite_y==b.sprite_y;
}
// Independent source-space oracle: Godot's authored parent positions, pivot
// and scales are composed as separate floating transforms. Production uses
// camera-scaled integer formation coordinates and a retained rational fit.
// Sources: NGame.FourByThree, combat_room.tscn, NCombatRoom's Ready/position/
// aspect methods, and the numeric CreatureVisuals scene metadata.
struct SourcePoint {long double x,y;};
static SourcePoint local_enemy(int index){
 const auto camera=encounter_camera(game.enemy_world.encounter);const long double zoom=camera.zoom/1000.0L;
 const auto*scene=encounter_positions(game.enemy_world.encounter);const auto&m=game.enemy_world.models[index];unsigned slot=m.slot;
 if(game.enemy_world.encounter==EncounterId::GremlinMercNormal)slot=m.kind==EnemyId::GremlinMerc?0:m.kind==EnemyId::SneakyGremlin?1:2;
 if(scene&&slot<scene->count)return {scene->points[slot][0]-960.0L,scene->points[slot][1]-540.0L};
 long double widths[MaxEnemies]{},sum=0;for(int i=0;i<game.enemy_count;++i){widths[i]=creature_bounds(game.enemy_world.models[i].kind).width;sum+=widths[i];}
 const long double half=960/zoom;long double gap=70,span=sum+(game.enemy_count-1)*gap,left=(half-span)/2,stagger=0;
 if(left<150)left=150;
 if(left+span>half&&game.enemy_count>1){gap=(half-150-sum)/(game.enemy_count-1);if(gap<5)gap=5;span=sum+(game.enemy_count-1)*gap;left=(half-span)/2;if(gap<30)stagger=60-(gap-5)*20/25;}
 for(int i=0;i<index;++i)left+=widths[i]+gap;return {left+widths[index]/2,200-(index%2?stagger:0)};
}
static SourcePoint local_player(bool pet=false){
 const long double zoom=encounter_camera(game.enemy_world.encounter).zoom/1000.0L;
 const long double player=game.enemy_world.encounter==EncounterId::KaiserCrabBoss?0:-480/zoom;
 if(!pet)return {player-(game.character==Character::Necrobinder?150:0),200};
 const auto b=player_anchors(game.character);const int hp=game.expansion.osty_max_hp<0?0:game.expansion.osty_max_hp>150?150:game.expansion.osty_max_hp;
 return {player+b.right-b.left+150+100.0L*hp/150,125};
}
static long double source_right(){
 const auto camera=encounter_camera(game.enemy_world.encounter);const long double zoom=camera.zoom/1000.0L;
 const long double origin=960+(840-960)*zoom+camera.x;long double right=0;
 const auto include=[&](SourcePoint p,int width){const long double edge=origin+(p.x+width/2.0L)*zoom;if(edge>right)right=edge;};
 const auto b=player_anchors(game.character);include(local_player(),b.right-b.left);
 if(game.character==Character::Necrobinder||game.expansion.osty_max_hp>0)include(local_player(true),232);
 for(int i=0;i<game.enemy_count;++i)include(local_enemy(i),creature_bounds(game.enemy_world.models[i].kind).width);
 return right+15>1680?right+15:1680;
}
static SourcePoint reference_point(SourcePoint local,bool enemy,long double right){
 const auto camera=encounter_camera(game.enemy_world.encounter);const long double zoom=camera.zoom/1000.0L,ratio=1680/right;
 const SourcePoint parent{840-(enemy?(right-1680)*ratio:0),630};
 return {(960+(parent.x+local.x*ratio-960)*zoom+camera.x)*4/21,
         (540+(parent.y+local.y*ratio-540)*zoom+camera.y)*4/21};
}
static int pixel(long double value){return int(value+0.000000001L);}
static void agrees_with_source(int index){
 const auto layout=combat_layout_for(game);const auto p=enemy_pose(game,index,layout);const auto local=local_enemy(index);const auto a=creature_anchors(game.enemy_world.models[index].kind);
 const long double right=source_right();CHECK(std::fabs(layout.right/1000.0L-right)<0.002L);
 const auto point=reference_point(local,true,right),top=reference_point({local.x+a.left,local.y+a.top},true,right),bottom=reference_point({local.x+a.right,local.y+a.bottom},true,right);
 const auto center=reference_point({local.x+a.center_x,local.y+a.center_y},true,right);
 CHECK(p.x==pixel(point.x)&&p.feet==pixel(point.y));CHECK(p.body_left==pixel(top.x)&&p.body_top==pixel(top.y));
 CHECK(p.body_width==pixel(bottom.x)-pixel(top.x)&&p.body_height==pixel(bottom.y)-pixel(top.y));
 CHECK(p.center_x==pixel(center.x)&&p.center_y==pixel(center.y));
 CHECK(p.intent_x==pixel(reference_point({local.x+a.intent_x,local.y+a.intent_y},true,right).x));
 CHECK(p.sprite_x==p.x&&p.sprite_y==p.feet);
 const long double scale=creature_sprite_units(game.enemy_world.models[index].kind)*
  (encounter_camera(game.enemy_world.encounter).zoom/1000.0L)*(1680/right)*4/21;
 CHECK(p.scale==(scale<1?1:pixel(scale)));
 CHECK(creature_health_left(p)==p.body_left+(p.body_width-p.width)/2);
 const auto player=player_pose(game,layout);const auto want_player=reference_point(local_player(),false,right);
 CHECK(player.x==pixel(want_player.x)&&player.feet==pixel(want_player.y));
 const auto registration=player_sprite_registration(game.character);const auto source=local_player();
 const auto want_sprite=reference_point({source.x+registration.x,source.y+registration.y},false,right);
 CHECK(player.sprite_x==pixel(want_sprite.x)&&player.sprite_y==pixel(want_sprite.y));
}
static void original_hand_fan(){
 // Independent authored samples and floating source projection catch a
 // cache-allocation scale/pivot being mistaken for the300x422 card frame.
 struct Sample {int count,index,x,y,degrees;};
 const Sample samples[]={{1,0,0,-50,0},{2,0,-100,-50,-2},{3,1,0,-59,0},
  {4,0,-240,-25,-8},{5,0,-340,10,-8},{5,1,-170,-30,-4},{5,2,0,-50,0},
  {6,0,-460,13,-9},{6,1,-273,-25,-6},{7,0,-534,18,-9},{7,2,-189,-39,-3},
  {8,0,-565,28,-12},{8,2,-231,-39,-6},{9,0,-600,37,-12},{9,3,-150,-45,-3},
  {10,0,-610,38,-15},{10,1,-472,5,-12},{10,3,-200,-41,-6},{10,9,610,38,15}};
 for(const auto&v:samples){const auto p=hand_pose(v.count,v.index,false);const double source_scale=.8*(v.count<8?1:1-.05*(v.count-7));
  CHECK(std::abs(p.cx-(840+v.x)*4.0/21)<=.51&&std::abs(p.cy-(1260+v.y)*4.0/21)<=.51);
  CHECK(std::abs(p.scale*69.0/1024-300*source_scale*4/21)<.05);
  const double angle=v.degrees*std::acos(-1.0)/180;
  CHECK(std::abs(p.sine-1024*std::sin(angle))<.51&&std::abs(p.cosine-1024*std::cos(angle))<.51);
  const auto focused=hand_pose(v.count,v.index,true);CHECK(focused.cx==p.cx&&focused.cy==200&&focused.scale==848&&focused.sine==0);
 }
 CHECK(hand_pose(5,0,false,2).cx==86&&hand_pose(5,1,false,2).cx==113&&hand_pose(5,3,false,2).cx==207&&hand_pose(5,4,false,2).cx==234);
 const auto selected=hand_selected_pose();CHECK(selected.cx==160&&selected.cy==210&&selected.scale==636);
 for(int count=1;count<=10;++count)for(int i=0;i<count;++i){const auto p=hand_pose(count,i,false);const auto bounds=hand_bounds(p);
  CHECK(hand_contains(p,p.cx,p.cy));CHECK(!hand_contains(p,bounds.x-1,p.cy));CHECK(!hand_contains(p,p.cx,bounds.y-1));
  if(p.sine)CHECK(!hand_contains(p,bounds.x,bounds.y));
 }
}
static void authored_widths(){
 fight(EncounterId::NibbitsWeak);auto p=enemy_pose(game,0);
 CHECK(combat_layout_for(game).right==1680000);CHECK(p.x==251&&p.feet==158&&p.body_width==52&&p.scale==311);
 CHECK(combat_player_x(game)==68);game.character=Character::Necrobinder;CHECK(combat_player_x(game)==40);
 fight(EncounterId::CorpseSlugsNormal);CHECK(!encounter_positions(game.enemy_world.encounter));
 // Three 232px Bounds use source centers191,480,769. Their right edge
 // plus15 is1740, so this encounter exercises the actual4:3 fit branch.
 CHECK(combat_layout_for(game).right==1740000);
 for(int i=0;i<3;++i)agrees_with_source(i);
 const auto layout=combat_layout_for(game);const auto middle=enemy_pose(game,1,layout),right=enemy_pose(game,2,layout);
 damage_unpowered(game,0,9999);resolve_pending(game);
 CHECK(!game.enemies[0].hp&&same(middle,enemy_pose(game,1,layout))&&same(right,enemy_pose(game,2,layout)));
 fight(EncounterId::ScrollsOfBitingNormal);
 for(int i=0;i<4;++i){agrees_with_source(i);if(i)CHECK(enemy_pose(game,i).x>enemy_pose(game,i-1).x);}
 CHECK(enemy_pose(game,1).feet<enemy_pose(game,0).feet&&enemy_pose(game,1).feet==enemy_pose(game,3).feet);
 fight(EncounterId::SlimesNormal);CHECK(encounter_camera(game.enemy_world.encounter).zoom==900);
 for(int i=0;i<game.enemy_count;++i)agrees_with_source(i);CHECK(enemy_pose(game,0).scale>enemy_pose(game,2).scale);
 fight(EncounterId::AeonglassBoss);p=enemy_pose(game,0);CHECK(p.x==253&&p.feet==164&&p.scale==1084);agrees_with_source(0);
}
static void scene_slots_without_slots_override(){
 for(auto encounter:{EncounterId::BowlbugsNormal,EncounterId::BowlbugsWeak,EncounterId::KnightsElite,EncounterId::SlumberingBeetleNormal}){
  fight(encounter);for(int i=0;i<game.enemy_count;++i)agrees_with_source(i);
 }
 CHECK(enemy_pose(game,2).scale>enemy_pose(game,0).scale*2);
 fight(EncounterId::GremlinMercNormal);const auto layout=combat_layout_for(game);agrees_with_source(0);
 damage_unpowered(game,0,9999);resolve_pending(game);CHECK(game.phase==Phase::Combat&&game.enemy_count==2);
 int fat=-1,sneaky=-1;
 for(unsigned i=0;i<game.enemy_count;++i){if(game.enemy_world.models[i].kind==EnemyId::FatGremlin)fat=int(i);if(game.enemy_world.models[i].kind==EnemyId::SneakyGremlin)sneaky=int(i);}
 CHECK(fat>=0&&sneaky>=0&&fat!=sneaky);CHECK(enemy_pose(game,fat,layout).x>enemy_pose(game,sneaky,layout).x);
 CHECK(enemy_pose(game,fat,layout).feet<enemy_pose(game,sneaky,layout).feet);CHECK(validate(game));
}
static void catalog_and_purity(){
 unsigned scene_count=0;
 for(unsigned id=1;id<EncounterKinds;++id){
  fight(EncounterId(id));before=game;if(encounter_positions(EncounterId(id)))++scene_count;
  for(int i=0;i<game.enemy_count;++i){const auto p=enemy_pose(game,i);agrees_with_source(i);
   CHECK(p.x>0&&p.x<320&&p.feet>30&&p.feet<200&&p.width>=22&&p.width<160&&p.scale>0&&p.scale<8192);
   CHECK(p.body_width>0&&p.body_height>0&&p.body_top<p.feet&&same(p,enemy_pose(game,i)));
  }
  CHECK(!std::memcmp(&before,&game,sizeof game));
 }
 CHECK(scene_count==24);
 fight(EncounterId::OvicopterNormal);const auto layout=combat_layout_for(game);const auto before_summon=enemy_pose(game,0,layout);
 queue_extension(game,XOp::EnemySpawn,int(EnemyId::ToughEgg),8);resolve_pending(game);
 CHECK(same(before_summon,enemy_pose(game,0,layout))&&enemy_pose(game,1,layout).scale<before_summon.scale);
 fight(EncounterId::KaiserCrabBoss);for(int i=0;i<game.enemy_count;++i)agrees_with_source(i);
 const int ironclad=combat_player_x(game);game.character=Character::Necrobinder;CHECK(combat_player_x(game)<ironclad);
 game.enemy_world.active=0;CHECK(combat_player_x(game)==60&&same(enemy_pose(game,0),enemy_pose(game.enemy_count,0)));
}
int main(){original_hand_fan();authored_widths();scene_slots_without_slots_override();catalog_and_purity();std::printf("combat layout: %u checks passed\n",checks);}
