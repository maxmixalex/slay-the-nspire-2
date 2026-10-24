#pragma once
namespace spire {
// merchant_inventory.tscn uses the original 1680x1260 4:3 canvas. This screen
// retains a deliberate 7/6 readability zoom around the top-center (160,0):
// (4/21)*(7/6)=2/9. Apply it equally to the rug and every child/slot scale;
// fitting the 1747px rug into 320px and separately enlarging items is wrong.
// The uniform zoom yields 43x61 cards without changing the card/rug ratio.
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
