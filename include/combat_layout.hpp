#pragma once
#include "core.hpp"
namespace spire {
// Integer-only presentation geometry. Q10 scales/angles are deliberately
// independent of the gameplay seed and clock. Cards keep a stable lower rail
// while their focused preview is raised above it.
struct HandPose {int cx,cy,scale,sine,cosine;};
inline HandPose hand_pose(int count,int index,bool focused){
    if(count<1)count=1;if(count>MaxHand)count=MaxHand;
    if(index<0)index=0;if(index>=count)index=count-1;
    int step=count>1?162/(count-1):0;if(step>40)step=40;
    int cx=160-(count-1)*step/2+index*step;
    int lane=2*index-(count-1),sine=lane*18;
    if(focused)return {cx<95?95:cx>211?211:cx,178,1024,0,1024};
    return {cx,210+lane*lane/10,672,sine,1024-sine*sine/2048};
}
struct HandRail {int x,y,w,h;};
inline HandRail hand_rail(int count,int index){
    auto p=hand_pose(count,index,false);
    int left=index?(hand_pose(count,index-1,false).cx+p.cx)/2:p.cx-28;
    int right=index+1<count?(hand_pose(count,index+1,false).cx+p.cx)/2:p.cx+28;
    return {left,190,right-left,50};
}
constexpr int HandTileW=80,HandTileH=112;
// Original merchant_room.tscn uses one scene transform for background,
// merchant and the relaxed player. 1920x1080 is uniformly covered by the
// 320x218 room below the top bar, never fitted separately per skeleton.
struct MerchantLayout {
 static constexpr int BodyX=209,BodyY=116,BodyW=55,BodyH=67;
};
struct CreaturePose {int x,feet,width,scale,intent,hp,status;};
inline CreaturePose enemy_pose(int count,int index){
 if(count<1)count=1;if(count>MaxEnemies)count=MaxEnemies;
 if(index<0)index=0;if(index>=count)index=count-1;
 if(count==1)return {238,122,60,1024,50,129,149};
 if(count==2)return {178+index*88,122,60,1024,50,129,149};
 if(count<=4){int width=180/count;return {137+width/2+index*width,122,width-4,720,58,129,148};}
 int columns=(count+1)/2,row=index/columns,col=index%columns,width=180/columns;
 int feet=row?145:86;return {137+width/2+col*width,feet,width-5,448,row?113:50,feet+3,feet+18};
}
struct EncounterPositions {EncounterId encounter;uint8_t count;uint16_t zoom;int16_t offset_x,offset_y;int16_t points[6][2];};
inline const EncounterPositions* encounter_positions(EncounterId encounter){
 static constexpr EncounterPositions layouts[]={
#include "combat_encounter_positions.inc"
 };
 unsigned low=0,high=sizeof layouts/sizeof layouts[0];
 while(low<high){const unsigned middle=(low+high)/2;if(unsigned(layouts[middle].encounter)<unsigned(encounter))low=middle+1;else high=middle;}
 return low<sizeof layouts/sizeof layouts[0]&&layouts[low].encounter==encounter?&layouts[low]:nullptr;
}
struct EncounterCamera {int zoom=1000,x=0,y=0;};
inline EncounterCamera encounter_camera(EncounterId encounter){
 if(const auto*p=encounter_positions(encounter))return {p->zoom,p->offset_x,p->offset_y};
 // All remaining camera overrides belong to encounters without a scene.
 switch(encounter){
 case EncounterId::AeonglassBoss:return {900,0,60};
 case EncounterId::BygoneEffigyElite:return {880,0,50};
 case EncounterId::CeremonialBeastBoss:case EncounterId::MechaKnightElite:
 case EncounterId::SlimesNormal:case EncounterId::VantomBoss:return {900,0,50};
 case EncounterId::KnowledgeDemonBoss:return {850,0,70};
 case EncounterId::LagavulinMatriarchBoss:case EncounterId::TerrorEelElite:
 case EncounterId::TheInsatiableBoss:case EncounterId::WaterfallGiantBoss:return {900,0,0};
 default:return {};
 }
}
struct CreatureBounds {uint16_t width,height;};
inline CreatureBounds creature_bounds(EnemyId kind){
 static constexpr CreatureBounds bounds[]={
#include "combat_creature_bounds.inc"
 };
 static_assert(sizeof bounds/sizeof bounds[0]==EnemyKinds,"Every model needs a bounds entry");
 return unsigned(kind)<EnemyKinds?bounds[unsigned(kind)]:CreatureBounds{0,0};
}
// Camera-scaled source coordinates in thousandths of a pixel. Scaling the
// formation before dividing preserves the exact 960px half-width and centers
// at fractional zooms; scene markers use the same units and one final /6000.
// Retaining removed models keeps survivors stationary after a death.
struct EnemyFormationPoint {int x=0,y=200000;};
inline EnemyFormationPoint unnamed_enemy_point(const State&s,int index,int zoom){
 if(s.enemy_count<1||s.enemy_count>MaxEnemies||index<0||index>=s.enemy_count||zoom<=0)return {};
 int widths[MaxEnemies]{},sum=0;
 for(int i=0;i<s.enemy_count;++i){const auto b=creature_bounds(s.enemy_world.models[i].kind);widths[i]=int(b.width?b.width:360)*zoom;sum+=widths[i];}
 const int area=960000;int gap=70*zoom,span=sum+(s.enemy_count-1)*gap;
 int left=(area-span)/2;if(left<150*zoom)left=150*zoom;int stagger=0;
 if(left+span>area&&s.enemy_count>1){
  gap=(area-150*zoom-sum)/(s.enemy_count-1);if(gap<5*zoom)gap=5*zoom;
  span=sum+(s.enemy_count-1)*gap;left=(area-span)/2;
  if(gap<30*zoom)stagger=60*zoom-(gap-5*zoom)*20/25;
 }
 int x=left+widths[index]/2;for(int i=0;i<index;++i)x+=widths[i]+gap;
 return {x,200*zoom-(index%2?stagger:0)};
}
inline int combat_player_x(const State&s){
 if(!s.enemy_world.active)return 60;
 // Single-player NCombatRoom.PositionPlayersAndPets centers the ordinary
 // player in the left half. Camera zoom cancels that half-width; the local
 // Necrobinder's additional -150 offset is still camera-scaled.
 const auto camera=encounter_camera(s.enemy_world.encounter);
 const int center=s.enemy_world.encounter==EncounterId::KaiserCrabBoss?960:480;
 return (center+camera.x-(s.character==Character::Necrobinder?150*camera.zoom/1000:0))/6;
}
inline CreaturePose enemy_pose(const State&s,int index){
 if(index<0||index>=s.enemy_count||!s.enemy_world.active)return enemy_pose(s.enemy_count,index);
 const auto*layout=encounter_positions(s.enemy_world.encounter);const auto&model=s.enemy_world.models[index];
 const auto camera=encounter_camera(s.enemy_world.encounter);int z=camera.zoom;
 unsigned slot=model.slot;
 // This encounter has HasScene and literal merc/sneaky/fat SlotNames but
 // no Slots override. Its simulation slots intentionally retain spawn order.
 if(s.enemy_world.encounter==EncounterId::GremlinMercNormal)
  slot=model.kind==EnemyId::GremlinMerc?0:model.kind==EnemyId::SneakyGremlin?1:2;
 EnemyFormationPoint point;
 if(layout&&slot<layout->count)point={(layout->points[slot][0]-960)*z,(layout->points[slot][1]-540)*z};
 else point=unnamed_enemy_point(s,index,z);
 // SceneContainer has a 960,540 pivot and camera offset. Retain the
 // calculator's 122px ground rail while preserving authored slot spacing.
 const int x=(960000+point.x+camera.x*1000)/6000;
 const int feet=122+(540000+point.y+camera.y*1000-740000)/6000;
 const auto bounds=creature_bounds(model.kind);const int body_width=bounds.width,body_height=bounds.height;
 int width=body_width?body_width*z/6000:enemy_pose(s.enemy_count,index).width;
 if(width<22)width=22;if(width>80)width=80;
 // Existing sprites share an 84px normalized body; the original Bounds
 // restores relative creature size (e.g. eggs versus their Ovicopter).
 int scale=body_height?body_height*z*1024/(6000*84):enemy_pose(s.enemy_count,index).scale;
 if(scale<160)scale=160;if(scale>1400)scale=1400;
 const int height=84*scale/1024,intent=feet-height-15;
 return {x,feet,width,scale,intent<25?25:intent,feet+7,feet+24};
}

}
