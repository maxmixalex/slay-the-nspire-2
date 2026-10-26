// Card hover ownership and content follow the pinned CardModel/NHoverTipSet.
// LCD text uses readable10px columns, and pages when the available viewport
// cannot contain the associated set. It adds no RNG calls or save fields.
struct HoverTipSpec {HoverTipKind kind;const char*key;uint8_t flags;};
struct CardHoverCatalog {uint16_t first;uint8_t count,flags;uint32_t order;};
#include "keyword_hover_catalog.inc"
#include "power_hover_catalog.inc"

static bool hover_power_debuff(const char*key){
    for(unsigned id=0;id<StatusKinds;++id)if((CombatStatus::PowerFlags[id]&2)&&hover_key_equal(ui_status_key(int(id)),key))return true;
    for(auto name:hover_debuff_keys)if(hover_key_equal(name,key))return true;return false;
}
static void hover_add(HoverTipList&out,HoverTipKind kind,const char*key,int amount=0,int actor=-1,bool smart=false){
    HoverTip tip{};tip.kind=kind;tip.key=key;tip.amount=int16_t(amount);tip.actor=int16_t(actor);tip.smart=smart;
    tip.debuff=kind==HoverTipKind::Affliction||(kind==HoverTipKind::Power&&hover_power_debuff(key));out.add(tip);
}
static void hover_word(HoverTipList&out,const char*key){hover_add(out,HoverTipKind::Keyword,key);}
static void hover_enchantment(HoverTipList&out,const char*key,int amount){
    hover_add(out,HoverTipKind::Enchantment,key,amount);
    if(hover_key_equal(key,"INKY"))hover_add(out,HoverTipKind::Power,"WEAK_POWER");
    else if(hover_key_equal(key,"GOOPY")||hover_key_equal(key,"SOULS_POWER"))hover_word(out,"EXHAUST");
    else if(hover_key_equal(key,"STEADY"))hover_word(out,"RETAIN");
    else if(hover_key_equal(key,"ROYALLY_APPROVED")){hover_word(out,"INNATE");hover_word(out,"RETAIN");}
    else if(hover_key_equal(key,"TEZCATARAS_EMBER"))hover_word(out,"ETERNAL");
    else if(hover_key_equal(key,"SPIRAL")||hover_key_equal(key,"GLAM"))hover_add(out,HoverTipKind::Static,"REPLAY_DYNAMIC",1);
}
static void hover_collect(const App&a,Card card,HoverTipList&out,int instance,int deck,const EventCardFacts*facts,unsigned depth){
    if(card.kind>=CardKinds||depth>2)return;const auto&s=a.game;const auto&row=card_hover_catalog[card.kind];
    const bool live=!a.library&&instance>=0&&instance<s.card_count&&s.cards[instance].kind==card.kind;
    if(!facts&&!a.library)facts=event_card_facts(s,card,instance,deck);
    for(unsigned i=0;i<row.count;++i){const auto&spec=card_hover_specs[row.first+i];
        if(spec.kind==HoverTipKind::Special){
            if(hover_key_equal(spec.key,"DEFEND_OWNER")){
                static const CardKind defends[]={Defend,DefendSilent,DefectDefend,RegentDefend,NecroDefendNecrobinder};
                HoverTip tip{};tip.kind=HoverTipKind::Card;tip.card={a.library?CardKind(Defend):defends[unsigned(s.character)<5?unsigned(s.character):0],0};
                tip.key=definitions[tip.card.kind].key;tip.smart=true;out.add(tip);
            }else if(hover_key_equal(spec.key,"SANDPIT_POWER")){
                int actor=-1,amount=0;if(!a.library&&s.enemy_world.active)for(int e=0;e<s.enemy_count;++e)
                    if(s.enemies[e].hp>0&&s.enemy_world.models[e].power[unsigned(EnemyPower::Sandpit)]>0){actor=e+1;amount=s.enemy_world.models[e].power[unsigned(EnemyPower::Sandpit)];break;}
                hover_add(out,HoverTipKind::Power,"SANDPIT_POWER",amount,actor,actor>=0);
            }else if(hover_key_equal(spec.key,"TINKER_RIDER")&&facts){
                if(facts->rider==unsigned(TinkerRider::Sapping)){hover_add(out,HoverTipKind::Power,"WEAK_POWER");hover_add(out,HoverTipKind::Power,"VULNERABLE_POWER");}
                else if(facts->rider==unsigned(TinkerRider::Choking))hover_add(out,HoverTipKind::Power,"STRANGLE_POWER");
                else if(facts->rider==unsigned(TinkerRider::Expertise)){hover_add(out,HoverTipKind::Power,"STRENGTH_POWER");hover_add(out,HoverTipKind::Power,"DEXTERITY_POWER");}
            }
        }else if(spec.kind==HoverTipKind::Card){
            const CardKind id=world_card_id(spec.key);if(id>=CardKinds)continue;
            HoverTip tip{};tip.kind=HoverTipKind::Card;tip.key=spec.key;tip.card={id,uint8_t((spec.flags&1)?card.upgraded:0)};tip.smart=true;out.add(tip);
            if(spec.flags&2){const EventCardFacts canonical{};hover_collect(a,tip.card,out,-1,-1,&canonical,depth+1);}
        }else if(spec.kind==HoverTipKind::Enchantment)hover_enchantment(out,spec.key,1);
        else{
            int amount=0;if(spec.kind==HoverTipKind::Static&&hover_key_equal(spec.key,"SUMMON_DYNAMIC"))
                character_description_value(s,card,-1,live?instance:-1,"Summon",amount);
            hover_add(out,spec.kind,spec.key,amount);
        }
    }
    // Extras are followed by the current card's enchantment and affliction.
    if(facts&&facts->enchantment!=EventEnchantment::None)hover_enchantment(out,event_enchantment_key(facts->enchantment),facts->enchantment_amount);
    unsigned affliction=0;
    if(live){
        if(s.silent.inky[instance])hover_enchantment(out,"INKY",1);
        affliction=s.enemy_world.affliction[instance];
        static const char* const keys[]={"","BOUND","ENTANGLED","GALVANIZED","HEXED","TAINTED","RINGING","SMOG"};
        if(affliction&&affliction<sizeof keys/sizeof*keys){hover_add(out,HoverTipKind::Affliction,keys[affliction],s.enemy_world.affliction_amount[instance]);
            if(affliction==unsigned(EnemyAffliction::Hexed))hover_word(out,"ETHEREAL");
            if(affliction==unsigned(EnemyAffliction::Tainted))hover_add(out,HoverTipKind::Power,"TAINTED_POWER");
        }
    }
    int replay=live?s.expansion.replay[instance]:0;
    if(facts&&(facts->enchantment==EventEnchantment::Spiral||
       (facts->enchantment==EventEnchantment::Glam&&(!live||!s.event_cards.enchantment_disabled[instance]))))++replay;
    if(replay>0)hover_add(out,HoverTipKind::Static,"REPLAY_DYNAMIC",replay);
    if(row.flags&1)hover_add(out,HoverTipKind::Static,"EVOKE");
    bool block=world_card_gains_block(card.kind);
    if(card.kind==EventCardMadScience)block=facts&&facts->science_type==unsigned(CardType::Skill);
    if(card.kind==SovereignBlade)block=live&&s.regent.powers[unsigned(RPower::Parry)]>0;
    if(block)hover_add(out,HoverTipKind::Static,"BLOCK");
    const bool canonical=a.library||depth||(!live&&deck<0);
    unsigned words=canonical?(card.upgraded?definitions[card.kind].upgraded_keywords:definitions[card.kind].keywords):card_keywords(s,card,live?instance:-1,deck);
    if(canonical){if(definitions[card.kind].exhausts)words|=1;
        if(card.kind==Purity||(card.kind==Scrawl&&card.upgraded))words|=4;
        if(card.kind==Backstab||(card.kind==InfiniteBlades&&card.upgraded))words|=2;
    }
    // Detached reward facts use the same canonical mutation rules as a deck
    // instance; no temporary card is inserted into the gameplay state.
    if(facts){switch(facts->enchantment){
        case EventEnchantment::Goopy:words|=1;break;
        case EventEnchantment::SoulsPower:words&=~1u;break;
        case EventEnchantment::Steady:words|=4;break;
        case EventEnchantment::RoyallyApproved:words|=6;break;
        case EventEnchantment::TezcatarasEmber:words|=64;break;
        default:break;
    }}
    if(live){if(s.silent.sly[instance])words|=16;if(s.silent.retain[instance])words|=4;
        if(s.necro.ethereal[instance]||affliction==unsigned(EnemyAffliction::Hexed))words|=8;}
    static const char* const word_keys[]={"","EXHAUST","INNATE","RETAIN","ETHEREAL","SLY","UNPLAYABLE","ETERNAL"};
    static const unsigned word_bits[]={0,1,2,4,8,16,128,64};
    auto add_word=[&](unsigned n){if(n<1||n>7)return;unsigned bit=word_bits[n];if(!(words&bit))return;words&=~bit;
        hover_word(out,word_keys[n]);if(n==4)hover_word(out,"EXHAUST");};
    for(uint32_t order=row.order;order;order>>=4)add_word(order&15);
    // Persistent mechanics store keyword bits, not the historical order in
    // which effects were applied. New local keywords use one stable tail.
    const unsigned added_order[]={5,3,4,2,6,1,7};for(auto n:added_order)add_word(n);
}
void collect_card_hover_tips(const App&a,Card card,HoverTipList&out,int instance,int deck,const EventCardFacts*facts){
    out=HoverTipList{};hover_collect(a,card,out,instance,deck,facts,0);out.finish();
}
#include "power_hover_ui.inc"

