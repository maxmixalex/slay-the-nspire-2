#pragma once
namespace spire {
// merchant_inventory.tscn uses the original 1680x1260 4:3 canvas. This screen
// retains the source rug and slot arrangement at2/9. LCD cards deliberately
// fit their60px columns without covering the neighboring title or cost. The
// colorless row retains the rug's authored384/658 and676 source positions;
// both its cards and their Cost children fit above the fixed Back control. A
// focused card draws last and may grow over its neighboring stock.
struct MerchantInventoryLayout {
 static constexpr int project(int source){return source>=0?(source*2+4)/9:-((-source*2+4)/9);}
 static constexpr int RugX=-27,RugY=18,RugW=388,RugH=217;
 static constexpr int CardW=54,CardH=76,FocusW=66,FocusH=94;
 static int card_x(unsigned i){static const int centers[]={39,99,159,219,279,58,119};return centers[i<7?i:0];}
 static int card_y(unsigned i){return i<5?73:RugY+project(676);}
 static unsigned card_scale(unsigned scale){return (scale*81+32)/65;}
 static int item_x(unsigned i){return 174+int(i)*35;}
 struct Point {int x,y;};
 static Point source_position(unsigned item,bool fake=false){
  if(fake){static constexpr Point points[]={{869,218},{1144,288},{737,516},{1010,583},{1345,498},{823,847}};return points[item<6?item:0];}
  if(item<7){static constexpr int x[]={319,582,843,1102,1368,384,658};return {x[item],item<5?(item==4?300:303):676};}
  if(item<13)return {871+int((item-7)%3)*150,item<10?595:739};
  return {1370,678};
 }
 static int fake_x(unsigned item){return project(-122+source_position(item,true).x);}
 static int fake_y(unsigned item){return project(80+source_position(item,true).y);}
 static constexpr int RelicY=151,PotionY=187,RemoveX=291,RemoveY=172;
 static constexpr int CardHitW=57,CardHitH=90,RelicHitY=135,RelicHitH=36,PotionHitY=176,PotionHitH=36;
 struct Bounds {int x,y,w,h;};
 static Bounds bounds(unsigned item,int dy=0){
  if(item<7)return {card_x(item)-CardHitW/2,card_y(item)-CardH/2+dy,CardHitW,CardHitH};
  if(item<10)return {item_x(item-7)-16,RelicHitY+dy,32,RelicHitH};
  if(item<13)return {item_x(item-10)-16,PotionHitY+dy,32,PotionHitH};
  return {RemoveX-26,RemoveY-30+dy,52,68};
 }
 static int size(int source,unsigned slot_scale){return (source*int(slot_scale)*2+4500)/9000;}
 static int offset(int source,unsigned slot_scale){return source<0?-size(-source,slot_scale):size(source,slot_scale);}
 static int eighth_offset(int eighths,unsigned slot_scale){
  const int value=eighths*int(slot_scale)*2;return (value+(value<0?-36000:36000))/72000;
 }
 static Bounds relic_bounds(int x,int y,unsigned scale,bool large=false,bool chinese=false){
  // NRelic's4px margins leave60px for ordinary icons and120px after the
  // Large128px resize. RelicHolder applies its own2x transform in both cases.
  // Large also subtracts CostLabel.Size.Y before the first container sort:
  // pinned Godot4.5.1 + exported fonts give49.5625 EN /58.5 ZHS pixels.
  const int top=large?eighth_offset(chinese?-1512:-1369,scale):offset(-72,scale);
  const int side=size(large?240:120,scale);
  return {x+offset(-56,scale),y+top,side,side};
 }
};
}
