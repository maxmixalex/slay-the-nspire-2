#pragma once
#include <stdint.h>
#include "run_history_event.hpp"
namespace spire {
constexpr unsigned UiJournalEntries=4096,UiJournalOwners=33,UiJournalFloors=96;
struct UiHistoryJournalEntry {
 uint16_t model=0,detail=0;
 uint8_t owner=0,floor=0;
 RunHistoryEvent kind=RunHistoryEvent::None;
 uint8_t flags=0;
 uint32_t amount()const{return uint32_t(model)|(uint32_t(detail)<<16);}
};
struct UiHistoryJournalOwner {
 uint32_t serial=0,known[3]{},partial[3]{};
 bool observed(unsigned floor)const{return floor<UiJournalFloors&&(known[floor/32]&(uint32_t(1)<<(floor%32)));}
 bool incomplete(unsigned floor)const{return floor>=UiJournalFloors||!observed(floor)||(partial[floor/32]&(uint32_t(1)<<(floor%32)));}
 void observe(unsigned floor,bool complete){if(floor>=UiJournalFloors)return;const uint32_t bit=uint32_t(1)<<(floor%32);known[floor/32]|=bit;if(!complete)partial[floor/32]|=bit;}
 void lose(unsigned floor){if(floor<UiJournalFloors)partial[floor/32]|=uint32_t(1)<<(floor%32);}
};
struct UiHistoryJournal {
 UiHistoryJournalEntry entries[UiJournalEntries]{};
 UiHistoryJournalOwner owners[UiJournalOwners]{};
 uint16_t first=0,count=0;
 const UiHistoryJournalEntry&at(unsigned i)const{return entries[(unsigned(first)+i)%UiJournalEntries];}
 UiHistoryJournalEntry&at(unsigned i){return entries[(unsigned(first)+i)%UiJournalEntries];}
 int owner(uint32_t serial)const{if(!serial)return -1;for(unsigned i=0;i<UiJournalOwners;++i)if(owners[i].serial==serial)return int(i);return -1;}
 void compact(){
  unsigned kept=0;const unsigned old=count;
  for(unsigned i=0;i<old;++i){const auto entry=at(i);if(entry.kind==RunHistoryEvent::None||entry.owner>=UiJournalOwners||!owners[entry.owner].serial)continue;at(kept++)=entry;}
  count=uint16_t(kept);
 }
 void prune(const uint32_t*live,unsigned live_count){
  bool removed=false;
  for(unsigned i=0;i<UiJournalOwners;++i){auto&o=owners[i];bool retained=false;
   if(o.serial)for(unsigned j=0;j<live_count;++j)if(live[j]==o.serial){retained=true;break;}
   if(!retained&&o.serial){o={};removed=true;}
  }
  // Compact before reusing an owner index, so stale entries can never become
  // associated with a later run when the history ring wraps.
  if(removed)compact();
 }
 int bind(uint32_t serial,const uint32_t*live,unsigned live_count){
  if(!serial)return -1;if(const int existing=owner(serial);existing>=0)return existing;
  prune(live,live_count);for(unsigned i=0;i<UiJournalOwners;++i)if(!owners[i].serial){owners[i].serial=serial;return int(i);}return -1;
 }
 bool append(unsigned owner_index,unsigned floor,RunHistoryEvent kind,unsigned model=0,unsigned detail=0,unsigned flags=0){
  if(owner_index>=UiJournalOwners||!owners[owner_index].serial||floor>=UiJournalFloors||kind==RunHistoryEvent::None||kind>=RunHistoryEvent::Count)return false;
  if(count==UiJournalEntries){const auto&lost=at(0);if(lost.owner<UiJournalOwners&&owners[lost.owner].serial)owners[lost.owner].lose(lost.floor);first=uint16_t((first+1)%UiJournalEntries);--count;}
  at(count++)={uint16_t(model),uint16_t(detail),uint8_t(owner_index),uint8_t(floor),kind,uint8_t(flags)};return true;
 }
 bool amount(unsigned owner_index,unsigned floor,RunHistoryEvent kind,uint32_t value){
  if(!value||owner_index>=UiJournalOwners||!owners[owner_index].serial||floor>=UiJournalFloors)return false;
  // Aggregate only scalar observations on the same point. Cards and choices
  // keep their original order and multiplicity, including repeated purchases.
  for(unsigned i=count;i;--i){auto&e=at(i-1);if(e.owner!=owner_index||e.floor!=floor)break;if(e.kind!=kind)continue;
   const uint32_t old=e.amount();if(value>UINT32_MAX-old){value=UINT32_MAX-old;owners[owner_index].lose(floor);}
   const uint32_t total=old+value;e.model=uint16_t(total);e.detail=uint16_t(total>>16);return true;}
  return append(owner_index,floor,kind,value&65535u,value>>16);
 }
};
static_assert(sizeof(UiHistoryJournalEntry)==8,"History entries must remain compact");
static_assert(sizeof(UiHistoryJournal)==33696,"History journal memory budget changed");
// The hover reader is presentation-only. Its offset addresses observed rows,
// so scrolling never needs a second copy of the 4,096-entry journal.
struct UiHistoryTooltip {
 uint32_t serial=0;
 int16_t floor=-1,offset=0,maximum=0,x=0,y=0,w=0,h=0,node_x=0,node_y=0;
 uint8_t source=0;
 bool keyboard=false;
 bool contains(int px,int py)const{return w&&px>=x-6&&px<x+w+6&&py>=y-6&&py<y+h+6;}
 void bind(uint32_t run,unsigned point,unsigned origin){if(serial!=run||floor!=int(point)||source!=origin){serial=run;floor=int16_t(point);source=uint8_t(origin);offset=maximum=0;keyboard=false;}}
 void scroll(int rows){const int next=int(offset)+rows;offset=int16_t(next<0?0:next>maximum?maximum:next);}
};
static_assert(sizeof(UiHistoryTooltip)<=28,"History hover reader exceeds UI budget");
}