static bool hover_in(const HoverRect&r,int x,int y){return x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;}
static const HitRegion* hover_keyboard_hit(const App&a){
    if(a.library&&a.library_tab==0){for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::LibraryItem&&a.hits[i].value==a.library_selected)return &a.hits[i];}
    if(a.panel==1&&a.pile_control_focus<0){int ids[MaxCards];const unsigned count=a.pile_items(ids);if(unsigned(a.inspect_cursor)<count)for(int i=0;i<a.hit_count;++i)if((a.hits[i].kind==HitKind::DeckCard||a.hits[i].kind==HitKind::InstanceCard)&&a.hits[i].value==ids[a.inspect_cursor])return &a.hits[i];}
    if(!a.panel&&!a.library){for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
        if(h.kind==HitKind::Choice&&h.value==a.cursor)return &h;
        if(h.kind==HitKind::CombatChoice){const auto&source=choice_pile(a.game);if(a.cursor>=0&&a.cursor<source.count&&h.value==source.ids[a.cursor])return &h;}
        if(h.kind==HitKind::WorldPreviewCard&&h.value==a.cursor)return &h;
        if(h.kind==HitKind::WorldChoice){uint16_t ids[MaxWorldUiChoices];const unsigned count=a.world_choice_options(ids,MaxWorldUiChoices);if(a.cursor>=0&&unsigned(a.cursor)<count&&h.value==ids[a.cursor])return &h;}
    }}return nullptr;
}
bool card_hover_target(const App&a,CardHoverTarget&target){
    target={};if(a.paused||a.help||a.settings_open||a.frontend_confirm||a.character_select||a.map_preview.open||a.status_actor>=0||
        a.detail_item>=0||a.event_reader||a.pointer.held||a.pointer.armed_card>=0||a.keyboard_card>=0||a.map_transition_blocks_pointer())return false;
    if(a.library&&!a.library_tab&&!a.pointer.visible&&a.card_library.control>=0)return false;
    if(a.detail_card>=0){
        target.card=card_from_token(a.detail_card);target.instance=a.detail_instance;target.deck=a.detail_deck_index;
        target.facts=a.detail_has_facts?&a.detail_facts:nullptr;
        target.owner={card_inspect_layout::TipX,card_inspect_layout::TipY,card_inspect_layout::TipW,card_inspect_layout::TipH};target.detail=true;
        target.identity=0x40000000u|uint32_t(a.detail_card);
        // NInspectCardScreen creates its hover-tip set when the displayed
        // card changes, independent of mouse location or a second hotkey.
        return target.card.kind<CardKinds;
    }
    if((a.title&&!a.library)||a.game_over_summary||(a.panel&&a.panel!=1)||a.map_window.is_open)return false;
    if(a.deck_candidate>=0){
        if(!a.deck_selection_active()||!a.deck_candidate_valid())return false;
        using namespace deck_selection_layout;
        const bool upgrade=a.game.phase==Phase::Upgrade;
        const int x=upgrade&&a.pointer.visible&&a.pointer.x>=AfterX?AfterX:upgrade?BeforeX:SingleX;
        if(a.pointer.visible&&(a.pointer.x<x||a.pointer.x>=x+PreviewW||a.pointer.y<PreviewY||a.pointer.y>=PreviewY+PreviewH))return false;
        target.deck=a.deck_candidate;target.card=a.game.deck[target.deck];
        if(upgrade&&x==AfterX)target.card.upgraded=1;
        target.owner={x,PreviewY,PreviewW,PreviewH};target.identity=0x310000u|unsigned(target.deck)|(unsigned(target.card.upgraded)<<12);return true;
    }
    const HitRegion*h=a.pointer.visible?a.hit_at(a.pointer.x,a.pointer.y):hover_keyboard_hit(a);
    const auto hand=a.combat_hand();
    if(!a.library&&!a.panel&&a.hand_can_focus()&&a.hand_focus&&a.cursor>=0&&a.cursor<hand.count&&
       (!a.pointer.visible||(h&&h->kind==HitKind::Hand))){
        target.instance=hand.ids[a.cursor];target.card=a.game.cards[target.instance];auto pose=a.combat_hand_pose(unsigned(a.cursor),true);
        const auto bounds=hand_bounds(pose);target.owner={bounds.x,bounds.y,bounds.w,bounds.h};target.identity=0x10000u|unsigned(target.instance);return true;
    }
    if(!h)return false;target.owner={h->x,h->y,h->w,h->h};target.identity=0x100000u|(unsigned(h->kind)<<16)|unsigned(uint16_t(h->value));
    const auto&s=a.game;
    switch(h->kind){
    case HitKind::LibraryItem:{if(!a.library||a.library_tab!=0)return false;CardKind ids[CardKinds];unsigned n=a.library_items(ids);if(h->value<0||unsigned(h->value)>=n||!a.card_library.was_seen(ids[h->value]))return false;
        target.card={ids[h->value],uint8_t(a.library_upgraded&&card_upgradeable({ids[h->value],0}))};break;}
    case HitKind::DeckCard:if(h->value<0||h->value>=s.deck_count)return false;target.deck=h->value;target.card=s.deck[h->value];if(a.panel==1&&!a.pile_tab&&a.pile_upgraded&&card_upgradeable(target.card))target.card.upgraded=1;break;
    case HitKind::InstanceCard:if(h->value<0||h->value>=s.card_count)return false;target.instance=h->value;target.card=s.cards[h->value];break;
    case HitKind::CombatChoice:
        if(h->value<0)return false;
        if(a.combat_selection_kind()==CombatSelectionKind::Hand)
            for(unsigned i=0;i<a.choice_selected_count;++i)if(a.choice_selected[i]==h->value)return false;
        target.card=choice_card(s,h->value);target.instance=h->value<s.card_count?h->value:-1;
        if(a.combat_selection_kind()==CombatSelectionKind::HandUpgrade&&a.choice_selected_count&&a.choice_selected[0]==h->value){
            if(!a.pointer.visible||a.pointer.x<183||a.pointer.x>=234||a.pointer.y<74||a.pointer.y>=146)return false;
            target.card.upgraded=1;target.owner={183,74,51,72};}
        break;
    case HitKind::WorldPreviewCard:{WorldSelectionCard model{};if(h->value<0||!a.world_selection_preview_card(unsigned(h->value),model)||!model.hover)return false;
        target.card=model.card;target.deck=model.deck;target.preview_facts=model.facts;target.has_preview_facts=true;break;}
    case HitKind::WorldChoice:{
        if(h->value<0)return false;const unsigned id=unsigned(h->value);
        if(s.phase==Phase::WorldRelicChoice){const auto&w=s.world_relics;
            if(w.choice==WorldRelicChoice::Hand){if(id>=s.card_count)return false;
                if(combat_selection_hand(a.combat_selection_kind()))for(unsigned i=0;i<a.choice_selected_count;++i)if(a.choice_selected[i]==id)return false;
                target.instance=int(id);target.card=s.cards[id];
            }else if(w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::CombatOffer){if(id>=w.offer_count)return false;target.card=card_from_token(w.offers[id]);target.facts=&w.offer_facts[id];}
            else if(w.choice==WorldRelicChoice::OfferBundle)return false;
            else if(a.world_selection_kind()!=WorldSelectionKind::None&&id<s.deck_count){target.deck=int(id);target.card=s.deck[id];}
            else return false;
        }else if(s.phase==Phase::OriginalEventChoice){const auto&e=s.original_events;
            if(e.selection==OriginalEventSelection::OfferCard){if(id>=e.offer_count)return false;target.card=card_from_token(e.offers[id]);target.facts=&e.offer_facts[id];}
            else if(a.world_selection_kind()!=WorldSelectionKind::None&&id<s.deck_count){target.deck=int(id);target.card=s.deck[id];}
            else return false;
        }else return false;
        if(a.world_selection_upgrades()&&a.deck_selection.show_upgrades&&card_upgradeable(target.card))target.card.upgraded=1;
        break;}
    case HitKind::Choice:
        if(h->value<0)return false;
        if(s.phase==Phase::Reward&&a.reward_cards&&!a.grimoire_select){unsigned n=s.world.active?s.world.reward_count:3;if(unsigned(h->value)>=n)return false;
            target.card=s.world.active?s.world.reward_cards[h->value]:Card{s.reward[h->value],0};target.facts=s.world.active?&s.world.reward_facts[h->value]:nullptr;
        }else if(s.phase==Phase::Shop&&a.shop_open&&h->value<ShopCardCount){
            target.card=s.world.active?s.world.shop_cards[h->value]:Card{s.shop_cards[h->value],0};target.facts=s.world.active?&s.world.shop_facts[h->value]:nullptr;
        }else if(s.phase==Phase::Upgrade||s.phase==Phase::Remove||s.phase==Phase::Transform||a.grimoire_select){
            if(h->value>=s.deck_count)return false;target.deck=h->value;target.card=s.deck[h->value];
            if(s.phase==Phase::Upgrade&&a.deck_selection.show_upgrades&&card_upgradeable(target.card))target.card.upgraded=1;
        }else return false;break;
    default:return false;
    }
    target.identity^=uint32_t(card_token(target.card))<<1;return target.card.kind<CardKinds;
}

