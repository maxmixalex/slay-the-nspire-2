#pragma once
namespace spire {
// MerchantInventory's 1747x978 SlotsContainer, fitted as one intact rug.
// Child centers retain their source coordinates; .65/.8 are NMerchantSlot's
// normal/hover scales, rather than enlarged independent calculator columns.
struct MerchantInventoryLayout {
 static constexpr int RugX=0,RugY=41,RugW=320,RugH=180;
 static constexpr int CardW=36,CardH=50,FocusW=44,FocusH=62;
 static int card_x(unsigned i){static const int centers[]={58,107,154,202,251,70,120};return centers[i<7?i:0];}
 static int card_y(unsigned i){return i<5?96:165;}
 static int item_x(unsigned i){return 160+int(i)*27;}
 static constexpr int RelicY=150,PotionY=177,RemoveX=251,RemoveY=165;
};
}
