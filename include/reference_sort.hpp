#pragma once

namespace spire::reference {
namespace sort_detail {
template<class T> void swap(T* values,unsigned a,unsigned b){T value=values[a];values[a]=values[b];values[b]=value;}
template<class T,class Key> void order_pair(T* values,unsigned a,unsigned b,Key key){if(key(values[a])>key(values[b]))swap(values,a,b);}
template<class T,class Key> void heap_down(T* values,unsigned at,unsigned count,Key key){
 T value=values[at-1];while(at<=count/2){unsigned child=at*2;
  if(child<count&&key(values[child-1])<key(values[child]))++child;
  if(key(value)>=key(values[child-1]))break;values[at-1]=values[child-1];at=child;
 }values[at-1]=value;
}
template<class T,class Key> void partition_sort(T* values,unsigned count,unsigned depth,Key key){
 while(count>1){
  if(count<=16){
   if(count==2)order_pair(values,0,1,key);
   else if(count==3){order_pair(values,0,1,key);order_pair(values,0,2,key);order_pair(values,1,2,key);}
   else for(unsigned i=1;i<count;i++){T value=values[i];unsigned j=i;
    while(j&&key(value)<key(values[j-1])){values[j]=values[j-1];--j;}values[j]=value;
   }return;
  }
  if(!depth){for(unsigned i=count/2;i>0;--i)heap_down(values,i,count,key);
   for(unsigned end=count;end>1;--end){swap(values,0,end-1);heap_down(values,1,end-1,key);}return;
  }
  --depth;const unsigned last=count-1,middle=last/2;
  order_pair(values,0,middle,key);order_pair(values,0,last,key);order_pair(values,middle,last,key);
  const auto pivot=key(values[middle]);swap(values,middle,last-1);
  unsigned left=0,right=last-1;
  while(left<right){do{++left;}while(key(values[left])<pivot);do{--right;}while(key(values[right])>pivot);
   if(left>=right)break;swap(values,left,right);
  }
  if(left!=last-1)swap(values,left,last-1);
  partition_sort(values+left+1,count-left-1,depth,key);count=left;
 }
}
}
// Scalar-key comparison sort matching the locked runtime's small-partition,
// pivot and equal-key rules. Deck transformation and reward order use List.Sort
// rather than the stable model-key ordering used before StableShuffle.
// Bounded callers supply their own storage; this implementation allocates none.
template<class T,class Key> void comparison_sort(T* values,unsigned count,Key key){
 unsigned depth=0;for(unsigned n=count;n;n>>=1)++depth;
 sort_detail::partition_sort(values,count,depth*2,key);
}
}