static bool hover_enchant_selection(const App&a,const CardHoverTarget&owner){
    return !a.panel&&!a.library&&!owner.detail&&owner.event_option<0&&owner.relic<0&&!owner.potion&&
        owner.power<0&&owner.creature<0&&a.world_selection_active()&&a.world_selection_kind()==WorldSelectionKind::Enchant;
}
#include "event_hover_ui.inc"

struct HoverTextContext {WorldTextContext world;const HoverTip*tip;};
static LocalizedValue hover_text_value(const void*opaque,const char*name){
    const auto&x=*static_cast<const HoverTextContext*>(opaque);
    if(wt_equal(name,"Summon")||wt_equal(name,"Times"))return wt_number(x.tip->amount);
    return wt_resolve(&x.world,name);
}
void format_card_hover_tip(const App&a,const CardHoverTarget&owner,const HoverTip&tip,char*title,size_t title_cap,char*body,size_t body_cap){
    if(title_cap)title[0]=0;if(body_cap)body[0]=0;
    if((tip.power>=0||owner.power>=0)&&tip.kind==HoverTipKind::Power&&tip.instanced){
        const auto token=uint16_t(tip.power>=0?tip.power:owner.power);const int id=int(power_id(token)),actor=int(power_actor(token));
        TextBuffer name{title,title_cap,0};name.text(ui_status_title(a,id));
        ui_status_description(a,actor,id,body,body_cap,power_instanced(unsigned(id))?int(power_instance(token)):-1);return;
    }
    if(tip.kind==HoverTipKind::Intent){
        TextBuffer name{title,title_cap,0};name.text(a.content.world_title("intents",tip.key));
        char key[80]{};TextBuffer path{key,sizeof key,0};path.text(tip.key);path.text(".description");
        a.localized_text("intents",key,body,body_cap,tip.actor-1,tip.actor);return;
    }
    if(tip.kind==HoverTipKind::Description){
        if(owner.event_option>=0){native_event_option(a,owner.event_option,false,title,title_cap);native_event_option(a,owner.event_option,true,body,body_cap);return;}
        TextBuffer name{title,title_cap,0,&a.content};name.text(a.content.card_string(owner.card.kind));if(owner.card.upgraded)name.ch('+');
        a.card_description(owner.card,a.card_preview_target(owner.instance),body,body_cap,owner.instance,owner.deck,owner.card_facts());return;
    }
    if(tip.kind==HoverTipKind::Card){
        TextBuffer name{title,title_cap,0,&a.content};name.text(a.content.card_string(tip.card.kind));if(tip.card.upgraded)name.ch('+');
        const EventCardFacts canonical{};a.card_description(tip.card,-1,body,body_cap,tip.amount>0?tip.amount-1:-1,-1,tip.has_facts?&tip.facts:&canonical);return;
    }
    if(tip.kind==HoverTipKind::Relic){
        const RelicId id=world_relic_id(tip.key);const int position=owner.historical||owner.canonical_relic?int(WorldRelicCapacity):owner.relic==int(id)?owner.owned_relic:-1;
        TextBuffer name{title,title_cap,0};name.text(a.relic_title(id,position));
        if(owner.owned_relic>=0&&owner.relic==int(id)){
            char progress[128]{},description[2048]{};ui_relic_progress(a.game,position,progress,sizeof progress,&a.content);a.relic_description(id,description,sizeof description,position);
            TextBuffer text{body,body_cap,0};if(*progress){text.text("[green]");text.text(progress);text.text("[/green]\n");}text.text(description);
        }else a.relic_description(id,body,body_cap,position);return;
    }
    if(tip.kind==HoverTipKind::Potion){
        TextBuffer name{title,title_cap,0};name.text(potion_name(tip.amount,&a.content));ui_potion_description(a,tip.amount,body,body_cap);return;
    }
    const char*table=tip.kind==HoverTipKind::Keyword?"card_keywords":tip.kind==HoverTipKind::Power?"powers":
        tip.kind==HoverTipKind::Orb?"orbs":tip.kind==HoverTipKind::Enchantment?"enchantments":tip.kind==HoverTipKind::Affliction?"afflictions":"static_hover_tips";
    int model=-1;if(tip.kind==HoverTipKind::Power)for(unsigned i=0;i<EnemyPowerCount;++i)if(wt_equal(tip.key,world_enemy_power_keys[i])){model=int(i);break;}
    HoverTextContext context{{&a,table,{},model,tip.actor,tip.amount,owner.card,owner.instance,owner.deck,owner.card_facts()},&tip};
    auto format=[&](const char*suffix,char*out,size_t cap){if(!cap)return;char key[160]{},raw[4096]{};TextBuffer k{key,sizeof key,0};k.text(tip.key);k.text(suffix);
        wt_model_key(context.world,key);TextBuffer source{raw,sizeof raw,0};source.text(a.content.world_string(table,key));LocalizedTemplate(out,cap,hover_text_value,&context,a.content.world_title("static_hover_tips","ENERGY_COUNT"),a.content.world_title("static_hover_tips","STAR_COUNT"),a.content.english()).for_language(a.content.language()).format(raw);};
    format(".title",title,title_cap);format(tip.smart?".smartDescription":".description",body,body_cap);
    if(body_cap&&!*body&&tip.smart)format(".description",body,body_cap);
}

