#pragma once
#include "core.hpp"
#include "merchant_layout.hpp"

namespace spire {
// Source NMerchantInventory/NFakeMerchantInventory focus graphs. Affordability
// does not remove a stocked item: selecting it still plays purchase failure.
namespace merchant_navigation {
enum Direction {Left,Right,Up,Down};
inline unsigned stock(const State&s,bool fake){
 if(!fake)return unsigned(~s.shop_sold)&0x3fffu;
 unsigned mask=0;for(unsigned i=0;i<6&&i<s.original_events.offer_count;++i)
  if(s.original_events.offers[i]<WorldRelicKinds)mask|=1u<<i;return mask;
}
inline int first(unsigned mask,int back){for(int i=0;i<back;++i)if(mask&(1u<<i))return i;return back;}
inline int closest(const unsigned*row,unsigned count,unsigned index,unsigned mask){
 if(!count)return -1;index=index<count?index:count-1;
 if(mask&(1u<<row[index]))return int(row[index]);
 // GetClosestStockedSlot checks the right neighbor before the left at each
 // distance; this is row-index navigation, not geometric nearest distance.
 for(unsigned d=1;d<count;++d){if(index+d<count&&(mask&(1u<<row[index+d])))return int(row[index+d]);
  if(d<=index&&(mask&(1u<<row[index-d])))return int(row[index-d]);}return -1;
}
inline int horizontal(const unsigned*row,unsigned count,int current,bool right,unsigned mask){
 int previous=-1;for(unsigned i=0;i<count;++i)if(mask&(1u<<row[i])){
  if(int(row[i])==current){if(!right)return previous>=0?previous:current;
   for(unsigned j=i+1;j<count;++j)if(mask&(1u<<row[j]))return int(row[j]);return current;}
  previous=int(row[i]);}return current;
}
inline int move(int current,Direction direction,unsigned mask,bool fake){
 const int back=fake?6:14;if(current<0||current>=back||!(mask&(1u<<current)))return first(mask,back);
 if(fake){
  static constexpr unsigned source[3][3]={{0,1,6},{2,3,4},{5,6,6}};
  unsigned rows[3][3]{},counts[3]{},used=0,selected_row=0,selected_column=0;
  for(unsigned r=0;r<3;++r){unsigned count=0;for(unsigned c=0;c<3;++c){const unsigned item=source[r][c];
    if(item<6&&(mask&(1u<<item))){if(int(item)==current){selected_row=used;selected_column=count;}rows[used][count++]=item;}}
   if(count)counts[used++]=count;
  }
  if(direction==Left)return int(rows[selected_row][selected_column?selected_column-1:0]);
  if(direction==Right)return int(rows[selected_row][selected_column+1<counts[selected_row]?selected_column+1:selected_column]);
  const unsigned next=direction==Up?(selected_row?selected_row-1:0):(selected_row+1<used?selected_row+1:selected_row);
  return int(rows[next][selected_column<counts[next]?selected_column:counts[next]-1]);
 }
 static constexpr unsigned top[]={0,1,2,3,4},middle[]={5,6,7,8,9,13},bottom[]={5,6,10,11,12,13};
 if(direction==Left||direction==Right){
  const bool right=direction==Right;
  if(current<5)return horizontal(top,5,current,right,mask);
  if(current>=10&&current<13)return horizontal(bottom,6,current,right,mask);
  // Shared colorless/removal controls get the middle row's links last.
  // The source explicitly connects them to potions after all relics sell.
  if(!(mask&(7u<<7))&&(mask&(7u<<10)))return horizontal(bottom,6,current,right,mask);
  return horizontal(middle,6,current,right,mask);
 }
 int result=-1;
 if(current<5){if(direction==Up)return current;
  result=closest(middle,6,unsigned(current),mask);if(result<0)result=closest(bottom,6,unsigned(current),mask);
 }else if(current>=10&&current<13){if(direction==Down)return current;
  result=closest(middle,6,unsigned(current-8),mask);if(result<0)result=closest(top,5,unsigned(current-8),mask);
 }else{
  const unsigned index=current==13?5u:unsigned(current-5);
  result=direction==Up?closest(top,5,index,mask):closest(bottom,6,index,mask);
 }
 return result>=0?result:current;
}
inline int after_purchase(unsigned mask,unsigned purchased,bool fake){
 const unsigned count=fake?6u:14u;const auto from=MerchantInventoryLayout::source_position(purchased,fake);
 int selected=int(count),distance=0x7fffffff;
 for(unsigned i=0;i<count;++i)if(i!=purchased&&(mask&(1u<<i))){const auto to=MerchantInventoryLayout::source_position(i,fake);
  const int dx=to.x-from.x,dy=to.y-from.y,value=dx*dx+dy*dy;if(value<distance){distance=value;selected=int(i);}}
 return selected;
}
}
}
