// EventOption.HoverTips owns these associations. Hovering reads prepared
// models only; it never generates an offer, advances RNG, or changes a save.
struct EventHoverSpec {OriginalEventId event;const char*option;HoverTipKind kind;const char*key;int16_t amount;uint8_t flags;};
struct RelicHoverSpec {const char*relic;HoverTipKind kind;const char*key;int16_t amount;uint8_t flags;const char*value;};
struct PotionHoverSpec {Potion potion;HoverTipKind kind;const char*key;int16_t amount;uint8_t flags;};
#include "event_hover_catalog.inc"
#include "relic_hover_catalog.inc"
#include "potion_hover_catalog.inc"

static void event_hover_card(const App&a,HoverTipList&out,Card card,bool associated,const EventCardFacts*facts=nullptr){
    if(card.kind>=CardKinds)return;
    HoverTip tip{};tip.kind=HoverTipKind::Card;tip.key=definitions[card.kind].key;tip.card=card;tip.smart=true;
    if(facts){tip.facts=*facts;tip.has_facts=true;}out.add(tip);
    if(associated){const EventCardFacts canonical{};hover_collect(a,card,out,-1,-1,facts?facts:&canonical,0);}
}
static void event_hover_potion(const App&,HoverTipList&,Potion,bool=false);
static void event_hover_relic(const App&,HoverTipList&,RelicId,bool,unsigned=0,bool=false);
static void event_hover_spec(const App&a,HoverTipList&out,HoverTipKind kind,const char*key,int amount,unsigned flags,unsigned depth=0,bool historical=false){
    if(kind==HoverTipKind::Card)event_hover_card(a,out,{world_card_id(key),uint8_t((flags&4)!=0)},(flags&2)!=0);
    else if(kind==HoverTipKind::Relic)event_hover_relic(a,out,world_relic_id(key),(flags&1)==0,depth+1,historical);
    else if(kind==HoverTipKind::Enchantment)hover_enchantment(out,key,amount);
    else if(kind==HoverTipKind::Potion){
        for(unsigned i=1;i<=PotionKinds;++i){const auto*d=potion_definition(Potion(i));if(d&&wt_equal(d->key,key)){event_hover_potion(a,out,Potion(i));break;}}
    }else hover_add(out,kind,key,amount);
}
static void event_hover_potion(const App&a,HoverTipList&out,Potion potion,bool associated){
    if(const auto*d=potion_definition(potion))hover_add(out,HoverTipKind::Potion,d->key,int(potion));
    if(associated)for(const auto&spec:potion_hover_specs)if(spec.potion==potion)
        event_hover_spec(a,out,spec.kind,spec.key,spec.amount,spec.flags);
}
static void event_hover_relic(const App&a,HoverTipList&out,RelicId id,bool include_self,unsigned depth,bool historical){
    if(id>=WorldRelicKinds||depth>2)return;const char*key=world_relic_key(id);
    if(include_self)hover_add(out,HoverTipKind::Relic,key,int(id));
    for(const auto&spec:relic_hover_specs)if(wt_equal(key,spec.relic)){
        int amount=spec.amount;
        if(*spec.value){char model[128]{};TextBuffer m{model,sizeof model,0};m.text("relics.");m.text(key);
            if(historical||!world_relic_description_value(a.game,id,spec.value,amount))
                for(const auto&value:world_text_numbers)if(wt_equal(model,value.model)&&wt_equal(spec.value,value.name)){amount=value.base;break;}
        }
        event_hover_spec(a,out,spec.kind,spec.key,amount,spec.flags,depth,historical);
    }
    // Serializable history currently retains IDs only. Prepared or owned
    // models in a different live run cannot reconstruct these old facts.
    if(historical)return;
    const char*names[2]{};unsigned count=0;
    if(id==WorldRelicArchaicTooth){names[count++]="StarterCard";names[count++]="AncientCard";}
    else if(id==WorldRelicDustyTome)names[count++]="AncientCard";
    else if(id==WorldRelicTouchOfOrobas){names[count++]="StarterRelic";names[count++]="UpgradedRelic";}
    for(unsigned i=0;i<count;++i){OriginalTextReference reference{};
        if(!world_relic_description_reference(a.game,id,names[i],reference))continue;
        if(reference.kind==OriginalTextReferenceKind::Card){
            Card card=card_from_token(reference.model);
            // These relic setters append the stored card's own hover tips
            // before its preview. Dusty Tome previews the upgraded result,
            // while its StringVar and associated tips retain the base model.
            hover_collect(a,card,out,-1,-1,&reference.facts,0);
            if(id==WorldRelicDustyTome)card.upgraded=1;
            event_hover_card(a,out,card,false,&reference.facts);
        }
        else if(reference.kind==OriginalTextReferenceKind::Relic)event_hover_relic(a,out,RelicId(reference.model),true,depth+1);
    }
}
void collect_relic_hover_tips(const App&a,RelicId id,HoverTipList&out,bool historical){out.clear();event_hover_relic(a,out,id,true,0,historical);out.finish();}
void collect_potion_hover_tips(const App&a,Potion potion,HoverTipList&out){out.clear();event_hover_potion(a,out,potion,true);out.finish();}
void collect_event_hover_tips(const App&a,int option,HoverTipList&out){
    out.clear();const auto&s=a.game;const auto&e=s.original_events;
    if(!e.active||s.phase!=Phase::Event||e.finished||option<0||unsigned(option)>=original_event_option_count(s))return;
    if(ui_ancient_event(e.id)){
        // FromRelic deliberately excludes the relic itself: its acquisition
        // description is already in the option. Extra mechanic tips remain.
        if(unsigned(option)<e.offer_count)event_hover_relic(a,out,RelicId(e.offers[option]),false);
        out.finish();return;
    }
    for(const auto&spec:event_hover_specs)if(spec.event==e.id&&wt_equal(spec.option,e.option_keys[option]))
        event_hover_spec(a,out,spec.kind,spec.key,spec.amount,spec.flags);
    // Dynamic associations use the same saved values as the event prose.
    switch(e.id){
    case OriginalEventId::DrowningBeacon:if(option==0)event_hover_potion(a,out,Potion::GlowwaterPotion);break;
    case OriginalEventId::DollRoom:if(e.page&&unsigned(option)<e.offer_count)event_hover_relic(a,out,RelicId(e.offers[option]),true);break;
    case OriginalEventId::FakeMerchant:if(option<6&&unsigned(option)<e.offer_count)event_hover_relic(a,out,RelicId(e.offers[option]),true);break;
    case OriginalEventId::RanwidTheElder:
        if(option==0&&original_event_option_enabled(s,option))event_hover_potion(a,out,Potion(e.values[2]),true);
        else if(option==2&&original_event_option_enabled(s,option))event_hover_relic(a,out,RelicId(e.values[1]),true);break;
    case OriginalEventId::RelicTrader:if(option<3){event_hover_relic(a,out,RelicId(e.values[option]),true);event_hover_relic(a,out,RelicId(e.values[option+3]),true);}break;
    case OriginalEventId::SlipperyBridge:if(option==0&&e.values[1]>=0&&e.values[1]<s.deck_count)
        event_hover_card(a,out,s.deck[e.values[1]],false,&s.event_cards.deck[e.values[1]]);break;
    case OriginalEventId::StoneOfAllTime:if(option==0&&original_event_option_enabled(s,option))event_hover_potion(a,out,Potion(e.values[2]));break;
    case OriginalEventId::TheFutureOfPotions:if(option<3&&e.values[option]>=0)event_hover_potion(a,out,potion_at(s,unsigned(e.values[option])),true);break;
    case OriginalEventId::WelcomeToWongos:if(option==1&&original_event_option_enabled(s,option))event_hover_relic(a,out,RelicId(e.values[0]),true);break;
    case OriginalEventId::EndlessConveyor:if(option==0){
        if(e.values[1]==7)event_hover_card(a,out,{EventCardFeedingFrenzy,0},true);
        else if(e.values[1]==2)hover_add(out,HoverTipKind::Static,"TRANSFORM");
    }break;
    case OriginalEventId::TinkerTime:if(e.page==2){
        auto rider=TinkerRider(e.values[option]);
        if(rider==TinkerRider::Sapping){hover_add(out,HoverTipKind::Power,"WEAK_POWER");hover_add(out,HoverTipKind::Power,"VULNERABLE_POWER");}
        else if(rider==TinkerRider::Choking)hover_add(out,HoverTipKind::Power,"STRANGLE_POWER");
        else if(rider==TinkerRider::Expertise){hover_add(out,HoverTipKind::Power,"STRENGTH_POWER");hover_add(out,HoverTipKind::Power,"DEXTERITY_POWER");}
    }break;
    default:break;
    }
    out.finish();
}
bool event_hover_target(const App&a,CardHoverTarget&target){
    target={};if(!a.game.original_events.active||a.game.phase!=Phase::Event||a.game.original_events.finished||
        a.title||a.library||a.paused||a.help||a.settings_open||a.frontend_confirm||a.character_select||a.map_preview.open||
        a.status_actor>=0||a.detail_item>=0||a.detail_card>=0||a.event_reader||a.panel||a.map_window.is_open||
        a.pointer.held||a.pointer.armed_card>=0||a.map_transition_blocks_pointer())return false;
    const HitRegion*hit=nullptr;
    for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];if(h.kind!=HitKind::Choice||!h.enabled)continue;
        if(a.pointer.visible&&!a.keyword_hover.details){if(hover_in({h.x,h.y,h.w,h.h},a.pointer.x,a.pointer.y))hit=&h;}
        else if(h.value==a.cursor)hit=&h;
    }
    if(!hit||hit->value<0||unsigned(hit->value)>=original_event_option_count(a.game))return false;
    target.event_option=hit->value;target.owner={hit->x,hit->y,hit->w,hit->h};target.detail=a.keyword_hover.details;
    target.identity=0x60000000u|(unsigned(a.game.original_events.id)<<16)|(unsigned(a.game.original_events.page)<<8)|unsigned(hit->value);
    if(a.game.original_events.id==OriginalEventId::FakeMerchant&&hit->value<6){
        // This custom event instantiates the ordinary NMerchantRelic. Its
        // source hover anchor is the122px slot, separate from the large icon.
        const int cx=MerchantInventoryLayout::fake_x(unsigned(hit->value));
        const int cy=MerchantInventoryLayout::fake_y(unsigned(hit->value))+a.room_presentation.merchant.offset(a.map_clock_ms);
        const int side=MerchantInventoryLayout::size(122,800),half=side/2;
        target.merchant=true;target.merchant_left=cx>ScreenW/2;
        target.relic=a.game.original_events.offers[hit->value];target.canonical_relic=true;
        target.owner={cx+(target.merchant_left?-half:half),cy-half,side,side};
    }
    return true;
}
static bool treasure_hover_target(const App&a,CardHoverTarget&target){
    const auto&s=a.game;
    if(!s.world.active||s.phase!=Phase::Treasure||s.treasure_state!=1||s.world.treasure_relic>=WorldRelicKinds||
       a.title||a.paused||a.help||a.settings_open||a.frontend_confirm||a.library||a.panel||a.map_window.visible||
       a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||a.pointer.held||a.map_transition_blocks_pointer())return false;
    const HitRegion*hit=nullptr;
    for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];if(h.kind!=HitKind::UiAction||h.value!=int(UiAction::TreasureTake)||!h.enabled)continue;
        if(a.pointer.visible?hover_in({h.x,h.y,h.w,h.h},a.pointer.x,a.pointer.y):a.cursor==0){hit=&h;break;}}
    if(!hit)return false;
    target={};target.relic=int(s.world.treasure_relic);target.owner={hit->x,hit->y,hit->w,hit->h};
    target.identity=0x71000000u|unsigned(s.world.treasure_relic);return true;
}
static bool merchant_stock_hover_target(const App&a,const State&s,int slot,CardHoverTarget&target){
    if(slot<ShopRelicStart||slot>=ShopRemove||(s.shop_sold&(1u<<slot)))return false;
    target={};target.merchant=true;
    if(slot<ShopPotionStart){
        if(s.world.active)target.relic=s.world.shop_relics[slot-ShopRelicStart];
        else for(int i=0;i<RelicKinds;++i)if(relic_definitions[i].id==s.shop_relics[slot-ShopRelicStart]){target.relic=i;break;}
        if(target.relic<0||target.relic>=WorldRelicKinds)return false;
        // Stock is a detached model, even when a live run owns that relic.
        target.canonical_relic=true;
    }else{
        target.potion=s.shop_potions[slot-ShopPotionStart];
        if(!potion_definition(Potion(target.potion)))return false;
    }
    // NMerchantRelic/Potion use the slot's122px Control at its focused0.8
    // scale, not its enlarged Hitbox. The ordinary source hover set is a
    // separate global UI layer and keeps its69px text columns.
    const int cx=MerchantInventoryLayout::item_x(unsigned(slot-ShopRelicStart)%3);
    const int cy=(slot<ShopPotionStart?MerchantInventoryLayout::RelicY:MerchantInventoryLayout::PotionY)+a.room_presentation.merchant.offset(a.map_clock_ms);
    const int size=MerchantInventoryLayout::size(122,800),half=size/2;
    target.merchant_left=cx>ScreenW/2;
    target.owner={cx+(target.merchant_left?-half:half),cy-half,size,size};
    target.identity=0x75000000u|(unsigned(slot)<<16)|unsigned(target.relic>=0?target.relic:target.potion);
    return true;
}
bool merchant_hover_target(const App&a,CardHoverTarget&target){
    const auto&s=a.game;
    if(s.phase!=Phase::Shop||!a.shop_open||a.title||a.paused||a.help||a.settings_open||a.frontend_confirm||
       a.library||a.panel||a.map_window.visible||a.map_preview.open||a.event_reader||a.character_select||
       a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||a.pointer.held||a.pointer.armed_card>=0||
       a.ftue.active()||a.map_transition_blocks_pointer())return false;
    for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
        if(h.kind!=HitKind::Choice||!h.enabled||h.value<ShopRelicStart||h.value>=ShopRemove||
           (s.shop_sold&(1u<<h.value)))continue;
        if(a.pointer.visible?hover_in({h.x,h.y,h.w,h.h},a.pointer.x,a.pointer.y):h.value==a.cursor)
            return merchant_stock_hover_target(a,s,h.value,target);
    }
    return false;
}
bool owned_relic_hover_target(const App&a,CardHoverTarget&target){
    if(a.event_reader||a.pointer.held||a.title||a.paused||a.help||a.settings_open||a.frontend_confirm||
       (a.panel&&a.panel!=3)||a.library||a.character_select||a.map_preview.open||a.status_actor>=0||
       a.detail_card>=0||a.detail_item>=0||a.deck_candidate>=0||choice_phase(a.game.phase)||a.ftue.active())return false;
    const HitRegion*hit=a.pointer.visible?a.hit_at(a.pointer.x,a.pointer.y):nullptr;
    if(!a.pointer.visible&&a.hud_focus>=HudRelicFocusFirst)for(int i=0;i<a.hit_count;++i)
        if(a.hits[i].kind==HitKind::OwnedRelic&&a.hits[i].value==a.hud_focus-HudRelicFocusFirst){hit=&a.hits[i];break;}
    if(!hit||!hit->enabled||hit->kind!=HitKind::OwnedRelic||hit->value<0||hit->value>=int(ui_owned_relic_count(a.game)))return false;
    target={};target.relic=ui_owned_relic_id(a.game,hit->value);target.owned_relic=hit->value;
    target.owner={hit->x,hit->y,hit->w,hit->h};target.identity=0x76000000u|(unsigned(hit->value)<<16)|unsigned(target.relic);return true;
}
static bool collection_hover_target(const App&a,CardHoverTarget&target){
    if(!a.library||(a.library_tab!=1&&a.library_tab!=2)||a.pointer.held||a.detail_item>=0||a.detail_card>=0||a.paused||a.help||a.settings_open||a.frontend_confirm)return false;
    const HitRegion*hit=nullptr;
    for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];if(h.kind!=HitKind::LibraryItem||!h.enabled)continue;
        if(a.pointer.visible?hover_in({h.x,h.y,h.w,h.h},a.pointer.x,a.pointer.y):h.value==a.library_selected){hit=&h;break;}}
    if(!hit||hit->value<0||hit->value>=int(a.collection.count))return false;
    const int id=a.collection.ids[hit->value];if(!a.library_item_seen(a.library_tab,id))return false;
    target={};target.owner={hit->x,hit->y,hit->w,hit->h};
    if(a.library_tab==1)target.relic=id;else target.potion=id+1;
    target.identity=0x72000000u|(unsigned(a.library_tab)<<20)|unsigned(id);return true;
}
static bool history_relic_hover_target(const App&a,CardHoverTarget&target){
    if(!a.history_open||a.pointer.held||a.detail_item>=0||a.detail_card>=0||a.paused||a.help||a.settings_open||a.frontend_confirm)return false;
    const auto*run=a.history_at(a.history_index);if(!run)return false;
    const HitRegion*hit=nullptr;
    for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];if(h.kind!=HitKind::HistoryRelic||!h.enabled)continue;
        if(a.pointer.visible?hover_in({h.x,h.y,h.w,h.h},a.pointer.x,a.pointer.y):a.history_presentation.hover==UiHistoryLayout::RelicFirst+h.value){hit=&h;break;}}
    if(!hit||hit->value<0||hit->value>=run->relic_count)return false;
    target={};target.relic=run->relics[hit->value];target.historical=true;target.owner={hit->x,hit->y,hit->w,hit->h};
    target.identity=0x73000000u|((run->serial&0xffffu)<<8)|unsigned(hit->value);return true;
}
static bool inspect_relic_hover_target(const App&a,CardHoverTarget&target){
    if(a.detail_item<0||a.detail_item>=WorldRelicKinds||!a.relic_inspect.bound||a.relic_inspect.exiting||a.settings_open||a.frontend_confirm)return false;
    const bool collection=a.relic_inspect.source==RelicInspectSource::Collection;
    if(collection&&!a.library_item_seen(1,a.detail_item))return false;
    // NInspectRelicScreen aligns to its stationary HoverTipRect, not the
    // entering popup or pointer. Related cards are on the opposite side.
    target={};target.relic=a.detail_item;target.owner={relic_inspect_layout::PopupX,relic_inspect_layout::PopupY,relic_inspect_layout::PopupWidth,relic_inspect_layout::PopupHeight};target.detail=true;
    target.inspect_relic=true;target.canonical_relic=collection||a.history_open;target.historical=a.history_open;
    target.identity=0x74000000u|unsigned(a.detail_item)|(unsigned(a.detail_owned+1)<<12);return true;
}
static bool keyword_hover_target(const App&a,CardHoverTarget&target){return inspect_relic_hover_target(a,target)||history_relic_hover_target(a,target)||power_hover_target(a,target)||creature_hover_target(a,target)||collection_hover_target(a,target)||owned_relic_hover_target(a,target)||treasure_hover_target(a,target)||merchant_hover_target(a,target)||event_hover_target(a,target)||card_hover_target(a,target);}
static void collect_keyword_hover_tips(const App&a,const CardHoverTarget&owner,HoverTipList&out){
    if(owner.creature>=0)collect_creature_hover_tips(a,owner,out);
    else if(owner.power>=0)collect_power_hover_tips(a,uint16_t(owner.power),out);
    else if(owner.relic>=0){
        if(owner.inspect_relic){out.clear();event_hover_relic(a,out,RelicId(owner.relic),false,0,owner.historical||owner.canonical_relic);out.finish();}
        else collect_relic_hover_tips(a,RelicId(owner.relic),out,owner.historical||owner.canonical_relic);
    }
    else if(owner.potion>0)collect_potion_hover_tips(a,Potion(owner.potion),out);
    else if(owner.event_option>=0)collect_event_hover_tips(a,owner.event_option,out);
    else collect_card_hover_tips(a,owner.card,out,owner.instance,owner.deck,owner.card_facts());
    if(hover_enchant_selection(a,owner)){
        // NDeckEnchant's fixed description and a hovered card share the
        // reserved right column on the LCD. Keep the pending enchantment
        // once, followed by the card's complete associated set.
        int amount=0;const auto enchantment=a.world_selection_enchantment(&amount);
        const char*key=event_enchantment_key(enchantment);HoverTipList combined;
        hover_enchantment(combined,key,amount);
        for(unsigned i=0;i<out.count;++i)if(out.items[i].kind!=HoverTipKind::Enchantment||!hover_key_equal(out.items[i].key,key))combined.add(out.items[i]);
        combined.overflow|=out.overflow;combined.finish();out=combined;
    }
}
static HoverRect keyword_hover_column(const CardHoverTarget&owner){
    if(owner.merchant){const int x=owner.merchant_left?owner.owner.x-SourceHoverWidth:owner.owner.x+owner.owner.w;
        return {mx(3,mn(317-SourceHoverWidth,x)),24,SourceHoverWidth,194};}
    if(owner.power>=0||owner.creature>=0){const int x=owner.power_left?owner.owner.x-SourceHoverWidth-1:owner.owner.x+owner.owner.w+1;
        return {mx(3,mn(317-SourceHoverWidth,x)),24,SourceHoverWidth,194};}
    if(owner.event_option>=0&&owner.owner.w>240)
        return {3,24,SourceHoverWidth,mx(56,owner.owner.y-42)};
    if(owner.event_option>=0&&owner.owner.w>=160&&!owner.detail)
        return {8,24,SourceHoverWidth,194};
    if(owner.relic>=0){const int x=owner.owner.x<240?owner.owner.x:owner.owner.x+owner.owner.w-SourceHoverWidth;return {mx(3,mn(317-SourceHoverWidth,x)),24,SourceHoverWidth,194};}
    return card_hover_column(owner.owner);
}