static bool hover_compact_event(const CardHoverTarget&owner){return owner.event_option>=0&&owner.owner.w>240&&!owner.detail;}
static HoverPage hover_page(Canvas&c,const CardHoverTarget&owner,const HoverTipList&tips,int selected){
    const bool details=owner.detail&&c.a.keyword_hover.details,inspect=owner.detail&&!details;
    int card_count=0;for(unsigned i=0;i<tips.count;++i)if(tips.items[i].kind==HoverTipKind::Card)++card_count;
    const bool event_lane=owner.event_option>=0&&!owner.merchant&&!owner.detail&&owner.owner.w>=160&&owner.owner.w<=240;
    const bool enchant_lane=hover_enchant_selection(c.a,owner);
    // Associated cards share the inspector's right column vertically. They
    // must not cover its text or the enlarged primary card on the left.
    const int card_w=(details||inspect||event_lane||enchant_lane)?69:46,card_h=(details||inspect||event_lane||enchant_lane)?96:62,card_step=card_h+1;
    const HoverRect column=enchant_lane?(card_count?HoverRect{212,121,104,71}:HoverRect{212,24,104,168}):event_lane&&card_count?HoverRect{8,125,SourceHoverWidth,93}:details?HoverRect{8,32,card_count?225:145,178}:inspect?
        HoverRect{212,8,104,card_count?109:209}:keyword_hover_column(owner);
    int direction=column.x<owner.owner.x?-1:1;if(owner.relic>=0)direction=owner.owner.x>240?-1:1;
    if(owner.power>=0||owner.creature>=0)direction=owner.power_left?-1:1;
    if(owner.merchant)direction=owner.merchant_left?-1:1;
    const int space=direction<0?column.x-3:317-column.x-column.w;
    const bool cards_need_room=card_count&&(hover_compact_event(owner)||(direction>0?owner.owner.x<50:owner.owner.x+owner.owner.w+47>317));
    const int columns=(event_lane||enchant_lane)?1:details?(card_count?1:2):inspect?1:mn(3,mx(1,1+(space-(cards_need_room?47:0))/(column.w+1)));
    const int step=source_hover_step(c),overhead=source_hover_overhead(c),minimum=overhead+step;
    int page=0,lane=0,y=0;HoverPage result{};
    for(unsigned i=0;i<tips.count;++i){
        if(tips.items[i].kind==HoverTipKind::Card)continue;
        char title[160]{},body[2048]{};format_card_hover_tip(c.a,owner,tips.items[i],title,sizeof title,body,sizeof body);
        if(!*title||!*body)continue;
        const int count=source_hover_paragraph(c,body,0,0,source_hover_body_width(column.w),0);int skip=0;
        while(skip<count){
            const int needed=overhead+(count-skip)*step;
            if(column.h-y<minimum||(needed<=column.h&&needed>column.h-y)){y=0;if(++lane>=columns){lane=0;++page;}}
            const int lines=mn(count-skip,mx(1,(column.h-y-overhead)/step)),height=overhead+lines*step;
            if(page==selected&&result.count<MaxCardHoverTips)result.pieces[result.count++]={i,skip,lines,{column.x+(details?1:direction)*lane*(column.w+1),column.y+y,column.w,height}};
            y+=height+1;skip+=lines;if(skip<count){y=0;if(++lane>=columns){lane=0;++page;}}
        }
    }
    int text_height=0;for(unsigned i=0;i<result.count;++i)text_height=mx(text_height,result.pieces[i].box.y+result.pieces[i].box.h-column.y);
    int text_top=column.y;
    if(!details&&!inspect&&!enchant_lane&&text_height){
        int requested=owner.relic>=0&&!owner.merchant?owner.owner.y+owner.owner.h+2:owner.owner.y;
        if(owner.relic>=0&&!owner.merchant&&owner.owner.y>180)requested=owner.owner.y-text_height;
        text_top=mn(mx(column.y,requested),column.y+column.h-text_height);
        for(unsigned i=0;i<result.count;++i)result.pieces[i].box.y+=text_top-column.y;
    }
    // CardHoverTip is a separate 239x323 source container with a 0.75 card;
    // it has no duplicate title or body beside its actual card face.
    const int cards_per_page=(inspect||event_lane||enchant_lane)?1:mx(1,(column.h+1)/card_step),card_pages=mx(1,(card_count+cards_per_page-1)/cards_per_page);
    int card_x=inspect?owner.owner.x-46:direction>0?owner.owner.x-47:owner.owner.x+owner.owner.w+1;
    if(details)card_x=243;
    if(inspect)card_x=column.x+(column.w-card_w)/2;
    if(hover_compact_event(owner))card_x=column.x+columns*(column.w+1);
    if(event_lane)card_x=column.x+column.w-card_w;
    if(enchant_lane)card_x=column.x+(column.w-card_w)/2;
    if((owner.power>=0||owner.creature>=0||owner.merchant)&&!details&&!inspect&&card_count&&result.count){
        // NHoverTipSet.CorrectHorizontalOverflow moves the actual containers
        // together. Empty available text columns must not separate a power
        // from its associated card when the original opposite side overflows.
        int left=317,right=3;for(unsigned i=0;i<result.count;++i){left=mn(left,result.pieces[i].box.x);right=mx(right,result.pieces[i].box.x+result.pieces[i].box.w);}
        int shift=0;if(card_x+46<=317&&right>317)shift=card_x-1-right;
        else if(card_x+46>317||right>317){card_x=right-46;shift=-47;}
        else if(card_x<3||left<3){card_x=left;shift=47;}
        for(unsigned i=0;i<result.count;++i)result.pieces[i].box.x+=shift;
    }else if(card_x<3||card_x+card_w>317)card_x=direction>0?column.x+columns*(column.w+1):column.x-(columns-1)*(column.w+1)-card_w-1;
    card_x=mx(3,mn(317-card_w,card_x));
    const int visible_cards=mx(0,mn(cards_per_page,card_count-selected*cards_per_page)),card_height=mx(0,visible_cards*card_step-1);
    int card_top=(event_lane||enchant_lane)?24:inspect?121:details?column.y:mn(mx(column.y,owner.owner.y),column.y+column.h-card_height);
    if(event_lane){
        // Keep an option's preview beside that option. The old fixed top24
        // put Wood Carvings' lower Torus choice a screen away from its card.
        // Associated keyword boxes remain together below the card.
        const int group_height=card_height+(text_height?text_height+4:0);
        card_top=mn(mx(24,owner.owner.y+(owner.owner.h-card_height)/2),mx(24,236-group_height));
        if(text_height)for(unsigned i=0;i<result.count;++i)result.pieces[i].box.y+=card_top+card_height+4-text_top;
    }
    if(owner.relic>=0&&!owner.inspect_relic&&!owner.merchant&&text_top+text_height+1+card_height<=column.y+column.h){card_x=column.x;card_top=text_top+text_height+1;}
    int card=0;for(unsigned i=0;i<tips.count;++i)if(tips.items[i].kind==HoverTipKind::Card){
        if(card/cards_per_page==selected&&result.count<MaxCardHoverTips)result.pieces[result.count++]={i,0,0,{card_x,card_top+(card%cards_per_page)*card_step,card_w,card_h}};
        ++card;
    }
    result.pages=mx(page+1,card_pages);return result;
}
HoverPage card_hover_page(App&a,const CardHoverTarget&owner,const HoverTipList&tips,int page){Canvas c{a};return hover_page(c,owner,tips,page);}
static void draw_hover_piece(Canvas&c,const CardHoverTarget&owner,const HoverTipList&tips,const HoverPiece&piece){
    const auto&tip=tips.items[piece.tip];const auto&r=piece.box;char title[160]{},body[2048]{};
    format_card_hover_tip(c.a,owner,tip,title,sizeof title,body,sizeof body);
    if(tip.kind==HoverTipKind::Card){
        const EventCardFacts canonical{};draw_card_face(c,tip.card,r.x+1,r.y,r.w-3,r.h-2,cost(tip.card),-2,tip.amount>0?tip.amount-1:-1,-1,tip.has_facts?&tip.facts:&canonical,body);return;
    }
    source_hover_skin(c,r,tip.debuff);int icon=-1;
    if(tip.kind==HoverTipKind::Relic){
        const RelicId id=world_relic_id(tip.key);int character=-1;
        // Only the actual held/history/event model has an owner. Related
        // relics are canonical models and share Cookie's process variant.
        if(owner.relic==int(id)&&owner.historical){const auto*run=c.a.history_at(c.a.history_index);if(run)character=run->character;}
        else if(owner.relic==int(id)&&owner.owned_relic>=0)character=int(c.a.game.character);
        else if(owner.relic==int(id)&&owner.inspect_relic)character=c.a.relic_inspect_character();
        else if(owner.event_option>=0){const auto&e=c.a.game.original_events;
            if(e.id!=OriginalEventId::FakeMerchant&&unsigned(owner.event_option)<e.offer_count&&e.offers[owner.event_option]==id)character=int(c.a.game.character);}
        icon=c.a.relic_face_image(id,character);
    }
    else if(tip.kind==HoverTipKind::Potion)icon=c.a.content.potion_image(Potion(tip.amount));
    else if(tip.kind==HoverTipKind::Intent){
        const auto intent=c.a.combat_intent(tip.actor-1);
        for(const auto&part:ui_intent_parts)if(part.flag==unsigned(tip.amount)){icon=ui_intent_image(c.a,part,intent);break;}
    }
    else if(tip.kind==HoverTipKind::Enchantment)icon=c.a.content.world_image("enchantments",tip.key);
    else if(tip.kind==HoverTipKind::Static&&hover_key_equal(tip.key,"ENERGY"))icon=c.a.content.energy_image(owner.card);
    source_hover_title(c,title,r,icon);
    source_hover_paragraph(c,body,r.x+SourceHoverLeft,r.y+SourceHoverTop+source_hover_step(c),source_hover_body_width(r.w),piece.lines,piece.skip);
}
// At calculator scale the original card face can ellipsize its body. Keep
// the full, current instance description in the ordinary hover stack instead
// of requiring a secondary inspector. The cached face already knows whether
// fitting truncated, so this adds no layout or glyph I/O to a settled hand.
static void collect_visible_hover_tips(Canvas&c,const CardHoverTarget&owner,HoverTipList&tips){
    collect_keyword_hover_tips(c.a,owner,tips);
    if(owner.inspect_relic)return;
    bool truncated=owner.detail&&c.a.card_inspect.description_truncated;
    if(owner.event_option>=0&&!owner.merchant){
        char title[256]{},body[4096]{},combined[4480]{};
        native_event_option(c.a,owner.event_option,false,title,sizeof title);native_event_option(c.a,owner.event_option,true,body,sizeof body);
        TextBuffer full{combined,sizeof combined,0};full.text(title);if(*body){full.ch('\n');full.text(body);}
        if(ui_ancient_event(c.a.game.original_events.id))truncated=ancient_text_rows(c,combined,ancient_layout::OptionsW-38)>3;
        else truncated=native_room_compact_plan(c,combined,mx(1,owner.owner.w-30),mx(1,owner.owner.h-12)).truncated;
    }else if(!owner.detail){
        if(owner.event_option>=0||owner.instance<0||c.a.library||c.a.panel||
           !c.a.hand_can_focus()||!c.a.hand_focus||owner.instance>=c.a.game.card_count)return;
        truncated=hand_tile(c,owner.instance).description_truncated;
    }
    if(!truncated)return;
    if(tips.count==MaxCardHoverTips){--tips.count;tips.overflow=true;}
    for(unsigned i=tips.count;i;i--)tips.items[i]=tips.items[i-1];
    HoverTip full{};full.kind=HoverTipKind::Description;full.key=owner.event_option>=0?"EVENT_OPTION_DESCRIPTION":definitions[owner.card.kind].key;full.card=owner.card;full.instanced=true;
    tips.items[0]=full;++tips.count;
}
static void keyword_tooltips(Canvas&c){
    auto&a=c.a;CardHoverTarget owner{};
    if(!keyword_hover_target(a,owner)){a.keyword_hover.owner=0;a.keyword_hover.page=0;if(a.detail_card<0)a.keyword_hover.details=false;return;}
    const bool new_owner=a.keyword_hover.owner!=owner.identity;
    if(new_owner){a.keyword_hover.owner=owner.identity;a.keyword_hover.page=0;}
    HoverTipList tips{};collect_visible_hover_tips(c,owner,tips);
    if(new_owner&&!owner.historical)for(unsigned i=0;i<tips.count;++i){const auto&tip=tips.items[i];
        if(tip.kind==HoverTipKind::Card)a.discover_card(tip.card.kind);
        else if(tip.kind==HoverTipKind::Relic)a.discover_relic(world_relic_id(tip.key));
        else if(tip.kind==HoverTipKind::Potion)a.discover_potion(Potion(tip.amount));
    }
    const bool detail=owner.detail&&a.keyword_hover.details;
    if(detail){c.dim(3,25,314,194,250);}
    if(!tips.count){if(detail){c.centered("这张牌没有关联词条。",18,102,284,12,Muted);
        c.centered(owner.event_option>=0?"返回事件":"返回",18,192,284,10,Gold);c.hit(18,190,284,13,HitKind::Command,int(Key::Statuses));}return;}
    HoverPage page=hover_page(c,owner,tips,a.keyword_hover.page);
    if(a.keyword_hover.page>=page.pages){a.keyword_hover.page=page.pages-1;page=hover_page(c,owner,tips,a.keyword_hover.page);}
    for(unsigned i=0;i<page.count;++i)draw_hover_piece(c,owner,tips,page.pieces[i]);
    if(!page.count)return;
    char footer[96]{};TextBuffer text{footer,sizeof footer,0,&c.a.content};text.number(a.keyword_hover.page+1);text.ch('/');text.number(page.pages);
    if(detail){if(page.pages>1)c.centered(footer,12,219,296,10,Gold);
        // Existing command dispatch keeps these controls modal and cancels a
        // captured release correctly; hover panels themselves remain passive.
        c.hit(12,216,65,22,HitKind::Command,int(Key::Up),a.keyword_hover.page>0);
        c.hit(243,216,65,22,HitKind::Command,int(Key::Down),a.keyword_hover.page+1<page.pages);
        c.hit(78,216,164,22,HitKind::Command,int(Key::Statuses));
    }else if(owner.detail&&page.pages>1){
        c.rect(212,222,104,16,Panel);c.centered(footer,212,224,104,source_hover_size(c),Gold);
        c.hit(212,222,45,16,HitKind::Scroll,-1,a.keyword_hover.page>0);
        c.hit(271,222,45,16,HitKind::Scroll,1,a.keyword_hover.page+1<page.pages);
    }else if(page.pages>1){
        const auto column=hover_enchant_selection(a,owner)?HoverRect{212,24,104,168}:keyword_hover_column(owner);
        const int foot_y=hover_enchant_selection(a,owner)?192:hover_compact_event(owner)?column.y+column.h+1:219;c.rect(column.x,foot_y,column.w,12,Panel);c.centered(footer,column.x,foot_y,column.w,source_hover_size(c),Gold);
    }
}
bool keyword_hover_scroll(App&a,int rows){
    if(!rows)return false;CardHoverTarget owner{};if(!keyword_hover_target(a,owner))return false;
    Canvas c{a};HoverTipList tips{};collect_visible_hover_tips(c,owner,tips);if(!tips.count)return false;
    auto page=hover_page(c,owner,tips,0);if(page.pages<2&&!a.keyword_hover.details)return false;
    if(a.keyword_hover.owner!=owner.identity){a.keyword_hover.owner=owner.identity;a.keyword_hover.page=0;}
    a.keyword_hover.page=mn(mx(0,a.keyword_hover.page+rows),page.pages-1);return true;
}
bool keyword_detail_input(App&a,Key k){
    CardHoverTarget event{};const bool is_event=event_hover_target(a,event);
    if((a.detail_card<0&&!is_event)||a.status_actor>=0||a.event_reader||a.map_preview.open||a.paused||a.help||a.settings_open||a.frontend_confirm)return false;
    if(k==Key::Statuses){if(is_event){HoverTipList tips{};collect_event_hover_tips(a,event.event_option,tips);if(!tips.count)return false;}a.keyword_hover.details=!a.keyword_hover.details;a.keyword_hover.page=0;return true;}
    if(!a.keyword_hover.details)return false;
    if(k==Key::Back||k==Key::Detail){a.keyword_hover.details=false;a.keyword_hover.page=0;return true;}
    if(k==Key::Up||k==Key::Down){keyword_hover_scroll(a,k==Key::Up?-1:1);return true;}
    return false;
}
