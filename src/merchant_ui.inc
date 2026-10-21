// Five class cards, two colorless, three relics/potions and a right-hand removal service.
static const char* relic_name(RelicMask r){auto*d=relic_definition(r);return d?d->name:"售罄";}
static const char* relic_desc(RelicMask r){auto*d=relic_definition(r);return d?d->description:"没有剩余的已实现遗物。";}
static const char* potion_name(int p,const Content*content=nullptr){auto*d=potion_definition((Potion)p);if(d&&content){const char*name=content->world_title("potions",d->key);if(*name)return name;}const char*name=d?d->name:"空药水槽";return content?content->ui_text(name):name;}
static const char* potion_desc(int p){auto*d=potion_definition((Potion)p);return d?d->description:"药水栏为空。可在商店购买或领取战斗掉落。";}
static void ui_potion_description(const App&a,int p,char*out,size_t size){
 if(!size)return;out[0]=0;if(const auto*d=potion_definition((Potion)p)){char key[96]{};TextBuffer b{key,sizeof key,0};b.text(d->key);b.text(".description");
  if(*a.content.world_string("potions",key)){a.localized_text("potions",key,out,size);return;}}
 TextBuffer b{out,size,0,&a.content};b.text(potion_desc(p));
}
static void merchant_price(Canvas&c,int item,int x,int y,int w){
    auto&s=c.a.game;bool sold=s.shop_sold&(1u<<item);int price=s.world.active?world_shop_price(s,item):item==ShopRemove?remove_price(s):s.shop_prices[item];
    if(sold){c.centered("售罄",x,y,w,10,Muted);return;}
    char value[24]{};TextBuffer t{value,sizeof value,0,&c.a.content};t.number(price);int textw=c.width(value,10),left=x+mx(0,(w-textw-13)/2);
    const bool sale=s.world.active&&item<ShopCardCount&&s.world.shop_sale==item;
    c.picture(A_Coin,left,y+2,10,10);c.text(value,left+12,y,10,s.gold>=price?(sale?Green:Gold):Red);
}
static void merchant_card(Canvas&c,Card card,int x,int y,bool selected,bool sold,const EventCardFacts*facts=nullptr){
    if(selected)c.box(x-2,y-2,49,66,Gold);
    draw_card_face(c,card,x,y,45,63,-2,-2,-1,-1,facts);
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
        merchant_card(c,s.world.active?s.world.shop_cards[i]:Card{s.shop_cards[i],0},x,y,a.cursor==i,sold,s.world.active?&s.world.shop_facts[i]:nullptr);merchant_price(c,i,x-1,y+61,48);
        if(s.world.active&&s.world.shop_sale==i&&!sold){int sale=a.content.world_image("merchant","SALE_TAG");if(sale>=0)c.picture(sale,x+27,y-7,24,19);}
        c.hit(x-2,y-3,49,78,HitKind::Choice,i,!sold);
    }
    for(int i=0;i<3;i++){
        int x=141+i*37,item=ShopRelicStart+i,index=-1;bool sold=s.shop_sold&(1u<<item);
        if(s.world.active)index=s.world.shop_relics[i]==NoWorldRelic?-1:s.world.shop_relics[i];else for(int r=0;r<RelicKinds;r++)if(relic_definitions[r].id==s.shop_relics[i])index=r;
        if(a.cursor==item)c.box(x-3,139,33,38,Gold);
        if(index>=0)c.picture(a.relic_face_image(index),x,140,26,26,sold?70:255);
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
    const char*name=a.cursor<7?a.content.card_string(s.world.active?s.world.shop_cards[a.cursor].kind:s.shop_cards[a.cursor]):a.cursor<10?(s.world.active?a.relic_title(s.world.shop_relics[a.cursor-7]):relic_name(s.shop_relics[a.cursor-7])):a.cursor<13?potion_name(s.shop_potions[a.cursor-10],&a.content):a.cursor==13?"移除一张牌":"返回商人";
    c.dim(14,43,246,18,180);c.centered(name,16,43,240,10,Gold);
    c.text("详情 I",265,44,10,Gold);c.hit(260,41,59,20,HitKind::Command,(int)Key::Detail,a.cursor<13);
    c.button("返回",3,218,80,a.cursor==ShopLeave,22);c.hit(0,217,90,23,HitKind::Choice,ShopLeave);
    c.centered("Enter 购买 · I 详情",96,223,214,10,Muted);
    // Only an actually hovered icon can show a tooltip. Moving off it clears it.
    if(a.pointer.visible&&!a.pointer.held&&a.cursor>=7&&a.cursor<13){
        int x=141+(a.cursor>=10?a.cursor-10:a.cursor-7)*37,y=a.cursor>=10?178:137;
        if(a.pointer.x>=x-4&&a.pointer.x<x+32&&a.pointer.y>=y&&a.pointer.y<y+39){
            char description[2048]{};if(a.cursor<10&&s.world.active)a.relic_description(s.world.shop_relics[a.cursor-7],description,sizeof description);
            if(a.cursor>=10)ui_potion_description(a,s.shop_potions[a.cursor-10],description,sizeof description);
            const char*desc=a.cursor<10?(s.world.active?description:relic_desc(s.shop_relics[a.cursor-7])):description;
            c.ui(U_RewardPanel,11,64,297,67);c.text(name,23,69,12,Gold,269);c.paragraph(desc,23,89,269,2,0,10);
        }
    }
}
