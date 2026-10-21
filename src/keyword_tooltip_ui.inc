// Card hover ownership and content follow the pinned CardModel/NHoverTipSet.
// The calculator adapter wraps at 10 px and pages when readable text cannot
// fit beside its owner. It adds no gameplay hits, RNG calls or save fields.
struct HoverTipSpec {HoverTipKind kind;const char*key;uint8_t flags;};
struct CardHoverCatalog {uint16_t first;uint8_t count,flags;uint32_t order;};
#include "keyword_hover_catalog.inc"

static bool hover_power_debuff(const char*key){for(auto name:hover_debuff_keys)if(hover_key_equal(name,key))return true;return false;}
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

static bool hover_in(const HoverRect&r,int x,int y){return x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;}
static bool hover_detail_word(const App&a,Card card){
    char text[2048]{};a.card_description(card,card_targets_enemy(a.game,card)?a.target:-1,text,sizeof text,a.detail_instance,a.detail_deck_index);
    RichReader reader(text);RichChar ch{};int x=123,row=0;
    while(reader.next(ch)){auto glyph=a.content.glyph(ch.codepoint,12);int advance=glyph.advance+(ch.style.bold?1:0);
        if(ch.codepoint=='\n'||(x>123&&x+advance>300)){x=123;++row;if(ch.codepoint=='\n')continue;}
        if(row>=a.detail_scroll&&row<a.detail_scroll+8&&ch.style.color==1&&
           hover_in({x,66+(row-a.detail_scroll)*15,mx(1,advance),15},a.pointer.x,a.pointer.y))return true;
        x+=advance;
    }return false;
}
static const HitRegion* hover_keyboard_hit(const App&a){
    if(a.library&&a.library_tab==0){for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::LibraryItem&&a.hits[i].value==a.library_selected)return &a.hits[i];}
    if(a.panel==1){int index=0;for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::DeckCard||a.hits[i].kind==HitKind::InstanceCard){if(index++==a.inspect_cursor-a.scroll)return &a.hits[i];}}
    if(!a.panel&&!a.library){for(int i=0;i<a.hit_count;++i){const auto&h=a.hits[i];
        if(h.kind==HitKind::Choice&&h.value==a.cursor)return &h;
        if(h.kind==HitKind::CombatChoice){const auto&source=choice_pile(a.game);if(a.cursor>=0&&a.cursor<source.count&&h.value==source.ids[a.cursor])return &h;}
    }}return nullptr;
}
bool card_hover_target(const App&a,CardHoverTarget&target){
    target={};if(a.paused||a.help||a.settings_open||a.frontend_confirm||a.character_select||a.map_preview.open||a.status_actor>=0||
        a.detail_item>=0||a.event_reader||a.pointer.held||a.pointer.armed_card>=0||a.map_transition_blocks_pointer())return false;
    if(a.detail_card>=0){
        target.card=card_from_token(a.detail_card);target.instance=a.detail_instance;target.deck=a.detail_deck_index;
        target.facts=a.detail_has_facts?&a.detail_facts:nullptr;target.owner={18,61,94,132};target.detail=true;
        target.identity=0x40000000u|uint32_t(a.detail_card);
        if(a.keyword_hover.details)return target.card.kind<CardKinds;
        if(!a.pointer.visible)return false;
        if(hover_in(target.owner,a.pointer.x,a.pointer.y))return target.card.kind<CardKinds;
        if(hover_detail_word(a,target.card)){target.owner={123,66,177,120};return target.card.kind<CardKinds;}return false;
    }
    if((a.title&&!a.library)||a.deck_candidate>=0||a.game_over_summary||(a.panel&&a.panel!=1)||a.map_window.is_open)return false;
    const HitRegion*h=a.pointer.visible?a.hit_at(a.pointer.x,a.pointer.y):hover_keyboard_hit(a);
    if(!a.library&&!a.panel&&a.hand_can_focus()&&a.hand_focus&&a.cursor>=0&&a.cursor<a.game.hand.count&&
       (!a.pointer.visible||(h&&h->kind==HitKind::Hand))){
        target.instance=a.game.hand.ids[a.cursor];target.card=a.game.cards[target.instance];auto pose=hand_pose(a.game.hand.count,a.cursor,true);
        target.owner={pose.cx-HandTileW/2,pose.cy-HandTileH/2,HandTileW,HandTileH};target.identity=0x10000u|unsigned(target.instance);return true;
    }
    if(!h)return false;target.owner={h->x,h->y,h->w,h->h};target.identity=0x100000u|(unsigned(h->kind)<<16)|unsigned(uint16_t(h->value));
    const auto&s=a.game;
    switch(h->kind){
    case HitKind::LibraryItem:{if(!a.library||a.library_tab!=0)return false;CardKind ids[CardKinds];unsigned n=a.library_items(ids);if(h->value<0||unsigned(h->value)>=n)return false;
        target.card={ids[h->value],uint8_t(a.library_upgraded&&card_upgradeable({ids[h->value],0}))};break;}
    case HitKind::DeckCard:if(h->value<0||h->value>=s.deck_count)return false;target.deck=h->value;target.card=s.deck[h->value];break;
    case HitKind::InstanceCard:if(h->value<0||h->value>=s.card_count)return false;target.instance=h->value;target.card=s.cards[h->value];break;
    case HitKind::CombatChoice:if(h->value<0)return false;target.card=choice_card(s,h->value);target.instance=h->value<s.card_count?h->value:-1;break;
    case HitKind::Choice:
        if(h->value<0)return false;
        if(s.phase==Phase::Reward&&a.reward_cards&&!a.grimoire_select){unsigned n=s.world.active?s.world.reward_count:3;if(unsigned(h->value)>=n)return false;
            target.card=s.world.active?s.world.reward_cards[h->value]:Card{s.reward[h->value],0};target.facts=s.world.active?&s.world.reward_facts[h->value]:nullptr;
        }else if(s.phase==Phase::Shop&&a.shop_open&&h->value<ShopCardCount){
            target.card=s.world.active?s.world.shop_cards[h->value]:Card{s.shop_cards[h->value],0};target.facts=s.world.active?&s.world.shop_facts[h->value]:nullptr;
        }else if(s.phase==Phase::Upgrade||s.phase==Phase::Remove||s.phase==Phase::Transform||a.grimoire_select){
            if(h->value>=s.deck_count)return false;target.deck=h->value;target.card=s.deck[h->value];
        }else return false;break;
    default:return false;
    }
    target.identity^=uint32_t(card_token(target.card))<<1;return target.card.kind<CardKinds;
}

