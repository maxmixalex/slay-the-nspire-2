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
}
