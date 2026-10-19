// Five class cards, two colorless, three relics/potions and a right-hand removal service.
static const char* relic_name(RelicMask r){auto*d=relic_definition(r);return d?d->name:"售罄";}
static const char* relic_desc(RelicMask r){auto*d=relic_definition(r);return d?d->description:"没有剩余的已实现遗物。";}
static const char* potion_name(int p){auto*d=potion_definition((Potion)p);return d?d->name:"空药水槽";}
static const char* potion_desc(int p){auto*d=potion_definition((Potion)p);return d?d->description:"药水栏为空。可在商店购买或领取战斗掉落。";}
static void merchant_price(Canvas&c,int item,int x,int y,int w){
    auto&s=c.a.game;bool sold=s.shop_sold&(1u<<item);int price=item==ShopRemove?remove_price(s):s.shop_prices[item];
    if(sold){c.centered("售罄",x,y,w,10,Muted);return;}
    char value[24]{};TextBuffer t{value,sizeof value,0};t.number(price);int textw=c.width(value,10),left=x+mx(0,(w-textw-13)/2);
    c.picture(A_Coin,left,y+2,10,10);c.text(value,left+12,y,10,s.gold>=price?Gold:Red);
}
static void merchant_card(Canvas&c,Card card,int x,int y,bool selected,bool sold){
    if(selected)c.box(x-2,y-2,49,66,Gold);
    c.picture(c.a.content.frame_image(card),x,y,45,63);
    c.picture(c.a.content.card_image(card.kind),x+3,y+15,39,29);
    c.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,x,y,45,12);
    c.text(c.a.content.card_string(card.kind),x+8,y+1,8,Ink,35);
    c.text(c.a.content.card_string(card.kind),x+7,y,8,White,36);
    c.picture(c.a.content.energy_image(card),x-6,y-6,14,14);c.energy_label(cost(card),x-2,y-7,10);
    c.centered(definitions[card.kind].type==CardType::Attack?"攻击":definitions[card.kind].type==CardType::Skill?"技能":"能力",x+3,y+46,39,10,Muted);
    if(sold)c.dim(x,y,45,63,170);
}
static void shop_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.picture(A_ShopBackground,0,22,320,218);
    if(!a.shop_open){
        c.actor(0,54,170);c.clip(AnimMerchantTop,181,62,a.animation*33,false);c.clip(AnimMerchantBottom,152,132,a.animation*33,false);
        c.centered("商人",169,45,139,16,Gold);
        c.button("查看商品",160,178,146,a.cursor==0,28);c.action(151,48,166,158,UiAction::OpenShop);
        c.ui(U_RewardSkip,211,210,105,27);c.centered("前进 ›",211,214,105,12,a.cursor==1?Gold:White);c.action(206,207,112,31,UiAction::LeaveShop);
        c.text("Enter 交谈",8,219,10,Muted);return;
    }
    c.dim(0,22,320,218,150);c.ui(U_MerchantRug,0,41,320,180);
    a.cursor=mn(mx(a.cursor,0),ShopLeave);
    for(int i=0;i<ShopCardCount;i++){
        int x=i<5?22+i*55:22+(i-5)*55,y=i<5?63:142;bool sold=s.shop_sold&(1u<<i);
        merchant_card(c,{s.shop_cards[i],0},x,y,a.cursor==i,sold);merchant_price(c,i,x-1,y+61,48);
        c.hit(x-2,y-3,49,78,HitKind::Choice,i,!sold);
    }
    for(int i=0;i<3;i++){
        int x=141+i*37,item=ShopRelicStart+i,index=-1;bool sold=s.shop_sold&(1u<<item);
        for(int r=0;r<RelicKinds;r++)if(relic_definitions[r].id==s.shop_relics[i])index=r;
        if(a.cursor==item)c.box(x-3,139,33,38,Gold);
        if(index>=0)c.picture(a.content.relic_image(index),x,140,26,26,sold?70:255);
        merchant_price(c,item,x-4,164,36);c.hit(x-4,137,36,41,HitKind::Choice,item,!sold);
        item=ShopPotionStart+i;sold=s.shop_sold&(1u<<item);
        if(a.cursor==item)c.box(x-3,179,33,38,Gold);
        c.picture(a.content.potion_image((Potion)s.shop_potions[i]),x+3,179,21,27,sold?70:255);
        merchant_price(c,item,x-4,203,36);c.hit(x-4,178,36,39,HitKind::Choice,item,!sold);
    }
    bool removed=s.shop_sold&(1u<<ShopRemove);
    c.ui(removed?U_ShopRemoveUsed:U_ShopRemove,261,149,51,51,removed?140:255);
    if(a.cursor==ShopRemove)c.box(259,146,55,68,Gold);
    merchant_price(c,ShopRemove,259,201,55);c.hit(259,144,57,71,HitKind::Choice,ShopRemove,!removed);
    const char*name=a.cursor<7?a.content.card_string(s.shop_cards[a.cursor]):a.cursor<10?relic_name(s.shop_relics[a.cursor-7]):a.cursor<13?potion_name(s.shop_potions[a.cursor-10]):a.cursor==13?"移除一张牌":"返回商人";
    c.dim(14,43,246,18,180);c.centered(name,16,43,240,10,Gold);
    c.text("详情 I",265,44,10,Gold);c.hit(260,41,59,20,HitKind::Command,(int)Key::Detail,a.cursor<13);
    c.button("返回",3,218,80,a.cursor==ShopLeave,22);c.hit(0,217,90,23,HitKind::Choice,ShopLeave);
    c.centered("Enter 购买 · I 详情",96,223,214,10,Muted);
    // Only an actually hovered icon can show a tooltip. Moving off it clears it.
    if(a.pointer.visible&&!a.pointer.held&&a.cursor>=7&&a.cursor<13){
        int x=141+(a.cursor>=10?a.cursor-10:a.cursor-7)*37,y=a.cursor>=10?178:137;
        if(a.pointer.x>=x-4&&a.pointer.x<x+32&&a.pointer.y>=y&&a.pointer.y<y+39){
            const char*desc=a.cursor<10?relic_desc(s.shop_relics[a.cursor-7]):potion_desc(s.shop_potions[a.cursor-10]);
            c.ui(U_RewardPanel,11,64,297,67);c.text(name,23,69,12,Gold,269);c.paragraph(desc,23,89,269,2,0,10);
        }
    }
}
