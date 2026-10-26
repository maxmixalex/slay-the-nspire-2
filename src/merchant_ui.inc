#include "room_text_ui.inc"
// Five class cards, two colorless, three relics/potions and a right-hand removal service.
static const char* potion_name(int p,const Content*content=nullptr){auto*d=potion_definition((Potion)p);if(d&&content){const char*name=content->world_title("potions",d->key);if(*name)return name;}const char*name=d?d->name:"空药水槽";return content?content->ui_text(name):name;}
static const char* potion_desc(int p){auto*d=potion_definition((Potion)p);return d?d->description:"药水栏为空。可在商店购买或领取战斗掉落。";}
static void ui_potion_description(const App&a,int p,char*out,size_t size){
 if(!size)return;out[0]=0;if(const auto*d=potion_definition((Potion)p)){char key[96]{};TextBuffer b{key,sizeof key,0};b.text(d->key);b.text(".description");
  if(*a.content.world_string("potions",key)){a.localized_text("potions",key,out,size);return;}}
 TextBuffer b{out,size,0,&a.content};b.text(potion_desc(p));
}
static void merchant_cost_label(Canvas&c,int amount,int x,int y,unsigned scale,bool affordable,bool sale=false,bool card=false){
    const bool english=c.a.content.english();const unsigned face=source_label_face(c.a.content.original_label_fonts(),5);
    const int size=c.a.content.font_size(mx(10,MerchantInventoryLayout::size(39,scale)),face,false,true);
    char value[24]{};TextBuffer text{value,sizeof value,0};text.number(amount);
    Glyph glyphs[12];int positions[12];unsigned count=0;int advance64=0;uint32_t previous=0;
    for(const char*p=value;*p&&count<12;++p){
        const uint32_t cp=uint8_t(*p);advance64+=c.a.content.glyph_kerning64(previous,cp,size,face);previous=cp;
        const auto g=c.a.content.glyph(cp,size,face);glyphs[count]=g;positions[count++]=text_pixel(advance64)+g.bearing_x;
        advance64+=g.advance64?g.advance64:int(g.advance)*64;
    }
    const int width=text_pixel(advance64);
    const int coin=MerchantInventoryLayout::size(54,scale),gap=MerchantInventoryLayout::size(6,scale),left=x-(coin+gap+width)/2;
    // The source HBox centers its54px coin beside the original39px label.
    // CostLabel's measured baseline, not the glyph's padded bitmap height,
    // controls vertical placement. Locale substitution changes that baseline.
    const int baseline=english?(card?675:643):712; // source pixels *16
    const int origin=y+(baseline*int(scale)*2+72000)/144000-(size+1);
    c.picture(A_Coin,left,y+MerchantInventoryLayout::size(card||!english?2:0,scale),coin,coin);
    Canvas outline=c;outline.opacity=uint8_t(div255(outline.opacity*110));
    const int radius=0,start=left+coin+gap;
    for(unsigned i=0;i<count;++i)outline.glyph(glyphs[i],start+positions[i]+1,origin+1,rgb(17,0,0));
    const auto color=affordable?(sale?rgb(127,255,0):rgb(255,246,226)):rgb(255,85,85);
    for(unsigned i=0;i<count;++i)c.glyph(glyphs[i],start+positions[i],origin,color);
}
static void merchant_price(Canvas&c,int item,int x,int y,unsigned scale){
    auto&s=c.a.game;bool sold=s.shop_sold&(1u<<item);int price=s.world.active?world_shop_price(s,item):item==ShopRemove?remove_price(s):s.shop_prices[item];
    if(sold)return;
    const bool sale=s.world.active&&item<ShopCardCount&&s.world.shop_sale==item;
    merchant_cost_label(c,price,x,y,scale,s.gold>=price,sale,item<ShopCardCount);
}
static bool merchant_hovered(const App&a,int item,int x,int y,int w,int h){
    if(a.paused||a.help||a.panel||a.library||a.settings_open||a.event_reader||a.map_window.is_open||a.frontend_confirm||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0)return false;
    return a.pointer.visible?a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h:a.cursor==item;
}
static unsigned room_rich_characters(const char*text){RichReader reader(text);RichChar ch{};unsigned count=0;while(reader.next(ch))count=ch.index+1;return count;}
#include "speech_bubble_ui.inc"
static void merchant_hand(Canvas&c,bool fake=false){
 auto&a=c.a;const auto&m=a.room_presentation.merchant;if(a.content.version()<RoomUi7ContentVersion)return;
 // The source drives rotate_me across a constrained weighted mesh. Its
 // bounded samples preserve deformation; a whole-texture rotation cannot.
 const int source_x=int(m.hand_x)*9/32;
 const int degrees=-10+(source_x-770)/5;
 const unsigned sample=unsigned(mn(46,mx(0,(degrees+134)/5)));
 // The sampled source mesh already uses the LCD canvas. Preserve its
 // full scale and fingertip anchor; native-size sparse rows avoid resampling.
 c.ui((fake?I7_FakeMerchantHandStart:I7_MerchantHandStart)+int(sample),m.hand_x/16-160,m.hand_y/16-230,320,240);
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
 const int width=int((128*scale+2048)/4096),height=int((80*scale+2048)/4096);
 const int center_x=mn(254,mx(66,int(m.dialogue_x)-10)),center_y=mx(62,node_y-6);
 Canvas bubble=c;bubble.opacity=uint8_t(div255(bubble.opacity*alpha));
 if(a.content.version()>=RoomUi7ContentVersion){
  Canvas shadow=bubble;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=uint8_t(div255(shadow.opacity*64));
  shadow.ui(I7_MerchantDialogue,center_x-width/2,center_y-height/2+1,width,height);
  bubble.ui(I7_MerchantDialogue,center_x-width/2,center_y-height/2,width,height);
 }else{const int skin=a.content.world_image("common","HOVER_TIP");if(skin>=0)bubble.nine_image(skin,center_x-64,center_y-40,128,80,28,22,46,16);}
 bubble.text_fly_in_elapsed=age;
 bubble.opacity=uint8_t(div255(bubble.opacity*230));
 native_room_compact_text(bubble,text,center_x-56,center_y-30,112,60,true);
}
static void merchant_card(Canvas&c,Card card,int cx,int cy,unsigned scale,bool sold,const EventCardFacts*facts=nullptr){
 if(sold)return;const int w=MerchantInventoryLayout::size(300,scale),h=MerchantInventoryLayout::size(422,scale);
 if(cy+h/2<22||cy-h/2>=240)return;
 draw_card_face(c,card,cx-w/2,cy-h/2,w,h,-2,-2,-1,-1,facts);
}
void App::prepare_merchant_inventory(){
 auto&p=merchant_preparation;p.reset();
 // Old/direct profiles have no cold tile work. Fake Merchant contains only
 // six relics and keeps its separate event inventory lifecycle.
 if(game.phase!=Phase::Shop||content.version()<Ui12ContentVersion)return;
 const auto add=[&](int image){
  if(image<0||content.image_cached(image))return;
  for(unsigned i=0;i<p.count;++i)if(content.image_alias(p.images[i],image))return;
  if(p.count<MerchantPreparation::Capacity)p.images[p.count++]=image;
 };
 add(content.ui_image(U_MerchantRug));add(content.ui_image(U_ShopRemove));add(A_Coin);
 add(content.world_image("merchant","SALE_TAG"));
 for(int i=0;i<ShopCardCount;++i)if(!(game.shop_sold&(1u<<i))){
  const Card card=game.world.active?game.world.shop_cards[i]:Card{game.shop_cards[i],0};
  const auto*facts=game.world.active?&game.world.shop_facts[i]:nullptr;
  const auto type=card_face_type(card,-1,-1,facts);
  const unsigned group=card_face::material_group(unsigned(card_rarity(card.kind)));
  const int base=U_CardBorderCommonAttack+int(group)*5;
  add(card_face_image(card,-1,-1,facts));add(card_face_frame(card,-1,-1,facts));
  add(content.ui_image(base+(type==CardType::Attack?0:type==CardType::Power?2:1)));
  add(content.ui_image(base+3));add(content.ui_image(base+4));add(content.energy_image(card));
 }
 for(int i=0;i<3;++i){
  if(!(game.shop_sold&(1u<<(ShopRelicStart+i)))){
   int id=game.world.active?(game.world.shop_relics[i]==NoWorldRelic?-1:game.world.shop_relics[i]):-1;
   if(!game.world.active)for(int r=0;r<RelicKinds;++r)if(relic_definitions[r].id==game.shop_relics[i])id=r;
   if(id>=0)add(relic_face_image(id));
  }
  if(!(game.shop_sold&(1u<<(ShopPotionStart+i))))add(content.potion_image(Potion(game.shop_potions[i])));
 }
 // Stored order improves tile-group locality without a resource-wide scan.
 for(unsigned i=1;i<p.count;++i){int id=p.images[i];unsigned j=i;while(j&&p.images[j-1]>id){p.images[j]=p.images[j-1];--j;}p.images[j]=id;}
 p.active=p.count!=0;
}
void App::step_merchant_preparation(){
 auto&p=merchant_preparation;
 if(!p.active||!p.shown||paused||help||panel||library||settings_open||map_window.is_open)return;
 if(title||game.phase!=Phase::Shop||!shop_open){p.reset();return;}
 ++p.updates;
 while(p.cursor<p.count){const int id=p.images[p.cursor++];if(content.image_cached(id))continue;
  ++p.cold_images;const auto image=content.image(id);
  if(!image.pixels){p.reset();shop_open=false;message("资源读取失败");}return;
 }
 // A clipped production face resolves the exact title/body/style glyphs.
 // It neither discovers a card nor mutates the canonical run or face cache.
 while(p.card<ShopCardCount){const unsigned i=p.card++;if(game.shop_sold&(1u<<i))continue;
  const Card card=game.world.active?game.world.shop_cards[i]:Card{game.shop_cards[i],0};
  const auto*facts=game.world.active?&game.world.shop_facts[i]:nullptr;
  Canvas hidden{*this};hidden.clip_w=hidden.clip_h=0;
  const unsigned scale=MerchantInventoryLayout::card_scale(650);
  render_card_face(hidden,card,0,0,MerchantInventoryLayout::size(300,scale),MerchantInventoryLayout::size(422,scale),-2,-2,-1,-1,facts);
  return;
 }
 p.active=false;room_presentation.merchant.set_open(true,map_clock_ms);room_presentation.merchant.speak(4,map_clock_ms);
}
// Rug coverage is immutable for a resource generation and geometry. Cache
// the actual alpha proof, including false results, instead of rescanning its
// thousands of samples for every flame, actor and glow on every frame.
static bool merchant_rug_covers(Canvas&c,int x,int y,int w,int h){
 struct Entry {int16_t x=0,y=0,w=0,h=0;bool valid=false,covered=false;};
 static const Content*owner=nullptr;static uint32_t generation=0;static Entry entries[64]{};static unsigned next=0;
 if(owner!=&c.a.content||generation!=c.a.content.generation()){
  owner=&c.a.content;generation=c.a.content.generation();next=0;for(auto&e:entries)e.valid=false;
 }
 if(c.opacity!=255||c.clip_w!=320||c.clip_h!=240)return false;
 for(const auto&e:entries)if(e.valid&&e.x==x&&e.y==y&&e.w==w&&e.h==h)return e.covered;
 const bool covered=c.opaque_picture_covers(c.a.content.image(c.a.content.ui_image(U_MerchantRug)),
  MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH,x,y,w,h);
 entries[next]={int16_t(x),int16_t(y),int16_t(w),int16_t(h),true,covered};next=(next+1)%64;return covered;
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
        if(size.w&&size.h&&merchant_rug_covers(c,x,y,size.w,size.h))return;
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
static void merchant_secondary_lights(Canvas&c,bool covered){
    // merchant_room.tscn candle/bottle lights, projected through its existing
    // BgContainer and the same1920x1080 camera used by the authored clips.
    // Flame planes already animate; these are their separate light/cinder nodes.
    struct Light {int16_t x,y;uint8_t size,alpha;uint32_t color;bool fire;};
    static constexpr Light lights[]={{12,52,24,72,0xc26121,true},{231,201,24,72,0xc26121,true},
        {240,207,20,72,0xc26121,true},{275,92,20,72,0xc26121,true},
        {222,84,47,45,0xbb71ca,false},{272,73,33,52,0xb5702e,false},
        {284,151,31,32,0x5c73cb,false},{320,160,31,40,0xc26120,false}};
    for(unsigned i=0;i<sizeof lights/sizeof lights[0];++i){const auto&light=lights[i];
        const int left=light.x-light.size/2,top=light.y-light.size/2;
        // Use the same exact opaque-rug proof as the animation planes. The
        // union also includes the cinders above a candle, not just its glow.
        const int margin=light.fire?39:light.size/2,above=light.fire?68:light.size/2;
        if(covered&&merchant_rug_covers(c,light.x-margin,light.y-above,margin*2+2,above+light.size/2+1))continue;
        const unsigned period=750+i*91;const int sway=room_curve::sine_turn((c.a.map_clock_ms%period)*4096/period+i*509);
        const unsigned alpha=unsigned(int(light.alpha)+sway*int(light.alpha)/24576);
        room_vfx_stamp(c,I7_Glow,left,top,light.size,light.size,alpha,light.color);
        if(light.fire)room_vfx_cinders(c,light.x,light.y,4,c.a.map_clock_ms,0x43414e44u+i,120);
    }
}
// Sample the fixed rug only once per resource generation. Its exact alpha
// proves which animated underlay pixels cannot reach the finished frame.
// Fractional rug edges still draw the complete live room and retain the two
// original RGB565 roundings: dim, then source-over rug. No animation clock,
// decoded frame, input or game state is omitted.
struct MerchantRugRaster {
 const Content*owner=nullptr;uint32_t generation=0;bool valid=false;
 uint16_t pixels[ScreenW*ScreenH]{};uint8_t alpha[ScreenW*ScreenH]{};
};
static MerchantRugRaster merchant_rug_raster;
static const MerchantRugRaster* merchant_cached_rug(Canvas&c){
 if(c.pixels!=c.a.pixels||c.pitch!=ScreenW||c.clip_w!=ScreenW||c.clip_h!=ScreenH||
    c.coverage||c.opacity!=255||c.brightness!=255||c.tint_red!=255||c.tint_green!=255||
    c.tint_blue!=255||c.color_matrix||c.occlusion)return nullptr;
 auto&r=merchant_rug_raster;
 if(r.owner!=&c.a.content||r.generation!=c.a.content.generation()){
  r.owner=&c.a.content;r.generation=c.a.content.generation();r.valid=false;
  for(unsigned i=0;i<ScreenW*ScreenH;++i){r.pixels[i]=0;r.alpha[i]=0;}
  Canvas tile=c;tile.pixels=r.pixels;tile.coverage=r.alpha;
  tile.ui(U_MerchantRug,MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY,
   MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH);
  r.valid=c.a.content.healthy();
 }
 return r.valid?&r:nullptr;
}
static void merchant_inventory_background(Canvas&c,const MerchantRugRaster*r,int dy,unsigned shade){
 if(!r){c.dim(0,22,320,218,shade);c.ui(U_MerchantRug,MerchantInventoryLayout::RugX,MerchantInventoryLayout::RugY+dy,MerchantInventoryLayout::RugW,MerchantInventoryLayout::RugH);return;}
 for(unsigned i=22*ScreenW;i<ScreenW*ScreenH;++i){
  const unsigned alpha=r->alpha[i];
  if(alpha==255){c.pixels[i]=r->pixels[i];continue;}
  uint16_t background=shade==255?Ink:shade?blend(c.pixels[i],Ink,shade):c.pixels[i];
  c.pixels[i]=alpha?blend(background,r->pixels[i],alpha):background;
 }
 // The authored rug begins at18, above the room's22px dim boundary. Keep
 // that small overlap exact without excluding the independently drawn HUD.
 for(unsigned i=0;i<22*ScreenW;++i)if(r->alpha[i])
  c.pixels[i]=r->alpha[i]==255?r->pixels[i]:blend(c.pixels[i],r->pixels[i],r->alpha[i]);
}
static void shop_screen(Canvas&c,bool cull_covered=true){
    auto&a=c.a;auto&s=a.game;auto&motion=a.room_presentation.merchant;motion.set_open(a.shop_open&&!a.merchant_preparation.active,a.map_clock_ms);const bool registered=a.content.ui_image(U_MerchantBackdrop)>=0&&a.content.clip(AnimMerchantRoomTop).count;
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
    const auto*rug=covered?merchant_cached_rug(c):nullptr;
    Canvas room=c;if(rug)room.occlusion=rug->alpha;
    // The preparation overlay is modal. Reuse its first complete room frame
    // while warming inventory assets; decoding a new idle/flame frame on
    // each yielded step would compete with the very work being prepared.
    // This scratch surface adds 136.25 KiB, only once, with no game/save state.
    struct PreparationBackdrop {
        const App*owner=nullptr;uint32_t generation=0;
        uint16_t pixels[ScreenW*(ScreenH-22)]{};
    };
    static PreparationBackdrop preparation_backdrop;
    const bool retained=a.merchant_preparation.active&&a.merchant_preparation.shown&&
        preparation_backdrop.owner==&a&&preparation_backdrop.generation==a.content.generation();
    if(retained){
        for(unsigned i=0;i<ScreenW*(ScreenH-22);++i)a.pixels[ScreenW*22+i]=preparation_backdrop.pixels[i];
    }else if(registered){
        room.ui(U_MerchantBackdrop,0,22,320,218);
        if(effects)merchant_secondary_lights(room,covered);
        if(effects){
            merchant_room_clip(room,AnimMerchantFlamesBehind,a.map_clock_ms%MerchantIdleDurationMs,covered);
            merchant_idle(room,AnimMerchantRoomBottom,covered);
            merchant_room_clip(room,AnimMerchantFlamesFront,a.map_clock_ms%MerchantIdleDurationMs,covered);
        }else room.clip(AnimMerchantRoomBottom,0,22,a.map_clock_ms);
        merchant_room_clip(room,AnimMerchantIronclad+int(s.character),a.map_clock_ms,covered);
        if(effects)merchant_idle(room,focused?AnimMerchantRoomFocused:AnimMerchantRoomTop,covered);
        else room.clip(AnimMerchantRoomTop,0,22,a.map_clock_ms);
    }else{c.picture(A_ShopBackground,0,22,320,218);c.actor(0,54,170);c.clip(AnimMerchantTop,181,62,a.animation*33,false);c.clip(AnimMerchantBottom,152,132,a.animation*33,false);}
    const char*proceed_label=a.content.world_string("gameplay_ui","PROCEED_BUTTON");
    if(a.merchant_preparation.active){
        auto&p=a.merchant_preparation;
        if(!retained){
            for(unsigned i=0;i<ScreenW*(ScreenH-22);++i)preparation_backdrop.pixels[i]=a.pixels[ScreenW*22+i];
            preparation_backdrop.owner=&a;preparation_backdrop.generation=a.content.generation();
        }
        p.shown=true;
        c.dim(0,22,320,218,110);
        // Immediate acknowledgement while the original inventory is prepared.
        // The three dots animate without introducing another font or texture.
        for(unsigned i=0;i<3;++i)c.rect(147+int(i)*11,119,5,5,(a.map_clock_ms/180)%3==i?Gold:Muted);
        native_back(c,0,211,31,28,"",true,!a.pointer.visible);c.action(0,209,32,31,UiAction::CloseShop);
        return;
    }
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
    merchant_inventory_background(c,rug,dy,motion.alpha(a.map_clock_ms));
    a.cursor=mn(mx(a.cursor,0),ShopLeave);
    int focus=-1,focus_x=0,focus_y=0;
    if(a.shop_open)for(unsigned item=0;item<ShopLeave;++item){auto r=MerchantInventoryLayout::bounds(item,dy);
        if(item>=ShopRelicStart&&item<ShopPotionStart)r=MerchantInventoryLayout::relic_bounds(MerchantInventoryLayout::item_x(item-ShopRelicStart),MerchantInventoryLayout::RelicY+dy,motion.scale(item,a.map_clock_ms));
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
    auto draw_stock_card=[&](int i){
        const int cx=MerchantInventoryLayout::card_x(i),cy=MerchantInventoryLayout::card_y(i)+dy-(i>=5&&i==focus?12:0);const bool sold=s.shop_sold&(1u<<i);
        const unsigned scale=MerchantInventoryLayout::card_scale(motion.scale(unsigned(i),a.map_clock_ms));const auto r=MerchantInventoryLayout::bounds(unsigned(i),dy);
        merchant_card(c,s.world.active?s.world.shop_cards[i]:Card{s.shop_cards[i],0},cx+motion.wiggle(unsigned(i),a.map_clock_ms),cy,scale,sold,s.world.active?&s.world.shop_facts[i]:nullptr);
        merchant_price(c,i,cx,cy+MerchantInventoryLayout::size(220,scale),scale);
        if(s.world.active&&s.world.shop_sale==i&&!sold){const int sale=a.content.world_image("merchant","SALE_TAG");if(sale>=0)c.picture(sale,cx+MerchantInventoryLayout::size(146,scale)-10,cy-MerchantInventoryLayout::size(153,scale)-8,20,16);}
        if(a.shop_open)c.hit(r.x,mx(22,r.y),r.w,mx(0,r.y+r.h-mx(22,r.y)),HitKind::Choice,i,!sold);
    };
    for(int i=0;i<ShopCardCount;i++)if(i!=focus)draw_stock_card(i);
    for(int i=0;i<3;i++){
        const int x=MerchantInventoryLayout::item_x(i);int item=ShopRelicStart+i,index=-1;bool sold=s.shop_sold&(1u<<item);
        if(s.world.active)index=s.world.shop_relics[i]==NoWorldRelic?-1:s.world.shop_relics[i];else for(int r=0;r<RelicKinds;r++)if(relic_definitions[r].id==s.shop_relics[i])index=r;
        unsigned scale=motion.scale(unsigned(item),a.map_clock_ms);
        const auto icon=MerchantInventoryLayout::relic_bounds(x,MerchantInventoryLayout::RelicY+dy,scale);
        if(index>=0&&!sold)c.picture(a.relic_face_image(index),icon.x+motion.wiggle(unsigned(item),a.map_clock_ms),icon.y,icon.w,icon.h);
        merchant_price(c,item,x,MerchantInventoryLayout::RelicY+dy+MerchantInventoryLayout::size(52,scale),scale);
        if(a.shop_open)c.hit(icon.x,mx(22,icon.y),icon.w,mx(0,icon.y+icon.h-mx(22,icon.y)),HitKind::Choice,item,!sold);
        item=ShopPotionStart+i;sold=s.shop_sold&(1u<<item);scale=motion.scale(unsigned(item),a.map_clock_ms);const auto r=MerchantInventoryLayout::bounds(unsigned(item),dy);
        const int w=MerchantInventoryLayout::size(90,scale),h=MerchantInventoryLayout::size(120,scale);
        if(!sold)c.picture(a.content.potion_image((Potion)s.shop_potions[i]),x-w/2+motion.wiggle(unsigned(item),a.map_clock_ms),MerchantInventoryLayout::PotionY+dy-h/2,w,h);
        merchant_price(c,item,x,MerchantInventoryLayout::PotionY+dy+MerchantInventoryLayout::size(48,scale),scale);
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
    merchant_price(c,ShopRemove,MerchantInventoryLayout::RemoveX,MerchantInventoryLayout::RemoveY+dy+MerchantInventoryLayout::size(177,remove_scale),remove_scale);
    if(focus>=0&&focus<ShopCardCount)draw_stock_card(focus);
    if(a.shop_open){c.hit(remove_rect.x,mx(22,remove_rect.y),remove_rect.w,mx(0,remove_rect.y+remove_rect.h-mx(22,remove_rect.y)),HitKind::Choice,ShopRemove,!removed);
        native_back(c,0,211,31,28,"",true,!a.pointer.visible&&a.cursor==ShopLeave);c.hit(0,209,32,31,HitKind::Choice,ShopLeave);}
    merchant_hand(c);merchant_dialogue(c);
    if(a.shop_open&&focus==ShopRemove&&!removed&&!a.pointer.held){
        char description[1024]{};const char*title=a.content.world_string("merchant_room","MERCHANT.cardRemovalService.title");
        const auto amount=[](const void*context,const char*key)->LocalizedValue{
            const char wanted[]="Amount";unsigned i=0;while(wanted[i]&&key[i]==wanted[i])++i;
            return !wanted[i]&&!key[i]?LocalizedValue{true,*static_cast<const int*>(context),1,nullptr}:LocalizedValue{};
        };
        const int increase=s.world.ascension>=6?50:25;
        LocalizedTemplate(description,sizeof description,amount,&increase).for_language(a.content.language()).format(a.content.world_string("merchant_room","MERCHANT.cardRemovalService.description"));
        source_hover_box(c,title,description,MerchantInventoryLayout::RemoveX-SourceHoverWidth-34,55);
    }
    // NMerchantRelic/Potion expose their complete model hover sets through
    // the shared source-sized layer after the room and its global UI.
}
