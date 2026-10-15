#pragma once
namespace spire {
inline int potion_slot_x(int slot,int count){return count>2?130+slot*20:163+slot*23;}
inline int potion_popup_x(int slot,int count){int x=potion_slot_x(slot,count)-35;return x<5?5:x>223?223:x;}
}
