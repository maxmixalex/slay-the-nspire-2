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
}