struct HoverTextContext {WorldTextContext world;const HoverTip*tip;};
static LocalizedValue hover_text_value(const void*opaque,const char*name){
    const auto&x=*static_cast<const HoverTextContext*>(opaque);
    if(wt_equal(name,"Summon")||wt_equal(name,"Times"))return wt_number(x.tip->amount);
    return wt_resolve(&x.world,name);
}
void format_card_hover_tip(const App&a,const CardHoverTarget&owner,const HoverTip&tip,char*title,size_t title_cap,char*body,size_t body_cap){
    if(title_cap)title[0]=0;if(body_cap)body[0]=0;
    if(tip.kind==HoverTipKind::Card){
        TextBuffer name{title,title_cap,0,&a.content};name.text(a.content.card_string(tip.card.kind));if(tip.card.upgraded)name.ch('+');
        const EventCardFacts canonical{};a.card_description(tip.card,-1,body,body_cap,-1,-1,&canonical);return;
    }
    const char*table=tip.kind==HoverTipKind::Keyword?"card_keywords":tip.kind==HoverTipKind::Power?"powers":
        tip.kind==HoverTipKind::Orb?"orbs":tip.kind==HoverTipKind::Enchantment?"enchantments":tip.kind==HoverTipKind::Affliction?"afflictions":"static_hover_tips";
    int model=-1;if(tip.kind==HoverTipKind::Power)for(unsigned i=0;i<EnemyPowerCount;++i)if(wt_equal(tip.key,world_enemy_power_keys[i])){model=int(i);break;}
    HoverTextContext context{{&a,table,{},model,tip.actor,tip.amount,owner.card,owner.instance,owner.deck,owner.facts},&tip};
    auto format=[&](const char*suffix,char*out,size_t cap){if(!cap)return;char key[160]{},raw[4096]{};TextBuffer k{key,sizeof key,0};k.text(tip.key);k.text(suffix);
        wt_model_key(context.world,key);TextBuffer source{raw,sizeof raw,0};source.text(a.content.world_string(table,key));LocalizedTemplate(out,cap,hover_text_value,&context,a.content.world_title("static_hover_tips","ENERGY_COUNT"),a.content.world_title("static_hover_tips","STAR_COUNT"),a.content.english()).format(raw);};
    format(".title",title,title_cap);format(tip.smart?".smartDescription":".description",body,body_cap);
    if(body_cap&&!*body&&tip.smart)format(".description",body,body_cap);
}

