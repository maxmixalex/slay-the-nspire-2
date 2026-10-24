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
inline int combat_player_x(const State&s){
 // KaiserCrabBoss.FullyCenterPlayers; the original local Necrobinder then
 // moves 150 source pixels left to leave Osty's place beside her.
 if(s.enemy_world.active&&s.enemy_world.encounter==EncounterId::KaiserCrabBoss)return s.character==Character::Necrobinder?141:160;
 return 60;
}
inline CreaturePose enemy_pose(const State&s,int index){
 if(index<0||index>=s.enemy_count||!s.enemy_world.active)return enemy_pose(s.enemy_count,index);
 const auto*layout=encounter_positions(s.enemy_world.encounter);const auto&model=s.enemy_world.models[index];
 if(!layout||model.slot>=layout->count)return enemy_pose(s.enemy_count,index);
 static constexpr uint16_t bounds[][2]={
#include "combat_creature_bounds.inc"
 };
 static_assert(sizeof bounds/sizeof bounds[0]==EnemyKinds,"Every model needs a bounds entry");
 const int raw_x=layout->points[model.slot][0],raw_y=layout->points[model.slot][1],z=layout->zoom;
 // SceneContainer has a 960,540 pivot and camera offset. Retain the
 // calculator's 122px ground rail while preserving authored slot spacing.
 const int x=(960+(raw_x-960)*z/1000+layout->offset_x)/6;
 const int feet=122+(540+(raw_y-540)*z/1000+layout->offset_y-740)/6;
 const int body_width=bounds[unsigned(model.kind)][0],body_height=bounds[unsigned(model.kind)][1];
 int width=body_width?body_width*z/6000:enemy_pose(layout->count,model.slot).width;
 if(width<22)width=22;if(width>80)width=80;
 // Existing sprites share an 84px normalized body; the original Bounds
 // restores relative creature size (e.g. eggs versus their Ovicopter).
 int scale=body_height?body_height*z*1024/(6000*84):enemy_pose(layout->count,model.slot).scale;
 if(scale<160)scale=160;if(scale>1400)scale=1400;
 const int height=84*scale/1024,intent=feet-height-15;
 return {x,feet,width,scale,intent<25?25:intent,feet+7,feet+24};
}

}
