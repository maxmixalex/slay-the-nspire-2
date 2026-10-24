#pragma once
namespace spire {
// merchant_inventory.tscn in the 4:3 1440x1080 logical viewport. The rug,
// its children and their .65/.8 slot scales share the same 2/9 projection.
// The original centered rug extends beyond the viewport; fitting its entire
// 1747px width into 320px incorrectly applies a second scale to every item.
struct MerchantInventoryLayout {
 static constexpr int project(int source){return source>=0?(source*2+4)/9:-((-source*2+4)/9);}
 static constexpr int RugX=-27,RugY=18,RugW=388,RugH=217;
 static constexpr int CardW=43,CardH=61,FocusW=53,FocusH=75;
 static int card_x(unsigned i){static const int centers[]={319,582,843,1102,1368,384,658};return project(-122+centers[i<7?i:0]);}
 static int card_y(unsigned i){return project(80+(i<5?(i==4?300:303):676));}
 static int item_x(unsigned i){return project(-122+871+int(i)*150);}
 static constexpr int RelicY=150,PotionY=182,RemoveX=277,RemoveY=168;
 static constexpr int CardHitW=54,CardHitH=85,RelicHitY=134,RelicHitH=36,PotionHitY=172,PotionHitH=36;
 struct Bounds {int x,y,w,h;};
 static Bounds bounds(unsigned item,int dy=0){
  if(item<7)return {card_x(item)-CardHitW/2,card_y(item)-FocusH/2+dy,CardHitW,CardHitH};
  if(item<10)return {item_x(item-7)-16,RelicHitY+dy,32,RelicHitH};
  if(item<13)return {item_x(item-10)-16,PotionHitY+dy,32,PotionHitH};
  return {RemoveX-26,RemoveY-30+dy,52,68};
 }
 static int size(int source,unsigned slot_scale){return (source*int(slot_scale)*2+4500)/9000;}
};
}
