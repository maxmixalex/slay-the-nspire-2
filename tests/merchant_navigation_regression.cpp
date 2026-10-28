#include "merchant_navigation.hpp"
#include <cstdio>
#include <cstdlib>

using namespace spire;
using namespace spire::merchant_navigation;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"merchant navigation: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)

static void original_rows(){
 // Independently transcribed neighbor assignments from NMerchantInventory's
 // top/bottom/middle pass order, with every slot stocked. Left/right edges
 // point to themselves; the calculator Back control is outside this graph.
 constexpr int ordinary[14][4]={
  {0,1,0,5},{0,2,1,6},{1,3,2,7},{2,4,3,8},{3,4,4,9},
  {5,6,0,5},{5,7,1,6},{6,8,2,10},{7,9,3,11},{8,13,4,12},
  {6,11,7,10},{10,12,8,11},{11,13,9,12},{9,13,4,13}};
 constexpr int fake[6][4]={
  {0,1,0,2},{0,1,1,3},{2,3,0,5},{2,4,1,5},{3,4,1,5},{5,5,2,5}};
 for(int slot=0;slot<14;++slot)for(unsigned d=0;d<4;++d)CHECK(move(slot,Direction(d),0x3fff,false)==ordinary[slot][d]);
 for(int slot=0;slot<6;++slot)for(unsigned d=0;d<4;++d)CHECK(move(slot,Direction(d),0x3f,true)==fake[slot][d]);
 // Sold controls are removed from horizontal links, but the ordinary
 // vertical index remains the original row index and searches right first.
 CHECK(move(2,Down,0x3fff&~(1u<<7),false)==8);
 CHECK(move(0,Right,0x3fff&~(3u<<1),false)==3);
 const unsigned no_relics=0x3fff&~(7u<<7);
 CHECK(move(6,Right,no_relics,false)==10);CHECK(move(13,Left,no_relics,false)==12);
 const unsigned no_middle=0x3fff&~((31u<<5)|(1u<<13));
 CHECK(move(0,Down,no_middle,false)==10);CHECK(move(4,Down,no_middle,false)==12);
 CHECK(move(10,Up,no_middle,false)==2);CHECK(move(12,Up,no_middle,false)==4);
 CHECK(move(9,Right,0x3fff&~(1u<<13),false)==9);
 // Fake merchant filters each of its2/3/1 rows, then removes empty rows.
 CHECK(move(2,Up,0x3c,true)==2);CHECK(move(4,Down,0x3c,true)==5);CHECK(move(5,Up,0x3c,true)==2);
 CHECK(move(1,Down,0x23,true)==5);CHECK(move(5,Up,0x23,true)==0);
 CHECK(move(4,Up,0x32,true)==1);CHECK(move(1,Down,0x32,true)==4);
 CHECK(move(0,Left,0,false)==14);CHECK(move(0,Right,0,true)==6);
 CHECK(move(14,Up,1u<<3,false)==3);CHECK(move(6,Down,1u<<5,true)==5);
}
static void purchase_focus(){
 // NMerchantInventory picks nearest remaining GlobalPosition, not the
 // center of its visual hitbox. The purchased slot is excluded even when
 // The Courier immediately replenishes it.
 CHECK(after_purchase(0x3fff,0,false)==1);CHECK(after_purchase(0x3f,0,true)==1);
 CHECK(after_purchase(0x3fff,7,false)==10);CHECK(after_purchase(0x3fff,10,false)==7);
 CHECK(after_purchase((1u<<2)|(1u<<3),0,true)==2);
 CHECK(after_purchase(1u<<0,0,false)==14);CHECK(after_purchase(1u<<0,0,true)==6);
 CHECK(first(0,14)==14&&first(0,6)==6&&first(1u<<9,14)==9);
 static State state{};state.shop_sold=uint16_t((1u<<0)|(1u<<8)|(1u<<13));
 CHECK(stock(state,false)==(0x3fff&~((1u<<0)|(1u<<8)|(1u<<13))));
 state.original_events.offer_count=6;for(unsigned i=0;i<6;++i)state.original_events.offers[i]=NoWorldRelic;
 state.original_events.offers[2]=WorldRelicVajra;state.original_events.offers[5]=WorldRelicGoldenPearl;
 CHECK(stock(state,true)==0x24);state.original_events.offer_count=3;CHECK(stock(state,true)==4);
}
static void source_geometry(){
 // Original122px slots live on the fixed1680x1260 canvas. Large NRelic
 // artwork is240px after its holder transform, translated upward by the
 // measured CostLabel height:49.5625px EN /58.5px ZHS in Godot4.5.1.
 constexpr int x[]={166,227,137,197,272,156},y[]={66,82,132,147,128,206};
 for(unsigned i=0;i<6;++i){CHECK(MerchantInventoryLayout::fake_x(i)==x[i]);CHECK(MerchantInventoryLayout::fake_y(i)==y[i]);}
 for(unsigned chinese=0;chinese<2;++chinese){
  auto rest=MerchantInventoryLayout::relic_bounds(166,66,650,true,chinese);
  CHECK(rest.x==158&&rest.y==(chinese?39:41)&&rest.w==35&&rest.h==35);
  auto focus=MerchantInventoryLayout::relic_bounds(166,66,800,true,chinese);
  CHECK(focus.x==156&&focus.y==(chinese?32:36)&&focus.w==43&&focus.h==43);
 }
 auto normal=MerchantInventoryLayout::relic_bounds(166,150,650);
 CHECK(normal.x==158&&normal.y==140&&normal.w==17&&normal.h==17);
 normal=MerchantInventoryLayout::relic_bounds(166,150,800);
 CHECK(normal.x==156&&normal.y==137&&normal.w==21&&normal.h==21);
}
int main(){original_rows();purchase_focus();source_geometry();std::printf("merchant navigation: %u checks passed; original stocked-row focus, purchase successor and large-relic transforms\n",checks);}