static int hover_body_width(const HoverTip&tip,int width){return mx(16,width-(tip.kind==HoverTipKind::Card?61:17));}
static HoverPage hover_page(Canvas&c,const CardHoverTarget&owner,const HoverTipList&tips,int selected){
    const bool details=owner.detail&&c.a.keyword_hover.details;
    const HoverRect column=details?HoverRect{13,52,145,139}:card_hover_column(owner.owner);
    const int columns=details?2:1;int page=0,lane=0,y=0;HoverPage result{};
    for(unsigned i=0;i<tips.count;++i){char title[160]{},body[2048]{};format_card_hover_tip(c.a,owner,tips.items[i],title,sizeof title,body,sizeof body);
        if(!*title||!*body)continue;
        const int count=c.paragraph(body,0,0,hover_body_width(tips.items[i],column.w),0,0,10),minimum=tips.items[i].kind==HoverTipKind::Card?83:42;
        int skip=0;while(skip<count){
            int needed=mx(minimum,28+(count-skip)*13),room=column.h-y;
            if(room<minimum||(needed<=column.h&&needed>room)){y=0;if(++lane>=columns){lane=0;++page;}room=column.h;}
            int lines=mn(count-skip,mx(1,(room-28)/13)),height=mx(minimum,28+lines*13);
            if(page==selected&&result.count<MaxCardHoverTips)result.pieces[result.count++]={i,skip,lines,{column.x+lane*(column.w+4),column.y+y,column.w,height}};
            y+=height+3;skip+=lines;
            if(skip<count){y=0;if(++lane>=columns){lane=0;++page;}}
        }
    }
    result.pages=page+1;
    if(!details&&result.count){int height=result.pieces[result.count-1].box.y+result.pieces[result.count-1].box.h-column.y;
        int top=mn(mx(24,owner.owner.y),218-height),shift=top-column.y;for(unsigned i=0;i<result.count;++i)result.pieces[i].box.y+=shift;}
    return result;
}
HoverPage card_hover_page(App&a,const CardHoverTarget&owner,const HoverTipList&tips,int page){Canvas c{a};return hover_page(c,owner,tips,page);}
static void draw_hover_piece(Canvas&c,const CardHoverTarget&owner,const HoverTipList&tips,const HoverPiece&piece){
    const auto&tip=tips.items[piece.tip];const auto&r=piece.box;char title[160]{},body[2048]{};
    format_card_hover_tip(c.a,owner,tip,title,sizeof title,body,sizeof body);
    int skin=c.a.content.world_image("common",tip.debuff?"HOVER_TIP_DEBUFF":"HOVER_TIP");
    if(skin>=0){Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.nine_image(skin,r.x+2,r.y+2,r.w,r.h,28,22,46,16,64);
        c.nine_image(skin,r.x,r.y,r.w,r.h,28,22,46,16);
    }else{c.rect(r.x+2,r.y+2,r.w,r.h,Ink);c.rect(r.x,r.y,r.w,r.h,tip.debuff?rgb(59,35,45):rgb(31,43,49));c.box(r.x,r.y,r.w,r.h,rgb(104,117,115));}
    const uint16_t title_color=rgb(239,200,81),body_color=rgb(255,246,226);
    c.text(title,r.x+7,r.y+5,10,title_color,r.w-16);
    int body_x=r.x+7;
    if(tip.kind==HoverTipKind::Card){
        c.picture(c.a.card_face_frame(tip.card),r.x+5,r.y+20,45,63);c.picture(c.a.content.card_image(tip.card.kind),r.x+8,r.y+34,39,29);
        c.picture(c.a.content.energy_image(tip.card),r.x+3,r.y+19,17,17);c.energy_label(cost(tip.card),r.x+8,r.y+20,10);body_x=r.x+54;
    }else if(tip.kind==HoverTipKind::Enchantment){int icon=c.a.content.world_image("enchantments",tip.key);if(icon>=0)c.picture(icon,r.x+r.w-20,r.y+3,13,13);}
    else if(tip.kind==HoverTipKind::Static&&hover_key_equal(tip.key,"ENERGY"))c.picture(c.a.content.energy_image(owner.card),r.x+r.w-20,r.y+3,13,13);
    c.paragraph(body,body_x,r.y+20,hover_body_width(tip,r.w),piece.lines,piece.skip,10,body_color);
}
static void keyword_tooltips(Canvas&c){
    auto&a=c.a;CardHoverTarget owner{};
    if(!card_hover_target(a,owner)){a.keyword_hover.owner=0;a.keyword_hover.page=0;if(a.detail_card<0)a.keyword_hover.details=false;return;}
    if(a.keyword_hover.owner!=owner.identity){a.keyword_hover.owner=owner.identity;a.keyword_hover.page=0;}
    HoverTipList tips{};collect_card_hover_tips(a,owner.card,tips,owner.instance,owner.deck,owner.facts);
    const bool detail=owner.detail&&a.keyword_hover.details;
    if(detail){c.dim(11,47,298,155,250);}
    if(!tips.count){if(detail){c.centered("这张牌没有关联词条。",18,102,284,12,Muted);
        c.centered("V 返回卡牌",18,192,284,10,Gold);c.hit(18,190,284,13,HitKind::Command,int(Key::Statuses));}return;}
    HoverPage page=hover_page(c,owner,tips,a.keyword_hover.page);
    if(a.keyword_hover.page>=page.pages){a.keyword_hover.page=page.pages-1;page=hover_page(c,owner,tips,a.keyword_hover.page);}
    for(unsigned i=0;i<page.count;++i)draw_hover_piece(c,owner,tips,page.pieces[i]);
    if(!page.count)return;
    char footer[96]{};TextBuffer text{footer,sizeof footer,0,&c.a.content};text.number(a.keyword_hover.page+1);text.ch('/');text.number(page.pages);
    if(detail){text.text("  ↑↓ 翻页 · V 返回卡牌");c.centered(footer,12,192,296,10,Gold);
        // Existing command dispatch keeps these controls modal and cancels a
        // captured release correctly; hover panels themselves remain passive.
        c.hit(12,190,65,13,HitKind::Command,int(Key::Up),a.keyword_hover.page>0);
        c.hit(243,190,65,13,HitKind::Command,int(Key::Down),a.keyword_hover.page+1<page.pages);
        c.hit(78,190,164,13,HitKind::Command,int(Key::Statuses));
    }else if(page.pages>1){
        text.text(" 滚轮 / I→V");const auto column=card_hover_column(owner.owner);c.rect(column.x,219,column.w,14,Panel);c.centered(footer,column.x,220,column.w,10,Gold);
    }
}
bool keyword_hover_scroll(App&a,int rows){
    if(!rows)return false;CardHoverTarget owner{};if(!card_hover_target(a,owner))return false;
    HoverTipList tips{};collect_card_hover_tips(a,owner.card,tips,owner.instance,owner.deck,owner.facts);if(!tips.count)return false;
    Canvas c{a};auto page=hover_page(c,owner,tips,0);if(page.pages<2&&!a.keyword_hover.details)return false;
    if(a.keyword_hover.owner!=owner.identity){a.keyword_hover.owner=owner.identity;a.keyword_hover.page=0;}
    a.keyword_hover.page=mn(mx(0,a.keyword_hover.page+rows),page.pages-1);return true;
}
bool keyword_detail_input(App&a,Key k){
    if(a.detail_card<0||a.status_actor>=0||a.event_reader||a.map_preview.open||a.paused||a.help||a.settings_open||a.frontend_confirm)return false;
    if(k==Key::Statuses){a.keyword_hover.details=!a.keyword_hover.details;a.keyword_hover.page=0;return true;}
    if(!a.keyword_hover.details)return false;
    if(k==Key::Back||k==Key::Detail){a.keyword_hover.details=false;a.keyword_hover.page=0;return true;}
    if(k==Key::Up||k==Key::Down){keyword_hover_scroll(a,k==Key::Up?-1:1);return true;}
    return false;
}
