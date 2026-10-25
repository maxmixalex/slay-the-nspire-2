#include "room_text_ui.inc"
// Five class cards, two colorless, three relics/potions and a right-hand removal service.
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
    if(a.paused||a.help||a.panel||a.library||a.settings_open||a.event_reader||a.map_window.is_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0)return false;
    return a.pointer.visible?a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h:a.cursor==item;
}
static unsigned room_rich_characters(const char*text){RichReader reader(text);RichChar ch{};unsigned count=0;while(reader.next(ch))count=ch.index+1;return count;}
static void merchant_hand(Canvas&c,bool fake=false){
 auto&a=c.a;const auto&m=a.room_presentation.merchant;if(a.content.version()<RoomUi7ContentVersion)return;
 // The source drives rotate_me across a constrained weighted mesh. Its
 // bounded samples preserve deformation; a whole-texture rotation cannot.
 const int source_x=int(m.hand_x)*9/32;
 const int degrees=-10+(source_x-770)/5;
 const unsigned sample=unsigned(mn(46,mx(0,(degrees+134)/5)));
 c.ui((fake?I7_FakeMerchantHandStart:I7_MerchantHandStart)+int(sample),m.hand_x/16-MerchantHandPivotX,m.hand_y/16-MerchantHandPivotY);
}
static void merchant_dialogue(Canvas&c,bool fake=false){
 auto&a=c.a;const auto&m=a.room_presentation.merchant;const unsigned alpha=m.dialogue_alpha(a.map_clock_ms);if(!alpha)return;
 const char*families[]={"purchaseSuccess","purchaseFailureGold","purchaseFailureSpace","purchaseFailureForbidden","openInventory","welcome"};
 if(m.dialogue<0||m.dialogue>=6)return;
 const unsigned ordinary_counts[]={3,4,2,2,0,0},fake_counts[]={6,3,3,3,8,4};const unsigned count=(fake?fake_counts:ordinary_counts)[m.dialogue];if(!count)return;
 char key[128]{};TextBuffer k{key,sizeof key,0};k.text(fake?"FAKE_MERCHANT.talk.":"MERCHANT.talk.");k.text(families[m.dialogue]);k.text(".line");k.number(1+int((m.dialogue_random>>16)%count));
 const char*text=a.content.world_string(fake?"events":"merchant_room",key);if(!*text)return;
 const unsigned age=a.map_clock_ms-m.dialogue_start;
 const int node_y=MerchantInventoryLayout::project(271)-18*(int(room_curve::Q)-room_curve::back_out(room_curve::linear(age,500)))/int(room_curve::Q);
 const unsigned scale=1365+2731*room_curve::expo_out(age,500)/room_curve::Q;
 const int width=int((70*scale+2048)/4096),height=int((48*scale+2048)/4096);
 const int center_x=m.dialogue_x-10,center_y=node_y-6;
 Canvas bubble=c;bubble.opacity=uint8_t(div255(bubble.opacity*alpha));
 if(a.content.version()>=RoomUi7ContentVersion){
  Canvas shadow=bubble;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=uint8_t(div255(shadow.opacity*64));
  shadow.ui(I7_MerchantDialogue,center_x-width/2,center_y-height/2+1,width,height);
  bubble.ui(I7_MerchantDialogue,center_x-width/2,center_y-height/2,width,height);
 }else{const int skin=a.content.world_image("common","HOVER_TIP");if(skin>=0)bubble.nine_image(skin,center_x-35,center_y-24,70,48,28,22,46,16);}
 bubble.text_visible_characters=room_rich_characters(text)*room_curve::linear(age,400)/room_curve::Q;
 bubble.opacity=uint8_t(div255(bubble.opacity*230));
 // Source MarginContainer (-182,-66)..(98,64), Text's +2px Y offset,
 // and 18..24px native font range share the inventory's uniform 2/9 scale.
 native_room_compact_text(bubble,text,m.dialogue_x-40,node_y-14,62,29,true);
}
static void merchant_card(Canvas&c,Card card,int cx,int cy,unsigned scale,bool sold,const EventCardFacts*facts=nullptr){
 if(sold)return;const int w=MerchantInventoryLayout::size(300,scale),h=MerchantInventoryLayout::size(422,scale);
 if(cy+h/2<22||cy-h/2>=240)return;
 draw_card_face(c,card,cx-w/2,cy-h/2,w,h,-2,-2,-1,-1,facts);
}
static void merchant_room_clip(Canvas&c,int clip,uint32_t milliseconds,bool covered){
    const auto sequence=c.a.content.clip(clip);if(!sequence.count)return;
    const unsigned frame=milliseconds/sequence.frame_ms%sequence.count;
    const int image=int(sequence.first+frame),x=-sequence.pivot_x,y=22-sequence.pivot_y;
    if(covered){
        // Geometry is resident metadata: a hidden frame is never decoded just
        // to discover its bounds. The actual rug mask, not its rectangular
        // control or a fixed silhouette, proves every covered screen sample.
        const auto size=c.a.content.image_dimensions(image);
        if(size.w&&size.h&&c.opaque_picture_covers(c.a.content.image(c.a.content.ui_image(U_MerchantRug)),
            MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH,
            x,y,size.w,size.h))return;
    }
    c.picture(image,x,y);
}
static void merchant_idle(Canvas&c,int clip,bool covered=false){
    const auto sequence=c.a.content.clip(clip);if(!sequence.count)return;
    // Both skins sample the same authored six-second idle phase. The v18
    // directory stores rounded frame_ms, so select by the original duration.
    const unsigned frame=(c.a.map_clock_ms%MerchantIdleDurationMs)*sequence.count/MerchantIdleDurationMs;
    merchant_room_clip(c,clip,frame*sequence.frame_ms,covered);
}
static void shop_screen(Canvas&c,bool cull_covered=true){
    auto&a=c.a;auto&s=a.game;auto&motion=a.room_presentation.merchant;motion.set_open(a.shop_open,a.map_clock_ms);const bool registered=a.content.ui_image(U_MerchantBackdrop)>=0&&a.content.clip(AnimMerchantRoomTop).count;
    const int body_x=registered?MerchantLayout::BodyX:181,body_y=registered?MerchantLayout::BodyY:62;
    const int body_w=registered?MerchantLayout::BodyW:105,body_h=registered?MerchantLayout::BodyH:109;
    const bool active=!a.shop_open&&!a.paused&&!a.help&&!a.panel&&!a.library&&!a.settings_open&&!a.frontend_confirm&&
        a.detail_card<0&&a.detail_item<0&&a.status_actor<0;
    const bool focused=active&&(a.pointer.visible?
        a.pointer.x>=body_x-2&&a.pointer.x<body_x+body_w+2&&a.pointer.y>=body_y-2&&a.pointer.y<body_y+body_h+2:a.cursor==0);
    const bool effects=registered&&a.content.version()>=MerchantEffectsContentVersion;
    // Wait for both source opening tweens. Opening, interruption and closing
    // always render the complete room; settled inventory can omit only layers
    // proven to be wholly overwritten by the opaque original rug.
    const bool covered=cull_covered&&a.shop_open&&motion.open&&a.map_clock_ms-motion.start>=1000;
    if(registered){
        c.ui(U_MerchantBackdrop,0,22,320,218);
        if(effects){
            merchant_room_clip(c,AnimMerchantFlamesBehind,a.map_clock_ms%MerchantIdleDurationMs,covered);
            merchant_idle(c,AnimMerchantRoomBottom,covered);
            merchant_room_clip(c,AnimMerchantFlamesFront,a.map_clock_ms%MerchantIdleDurationMs,covered);
        }else c.clip(AnimMerchantRoomBottom,0,22,a.map_clock_ms);
        merchant_room_clip(c,AnimMerchantIronclad+int(s.character),a.map_clock_ms,covered);
        if(effects)merchant_idle(c,focused?AnimMerchantRoomFocused:AnimMerchantRoomTop,covered);
        else c.clip(AnimMerchantRoomTop,0,22,a.map_clock_ms);
    }else{c.picture(A_ShopBackground,0,22,320,218);c.actor(0,54,170);c.clip(AnimMerchantTop,181,62,a.animation*33,false);c.clip(AnimMerchantBottom,152,132,a.animation*33,false);}
    const char*proceed_label=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
    native_proceed(c,*proceed_label?proceed_label:"前进",!a.shop_open,!a.pointer.visible&&a.cursor==1);
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
        native_proceed_action(c,UiAction::LeaveShop);
        if(!motion.visible(a.map_clock_ms))return;
    }
    const int dy=motion.offset(a.map_clock_ms);
    c.dim(0,22,320,218,motion.alpha(a.map_clock_ms));c.ui(U_MerchantRug,MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY+dy,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH);
    a.cursor=mn(mx(a.cursor,0),ShopLeave);
    int focus=-1,focus_x=0,focus_y=0;
    if(a.shop_open)for(unsigned item=0;item<ShopLeave;++item){const auto r=MerchantInventoryLayout::bounds(item,dy);
        if(!(s.shop_sold&(1u<<item))&&merchant_hovered(a,int(item),r.x,r.y,r.w,r.h)){
            focus=int(item);focus_x=item<7?MerchantInventoryLayout::card_x(item):item<13?MerchantInventoryLayout::item_x((item-7)%3):MerchantInventoryLayout::RemoveX;
            focus_y=(item<7?MerchantInventoryLayout::card_y(item):item<10?MerchantInventoryLayout::RelicY:item<13?MerchantInventoryLayout::PotionY:MerchantInventoryLayout::RemoveY)+dy;}}
    motion.observe_focus(focus,focus_x,focus_y,a.map_clock_ms);
    if((s.shop_sold&(1u<<ShopRemove))&&!(motion.sold_mask&(1u<<ShopRemove)))motion.remove_start=a.map_clock_ms;
    motion.sold_mask=s.shop_sold;
    // Prepare only portraits whose moving source slot intersects the viewport.
    // The opening tween supplies work time without an artificial loading screen.
    int portraits[ShopCardCount];unsigned portrait_count=0;
    for(int i=0;i<ShopCardCount;++i)if(!(s.shop_sold&(1u<<i))){const int y=MerchantInventoryLayout::card_y(i)+dy;
        if(y+MerchantInventoryLayout::FocusH/2<22||y-MerchantInventoryLayout::FocusH/2>=240)continue;
        const Card card=s.world.active?s.world.shop_cards[i]:Card{s.shop_cards[i],0};portraits[portrait_count++]=a.card_face_image(card,-1,-1,s.world.active?&s.world.shop_facts[i]:nullptr);}
    a.content.prepare_images(portraits,portrait_count);
    for(int i=0;i<ShopCardCount;i++){
        const int cx=MerchantInventoryLayout::card_x(i),cy=MerchantInventoryLayout::card_y(i)+dy;const bool sold=s.shop_sold&(1u<<i);
        const unsigned scale=motion.scale(unsigned(i),a.map_clock_ms);const auto r=MerchantInventoryLayout::bounds(unsigned(i),dy);
        merchant_card(c,s.world.active?s.world.shop_cards[i]:Card{s.shop_cards[i],0},cx+motion.wiggle(unsigned(i),a.map_clock_ms),cy,scale,sold,s.world.active?&s.world.shop_facts[i]:nullptr);
        merchant_price(c,i,cx-26,cy+MerchantInventoryLayout::size(220,scale),52);
        if(s.world.active&&s.world.shop_sale==i&&!sold){const int sale=a.content.world_image("merchant","SALE_TAG");if(sale>=0)c.picture(sale,cx+MerchantInventoryLayout::size(146,scale)-10,cy-MerchantInventoryLayout::size(153,scale)-8,20,16);}
        if(a.shop_open)c.hit(r.x,mx(22,r.y),r.w,mx(0,r.y+r.h-mx(22,r.y)),HitKind::Choice,i,!sold);
    }
    for(int i=0;i<3;i++){
        const int x=MerchantInventoryLayout::item_x(i);int item=ShopRelicStart+i,index=-1;bool sold=s.shop_sold&(1u<<item);
        if(s.world.active)index=s.world.shop_relics[i]==NoWorldRelic?-1:s.world.shop_relics[i];else for(int r=0;r<RelicKinds;r++)if(relic_definitions[r].id==s.shop_relics[i])index=r;
        unsigned scale=motion.scale(unsigned(item),a.map_clock_ms);int size=MerchantInventoryLayout::size(128,scale);auto r=MerchantInventoryLayout::bounds(unsigned(item),dy);
        if(index>=0&&!sold)c.picture(a.relic_face_image(index),x-size/2+motion.wiggle(unsigned(item),a.map_clock_ms),MerchantInventoryLayout::RelicY+dy-size/2,size,size);
        merchant_price(c,item,x-16,MerchantInventoryLayout::RelicY+dy+MerchantInventoryLayout::size(52,scale),32);
        if(a.shop_open)c.hit(r.x,mx(22,r.y),r.w,mx(0,r.y+r.h-mx(22,r.y)),HitKind::Choice,item,!sold);
        item=ShopPotionStart+i;sold=s.shop_sold&(1u<<item);scale=motion.scale(unsigned(item),a.map_clock_ms);r=MerchantInventoryLayout::bounds(unsigned(item),dy);
        const int w=MerchantInventoryLayout::size(90,scale),h=MerchantInventoryLayout::size(120,scale);
        if(!sold)c.picture(a.content.potion_image((Potion)s.shop_potions[i]),x-w/2+motion.wiggle(unsigned(item),a.map_clock_ms),MerchantInventoryLayout::PotionY+dy-h/2,w,h);
        merchant_price(c,item,x-16,MerchantInventoryLayout::PotionY+dy+MerchantInventoryLayout::size(48,scale),32);
        if(a.shop_open)c.hit(r.x,mx(22,r.y),r.w,mx(0,r.y+r.h-mx(22,r.y)),HitKind::Choice,item,!sold);
    }
    const bool removed=s.shop_sold&(1u<<ShopRemove);const unsigned remove_scale=motion.scale(ShopRemove,a.map_clock_ms);
    int remove_image=removed?U_ShopRemoveUsed:U_ShopRemove;const unsigned remove_age=a.map_clock_ms-motion.remove_start;
    if(removed&&a.content.version()>=RoomUi7ContentVersion){
        if(remove_age<533)remove_image=U_ShopRemove;
        else if(remove_age<600)remove_image=I7_MerchantRemoval01;
        else if(remove_age<667)remove_image=I7_MerchantRemoval02;
        else if(remove_age<733)remove_image=I7_MerchantRemoval04;
    }
    const int remove_size=MerchantInventoryLayout::size(270,remove_scale);const auto remove_rect=MerchantInventoryLayout::bounds(ShopRemove,dy);
    c.ui(remove_image,MerchantInventoryLayout::RemoveX-remove_size/2+motion.wiggle(ShopRemove,a.map_clock_ms),MerchantInventoryLayout::RemoveY+dy-remove_size/2,remove_size,remove_size);
    merchant_price(c,ShopRemove,MerchantInventoryLayout::RemoveX-26,MerchantInventoryLayout::RemoveY+dy+MerchantInventoryLayout::size(177,remove_scale),52);
    if(a.shop_open){c.hit(remove_rect.x,mx(22,remove_rect.y),remove_rect.w,mx(0,remove_rect.y+remove_rect.h-mx(22,remove_rect.y)),HitKind::Choice,ShopRemove,!removed);
        native_back(c,3,212,52,28,"返回",true,!a.pointer.visible&&a.cursor==ShopLeave);c.hit(0,209,61,31,HitKind::Choice,ShopLeave);}
    merchant_hand(c);merchant_dialogue(c);
    // NMerchantRelic/Potion expose their complete model hover sets through
    // the shared source-sized layer after the room and its global UI.
}
