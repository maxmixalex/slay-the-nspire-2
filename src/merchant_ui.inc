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
    if(sold)return;
    char value[24]{};TextBuffer t{value,sizeof value,0,&c.a.content};t.number(price);int textw=c.width(value,8),left=x+mx(0,(w-textw-10)/2);
    const bool sale=s.world.active&&item<ShopCardCount&&s.world.shop_sale==item;
    c.picture(A_Coin,left,y+2,7,7);c.text(value,left+9,y,8,s.gold>=price?(sale?Green:White):Red);
}
static bool merchant_hovered(const App&a,int item,int x,int y,int w,int h){
    if(a.paused||a.help||a.panel||a.library||a.settings_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0)return false;
    return a.pointer.visible?a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h:a.cursor==item;
}
static void merchant_card(Canvas&c,Card card,int cx,int cy,bool selected,bool sold,const EventCardFacts*facts=nullptr){
    if(sold)return;
    const int w=selected?MerchantInventoryLayout::FocusW:MerchantInventoryLayout::CardW,h=selected?MerchantInventoryLayout::FocusH:MerchantInventoryLayout::CardH;
    draw_card_face(c,card,cx-w/2,cy-h/2,w,h,-2,-2,-1,-1,facts);
}
static void merchant_idle(Canvas&c,int clip){
    const auto sequence=c.a.content.clip(clip);if(!sequence.count)return;
    // Both skins sample the same authored six-second idle phase. The v18
    // directory stores rounded frame_ms, so select by the original duration.
    const unsigned frame=(c.a.map_clock_ms%MerchantIdleDurationMs)*sequence.count/MerchantIdleDurationMs;
    c.clip(clip,0,22,frame*sequence.frame_ms);
}
static void shop_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;const bool registered=a.content.ui_image(U_MerchantBackdrop)>=0&&a.content.clip(AnimMerchantRoomTop).count;
    const int body_x=registered?MerchantLayout::BodyX:181,body_y=registered?MerchantLayout::BodyY:62;
    const int body_w=registered?MerchantLayout::BodyW:105,body_h=registered?MerchantLayout::BodyH:109;
    const bool active=!a.shop_open&&!a.paused&&!a.help&&!a.panel&&!a.library&&!a.settings_open&&!a.frontend_confirm&&
        a.detail_card<0&&a.detail_item<0&&a.status_actor<0;
    const bool focused=active&&(a.pointer.visible?
        a.pointer.x>=body_x-2&&a.pointer.x<body_x+body_w+2&&a.pointer.y>=body_y-2&&a.pointer.y<body_y+body_h+2:a.cursor==0);
    const bool effects=registered&&a.content.version()>=MerchantEffectsContentVersion;
    if(registered){
        c.ui(U_MerchantBackdrop,0,22,320,218);
        if(effects){
            c.clip(AnimMerchantFlamesBehind,0,22,a.map_clock_ms%MerchantIdleDurationMs);
            merchant_idle(c,AnimMerchantRoomBottom);
            c.clip(AnimMerchantFlamesFront,0,22,a.map_clock_ms%MerchantIdleDurationMs);
        }else c.clip(AnimMerchantRoomBottom,0,22,a.map_clock_ms);
        c.clip(AnimMerchantIronclad+int(s.character),0,22,a.map_clock_ms);
        if(effects)merchant_idle(c,focused?AnimMerchantRoomFocused:AnimMerchantRoomTop);
        else c.clip(AnimMerchantRoomTop,0,22,a.map_clock_ms);
    }else{c.picture(A_ShopBackground,0,22,320,218);c.actor(0,54,170);c.clip(AnimMerchantTop,181,62,a.animation*33,false);c.clip(AnimMerchantBottom,152,132,a.animation*33,false);}
    if(!a.shop_open){
        const int x=body_x,y=body_y,w=body_w,h=body_h;
        // Pinned NMerchantButton.RefreshFocus (0x06007d3d) uses the outline
        // skin for ordinary focus; its corner reticle is targeting-only.
        // Older packs lack that skin, so retain their explicit art fallback.
        if(focused&&!effects){const auto p=a.content.image(a.content.world_image("map","SELECTION_RETICLE"));
            if(p.pixels&&p.format<4)for(unsigned corner=0;corner<4;++corner){bool right=corner&2,bottom=corner&1;int left=right?x+w-4:x-2,top=bottom?y+h-4:y-2;
                map_reticle_tile(c,p,left+1,top+1,left+7,top+7,right,bottom,255,true);
                map_reticle_tile(c,p,left,top,left+6,top+6,right,bottom,255,false);}}
        c.action(x-2,y-2,w+4,h+4,UiAction::OpenShop);
        const char*label=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
        native_proceed(c,211,210,105,27,*label?label:"前进",true,!a.pointer.visible&&a.cursor==1);c.action(206,207,112,31,UiAction::LeaveShop);
        return;
    }
    c.dim(0,22,320,218,204);c.ui(U_MerchantRug,MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH);
    a.cursor=mn(mx(a.cursor,0),ShopLeave);
    // These seven portraits are all visible now. Decode their shared retained
    // groups once through the existing bounded image pool on the cold frame.
    int portraits[ShopCardCount];unsigned portrait_count=0;
    for(int i=0;i<ShopCardCount;++i)if(!(s.shop_sold&(1u<<i))){const Card card=s.world.active?s.world.shop_cards[i]:Card{s.shop_cards[i],0};
        portraits[portrait_count++]=a.card_face_image(card,-1,-1,s.world.active?&s.world.shop_facts[i]:nullptr);}
    a.content.prepare_images(portraits,portrait_count);
    for(int i=0;i<ShopCardCount;i++){
        const int cx=MerchantInventoryLayout::card_x(i),cy=MerchantInventoryLayout::card_y(i);const bool sold=s.shop_sold&(1u<<i);
        const int hit_height=i<5?69:75;const bool selected=merchant_hovered(a,i,cx-22,cy-31,44,hit_height);
        merchant_card(c,s.world.active?s.world.shop_cards[i]:Card{s.shop_cards[i],0},cx,cy,selected,sold,s.world.active?&s.world.shop_facts[i]:nullptr);
        merchant_price(c,i,cx-23,cy+(selected?32:27),46);
        if(s.world.active&&s.world.shop_sale==i&&!sold){int sale=a.content.world_image("merchant","SALE_TAG");if(sale>=0)c.picture(sale,cx+9,cy-(selected?30:25),18,14);}
        c.hit(cx-22,cy-31,44,hit_height,HitKind::Choice,i,!sold);
    }
    for(int i=0;i<3;i++){
        const int x=MerchantInventoryLayout::item_x(i);int item=ShopRelicStart+i,index=-1;bool sold=s.shop_sold&(1u<<item);
        if(s.world.active)index=s.world.shop_relics[i]==NoWorldRelic?-1:s.world.shop_relics[i];else for(int r=0;r<RelicKinds;r++)if(relic_definitions[r].id==s.shop_relics[i])index=r;
        bool selected=merchant_hovered(a,item,x-13,137,26,33);int size=selected?20:16;
        if(index>=0&&!sold)c.picture(a.relic_face_image(index),x-size/2,MerchantInventoryLayout::RelicY-size/2,size,size);
        merchant_price(c,item,x-14,160,28);c.hit(x-13,137,26,33,HitKind::Choice,item,!sold);
        item=ShopPotionStart+i;sold=s.shop_sold&(1u<<item);selected=merchant_hovered(a,item,x-13,171,26,29);
        const int w=selected?14:11,h=selected?18:14;
        if(!sold)c.picture(a.content.potion_image((Potion)s.shop_potions[i]),x-w/2,MerchantInventoryLayout::PotionY-h/2,w,h);
        merchant_price(c,item,x-14,188,28);c.hit(x-13,171,26,29,HitKind::Choice,item,!sold);
    }
    const bool removed=s.shop_sold&(1u<<ShopRemove),remove_focus=merchant_hovered(a,ShopRemove,228,140,48,59);
    const int remove_size=remove_focus?40:32;
    c.ui(removed?U_ShopRemoveUsed:U_ShopRemove,MerchantInventoryLayout::RemoveX-remove_size/2,MerchantInventoryLayout::RemoveY-remove_size/2,remove_size,remove_size);
    merchant_price(c,ShopRemove,228,188,46);c.hit(228,140,48,59,HitKind::Choice,ShopRemove,!removed);
    native_back(c,3,212,52,28,"返回",true,!a.pointer.visible&&a.cursor==ShopLeave);c.hit(0,209,61,31,HitKind::Choice,ShopLeave);
    // Original slots own hover tips; there is no duplicate title strip or
    // permanent keyboard-instruction footer over the merchant's rug.
    if(a.cursor>=7&&a.cursor<13&&merchant_hovered(a,a.cursor,MerchantInventoryLayout::item_x(unsigned((a.cursor-7)%3))-13,a.cursor<10?137:171,26,a.cursor<10?33:29)&&!a.pointer.held){
        char description[2048]{};const char*name=nullptr;
        if(a.cursor<10){name=s.world.active?a.relic_title(s.world.shop_relics[a.cursor-7]):relic_name(s.shop_relics[a.cursor-7]);
            if(s.world.active)a.relic_description(s.world.shop_relics[a.cursor-7],description,sizeof description);
            else{TextBuffer b{description,sizeof description,0,&a.content};b.text(relic_desc(s.shop_relics[a.cursor-7]));}}
        else{name=potion_name(s.shop_potions[a.cursor-10],&a.content);ui_potion_description(a,s.shop_potions[a.cursor-10],description,sizeof description);}
        const int width=174,lines=mn(5,c.paragraph(description,0,0,width-16,0,0,10)),height=27+lines*13,x=10,y=mn(135-height,55);
        const int skin=a.content.world_image("common","HOVER_TIP");
        if(skin>=0)c.nine_image(skin,x,y,width,height,28,22,46,16);else{c.rect(x,y,width,height,Panel);c.box(x,y,width,height,Muted);}
        c.text(name,x+8,y+5,12,Gold,width-16);c.paragraph(description,x+8,y+24,width-16,lines,0,10,rgb(255,246,226));
    }
}
