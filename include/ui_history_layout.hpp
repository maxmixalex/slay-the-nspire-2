#pragma once
#include "ui_progress.hpp"
#include "room_ui7_presentation.hpp"
namespace spire {
// SerializableCard.Equals groups ID, upgrade and enchantment/amount. Other
// per-card saved properties still belong to the first actual member's preview.
inline unsigned ui_history_groups(const UiRunHistory&r,uint16_t*indices,uint16_t*counts=nullptr){
 unsigned n=0;uint16_t local[MaxDeck]{};if(!indices)indices=local;
 for(unsigned i=0;i<r.deck_count;++i){const auto&card=r.deck[i];unsigned at=0;
  for(;at<n;++at){const auto&other=r.deck[indices[at]];
   if(card.card.kind==other.card.kind&&card.card.upgraded==other.card.upgraded&&
      card.facts.enchantment==other.facts.enchantment&&card.facts.enchantment_amount==other.facts.enchantment_amount)break;
  }
  if(at==n){indices[n]=uint16_t(i);if(counts)counts[n]=1;++n;}else if(counts)++counts[at];
 }return n;
}
struct UiHistoryLayout {
 static constexpr int ViewTop=6,ViewHeight=216,Left=25,Width=270;
 static constexpr unsigned FloorColumns=18,RelicColumns=20,CardColumns=5;
 static constexpr int RelicFirst=256,CardFirst=512;
 struct Point{int x,y,w,h;};
 int map_y=54,relic_header=0,relic_y=0,deck_header=0,deck_y=0,height=0;
 unsigned groups=0;
 static unsigned floor_row(const UiRunHistory&r,unsigned index,unsigned*column=nullptr){
  unsigned row=0,col=0;
  for(unsigned i=0;i<index&&i<r.floor_count;++i){
   if(i+1<r.floor_count&&(r.floors[i+1].act!=r.floors[i].act||col+1==FloorColumns)){++row;col=0;}else ++col;
  }
  if(column)*column=col;return row;
 }
 explicit UiHistoryLayout(const UiRunHistory&r,unsigned count):groups(count){
  const unsigned rows=r.floor_count?floor_row(r,r.floor_count-1)+1:0;
  relic_header=map_y+int(rows>2?rows*16:32)+12;relic_y=relic_header+24;
  deck_header=relic_y+int((r.relic_count+RelicColumns-1)/RelicColumns)*13+12;deck_y=deck_header+24;
  height=deck_y+int((groups+CardColumns-1)/CardColumns)*13+12;
 }
 Point floor(const UiRunHistory&r,unsigned index)const{unsigned col=0;const unsigned row=floor_row(r,index,&col);return {60+int(col)*12,map_y+int(row)*16,12,15};}
 Point relic(unsigned index)const{return {37+int(index%RelicColumns)*13,relic_y+int(index/RelicColumns)*13,13,13};}
 Point card(unsigned index)const{return {37+int(index%CardColumns)*50,deck_y+int(index/CardColumns)*13,49,13};}
 bool point(const UiRunHistory&r,int id,Point&out)const{
  if(id>=CardFirst&&unsigned(id-CardFirst)<groups){out=card(unsigned(id-CardFirst));return true;}
  if(id>=RelicFirst&&id<CardFirst&&unsigned(id-RelicFirst)<r.relic_count){out=relic(unsigned(id-RelicFirst));return true;}
  if(id>=0&&id<RelicFirst&&unsigned(id)<r.floor_count){out=floor(r,unsigned(id));return true;}return false;
 }
 int first(const UiRunHistory&r)const{return r.floor_count?0:r.relic_count?RelicFirst:groups?CardFirst:-1;}
};
struct UiHistoryPresentation {
 struct Motion{uint32_t start=0;int16_t id=-1;uint16_t from=1000;bool focus=false;};
 Motion motions[8]{};int16_t focus=-1,hover=-1;uint32_t start=0;int8_t direction=0;
 void open(uint32_t now,int next=0){*this=UiHistoryPresentation{};start=now;direction=int8_t(next);}
 unsigned sample(const Motion&m,uint32_t now)const{
  const unsigned duration=m.focus?50:m.id<UiHistoryLayout::RelicFirst?1000:500;
  const unsigned q=m.focus&&m.id>=UiHistoryLayout::CardFirst?room_curve::linear(now-m.start,duration):room_curve::expo_out(now-m.start,duration);
  return unsigned(room_curve::interpolate(m.from,m.focus?1500:1000,q));
 }
 void animate(int id,bool selected,uint32_t now){
  if(id<0)return;Motion*slot=nullptr;
  for(auto&m:motions)if(m.id==id){slot=&m;break;}
  if(!slot)for(auto&m:motions)if(m.id<0||(!m.focus&&now-m.start>=1000)){slot=&m;break;}
  if(!slot){slot=&motions[0];for(auto&m:motions)if(now-m.start>now-slot->start)slot=&m;}
  const unsigned from=slot->id==id?sample(*slot,now):1000;*slot={now,int16_t(id),uint16_t(from),selected};
 }
 void observe(int id,uint32_t now){if(id==hover)return;animate(hover,false,now);hover=int16_t(id);animate(id,true,now);}
 unsigned scale(int id,uint32_t now)const{for(const auto&m:motions)if(m.id==id)return sample(m,now);return 1000;}
 unsigned alpha(uint32_t now)const{return 255*room_curve::linear(now-start,400)/room_curve::Q;}
 int offset(uint32_t now)const{return direction*190*int(room_curve::Q-room_curve::expo_out(now-start,500))/int(room_curve::Q);}
};
static_assert(sizeof(UiHistoryPresentation)<=128,"History interaction stays bounded");
}
