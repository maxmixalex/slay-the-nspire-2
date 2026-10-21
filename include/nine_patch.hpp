#pragma once

namespace spire::nine_patch {
// Preserve end caps when stretching a component. If a readable hit target is
// narrower than both caps, scale those caps proportionally, never index past
// the texture or divide by a zero-width authored center.
inline int coordinate(int at,int extent,int source,int before,int after){
    if(extent<=0||source<=0)return 0;
    if(before<0)before=0;if(after<0)after=0;
    if(before>source)before=source;if(after>source-before)after=source-before;
    if(at<0)at=0;if(at>=extent)at=extent-1;
    int left=before,right=after;
    if(left+right>extent){left=extent*before/(before+after);right=extent-left;}
    if(at<left)return at*before/left;
    if(at>=extent-right)return source-after+(at-(extent-right))*after/right;
    const int middle=source-before-after;
    if(middle<=0)return before>0?before-1:0;
    return before+(at-left)*middle/(extent-left-right);
}
}
