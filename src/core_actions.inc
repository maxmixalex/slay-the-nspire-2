static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
void record(State&s,LogKind k,int a,int b){
    if(s.log_count==24){for(int i=1;i<24;i++)s.logs[i-1]=s.logs[i];s.log_count--;}
    s.logs[s.log_count++]={k,(int32_t)a,(int32_t)b};
}
static void fail(State&s){s.phase=Phase::Error;record(s,LogKind::Capacity);}
uint32_t random_below(uint32_t&r,uint32_t bound){
    if(!r)r=0x7A31B529;
    if(!bound)return 0;
    // Independent xorshift streams; deliberately not claimed to reproduce PC seeds.
    uint32_t limit=0xFFFFFFFFU-(0xFFFFFFFFU%bound),x;
    do{r^=r<<13;r^=r>>17;r^=r<<5;x=r;}while(x>=limit);
    return x%bound;
}
static uint16_t remove(Pile&p,int i){uint16_t id=p.ids[i];for(int j=i+1;j<p.count;j++)p.ids[j-1]=p.ids[j];--p.count;return id;}
static bool put(State&s,Pile&p,int id,Zone z){
    if(id<0||id>=s.card_count||p.count>=MaxCards){fail(s);return false;}
    p.ids[p.count++]=(uint16_t)id;s.zones[id]=z;return true;
}
static int make_card(State&s,Card c){
    if(s.card_count==MaxCards){fail(s);return -1;}
    int id=s.card_count++;s.cards[id]=c;return id;
}
static void shuffle(State&s){
    while(s.discard.count){int id=remove(s.discard,s.discard.count-1);put(s,s.draw,id,Zone::Draw);}
    for(int i=s.draw.count-1;i>0;i--){int j=(int)random_below(s.rng_cards,i+1);auto t=s.draw.ids[i];s.draw.ids[i]=s.draw.ids[j];s.draw.ids[j]=t;}
    record(s,LogKind::Shuffle);shuffle_hook(s);
}
void draw_cards(State&s,int n){
    if(n<=0||s.no_draw)return;
    queue_extension(s,XOp::Draw,n,0,true);
    resolve_pending(s);
}
bool card_retains(Card c){return c.kind<CardKinds&&(((c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords)&4)||c.kind==Purity||(c.kind==Scrawl&&c.upgraded));}
bool card_innate(Card c){return c.kind<CardKinds&&(((c.upgraded?definitions[c.kind].upgraded_keywords:definitions[c.kind].keywords)&2)||c.kind==Backstab||(c.kind==InfiniteBlades&&c.upgraded));}
bool card_strike_tag(unsigned kind){
    return kind==Strike||kind==StrikeSilent||kind==PommelStrike||kind==TwinStrike||
           kind==LeadingStrike||kind==PerfectedStrike||kind==UltimateStrike||kind==SeekerStrike;
}
const Pile& choice_pile(const State&s){return s.phase==Phase::ExtendedChoice?s.expansion.choices:s.phase==Phase::ReturnChoice?s.discard:s.hand;}
// First-turn innate cards are placed above the shuffled ordinary cards. They
// occupy normal hand slots; excess innate cards stay in draw under the hand cap.
static int prioritize_innate(State&s){
    uint16_t sorted[MaxCards];unsigned count=0,innate=0;
    for(unsigned i=0;i<s.draw.count;i++)if(!card_innate(s.cards[s.draw.ids[i]]))sorted[count++]=s.draw.ids[i];
    for(unsigned i=0;i<s.draw.count;i++)if(card_innate(s.cards[s.draw.ids[i]])){sorted[count++]=s.draw.ids[i];++innate;}
    for(unsigned i=0;i<count;i++)s.draw.ids[i]=sorted[i];
    return (int)innate;
}
int cost(Card c){if(c.kind>=CardKinds)return 99;return definitions[c.kind].cost+(c.upgraded?definitions[c.kind].cost_up:0)-((c.kind==DarkEmbrace||c.kind==BodySlam)&&c.upgraded?1:0);}
int card_base_damage(const State&s,Card c){
    if(c.kind>=CardKinds)return 0;
    if(c.kind==BodySlam)return s.block;
    if(c.kind==GoldAxe)return s.expansion.cards_finished;
    if(c.kind==MindBlast)return s.draw.count;
    if(c.kind==SovereignBlade){
        int id=s.playing;
        if(id>=0&&id<s.card_count&&s.cards[id].kind==SovereignBlade)return max(10,s.expansion.blade_damage[id]);
        return 10;
    }
    if(c.kind==PerfectedStrike){
        int count=0;for(unsigned i=0;i<s.card_count;i++)if(card_strike_tag(s.cards[i].kind))++count;
        return 6+count*(c.upgraded?3:2);
    }
    if(c.kind==Shiv)return definitions[Shiv].damage+(c.upgraded?definitions[Shiv].damage_up:0)+s.accuracy;
    return definitions[c.kind].damage+(c.upgraded?definitions[c.kind].damage_up:0);
}
int damage(const State&s,Card c,int t,int instance){
    if(c.kind>=CardKinds)return 0;
    int d=card_base_damage(s,c);
    if(c.kind==SovereignBlade&&instance>=0&&instance<s.card_count&&s.cards[instance].kind==SovereignBlade)
        d=max(10,s.expansion.blade_damage[instance]);
    if(c.kind==Rend&&t>=0&&t<s.enemy_count){
        const auto&e=s.enemies[t];const auto&x=s.expansion;
        int debuffs=(e.strength<0)+(e.weak>0)+(e.vulnerable>0)+(e.poison>0)+(x.beetle[t]>0)+(x.demise[t]>0)+(x.doom[t]>0);
        d+=debuffs*(definitions[c.kind].power+(c.upgraded?definitions[c.kind].power_up:0));
    }
    if(d<=0&&definitions[c.kind].type!=CardType::Attack)return 0;
    d=max(0,d+s.strength+(s.attack_vigor?s.attack_vigor:s.vigor));
    d*=relic_attack_multiplier(s,true);
    if(t>=0&&t<s.enemy_count&&s.enemies[t].vulnerable>0)d=d*3/2;
    return d;
}
int enemy_damage(const Enemy&e){
    if(e.move==2)return 0;
    int amount=max(0,(e.move==0?12:6)+e.strength);
    return e.weak?amount*3/4:amount;
}
static bool enqueue(State&s,EffectKind k,int v=0,int t=0,bool first=false){
    if(s.effect_count>=MaxEffects){fail(s);return false;}
    if(first)for(int i=s.effect_count;i>0;i--)s.effects[i]=s.effects[i-1];
    s.effects[first?0:s.effect_count]={k,(int16_t)v,(int16_t)t};s.effect_count++;return true;
}
static bool alive(const State&s){for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp>0)return true;return false;}
static void victory(State&s){
    if(s.phase==Phase::Reward||s.phase==Phase::Complete)return;
    award_victory(s);
    // A lethal nested autoplay cannot leave its caller orphaned in Limbo.
    for(unsigned id=0;id<s.card_count;id++)if(s.zones[id]==Zone::Limbo){
        Card c=s.cards[id];Zone z=definitions[c.kind].type==CardType::Power?Zone::Power:card_exhausts(c)?Zone::Exhaust:Zone::Discard;
        put(s,z==Zone::Power?s.powers:z==Zone::Exhaust?s.exhaust:s.discard,id,z);
    }
    s.effect_count=0;s.playing=-1;s.expansion.depth=0;s.expansion.potion_active=0;s.expansion.enemy_side=0;
    s.discard_required=s.exhaust_limit=0;clear_extended_choice(s);relic_attack_end(s);s.expansion.triple=0;
    // Bounded prototype reward pool, not the original game's rarity/odds system.
    for(int i=0;i<3;i++){
        int k;bool duplicate;
        do{k=(int)random_reward_card(s);duplicate=false;for(int j=0;j<i;j++)if(s.reward[j]==k)duplicate=true;}while(duplicate);
        s.reward[i]=(uint8_t)k;
    }
    s.phase=Phase::Reward;
}
static void exhaust_id(State&s,int id){
    put(s,s.exhaust,id,Zone::Exhaust);
    record(s,LogKind::Exhaust,s.cards[id].kind);
    // Exhaust hooks run before the suspended card's remaining commands.
    // A power card is removed from combat when played, not exhausted.
    for(int i=s.power_count-1;i>=0;i--){
        if(s.power_order[i]==0&&s.pain)enqueue(s,EffectKind::Block,s.pain,0,true);
        if(s.power_order[i]==1&&s.embrace)enqueue(s,EffectKind::Draw,s.embrace,0,true);
    }
}
static void exhaust(State&s,int index){exhaust_id(s,remove(s.hand,index));}
static void apply_poison(State&s,int target,int count){
    if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0)return;
    auto&e=s.enemies[target];e.poison=(int16_t)min(9999,e.poison+count);
    record(s,LogKind::Poison,count,target);
}
static void create_shivs(State&s,int count){
    for(int i=0;i<count;i++){
        int id=make_card(s,{Shiv,0});if(id<0)return;
        // Newly created cards overflow to discard, not draw. They are not drawn,
        // so Battle Trance's No Draw must not suppress creation.
        if(s.hand.count<MaxHand)put(s,s.hand,id,Zone::Hand);else put(s,s.discard,id,Zone::Discard);
    }
    record(s,LogKind::Create,count,Shiv);
}
static void resolve(State&s){
    if(resolving)return;
    struct Guard { Guard(){resolving=true;} ~Guard(){resolving=false;} } guard;
    while(s.effect_count&&s.phase==Phase::Combat){
        Effect e=s.effects[0];for(int i=1;i<s.effect_count;i++)s.effects[i-1]=s.effects[i];s.effect_count--;
        switch(e.kind){
        case EffectKind::Extended:extension_effect(s,e);break;
        case EffectKind::RandomDamage:{
            int targets[MaxEnemies],count=0;
            for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp>0)targets[count++]=i;
            if(count)enqueue(s,EffectKind::Damage,e.value,targets[random_below(s.rng_cards,count)],true);
            break;}
        case EffectKind::ChooseReturn:
            if(alive(s)&&s.discard.count)s.phase=Phase::ReturnChoice;
            break;
        case EffectKind::Demon:s.demon_form=(int16_t)min(9999,s.demon_form+e.value);break;
        case EffectKind::Blades:s.infinite_blades=(int16_t)min(MaxCards,s.infinite_blades+e.value);break;
        case EffectKind::DodgeBlock:{
            int gained=min(9999-s.block,e.value);s.block+=(int16_t)gained;
            s.block_next_turn=(int16_t)min(9999,s.block_next_turn+gained);
            record(s,LogKind::Block,gained);break;}
        case EffectKind::DrawNextTurn:s.draw_next_turn=(int16_t)min(MaxCards,s.draw_next_turn+e.value);break;
        case EffectKind::EscapeSkillBlock:
            s.expansion.last_drawn=-1;
            queue_extension(s,XOp::EscapeBlock,e.value,0,true);
            queue_extension(s,XOp::Draw,1,0,true);break;
        case EffectKind::Damage:case EffectKind::GreedDamage:
            if(e.target>=0&&e.target<s.enemy_count&&s.enemies[e.target].hp>0){
                auto& t=s.enemies[e.target];if(!s.attack_vigor&&s.vigor){s.attack_vigor=s.vigor;s.vigor=0;}
                if(!s.expansion.triple&&power_value(s,XPower::Gigantic)){s.expansion.triple=1;gain_power(s,XPower::Gigantic,-1);}
                int amount=max(0,e.value+s.strength+s.attack_vigor)*relic_attack_multiplier(s);
                if(t.vulnerable)amount=amount*3/2;
                int absorb=min(t.block,amount);t.block-=absorb;t.hp=max(0,t.hp-amount+absorb);
                // Fisticuffs and Omnislice use TotalDamage + OverkillDamage.
                // Their follow-up keeps damage beyond the target's remaining HP.
                s.expansion.last_damage=(int16_t)min(9999,amount);
                record(s,LogKind::Damage,amount,e.target);
                if(e.kind==EffectKind::GreedDamage&&!t.hp&&s.playing>=0){
                    Card card=s.cards[s.playing];const auto& d=definitions[card.kind];
                    gain_gold(s,d.power+(card.upgraded?d.power_up:0));
                }
            }break;
        case EffectKind::Block:s.block=min(9999,s.block+e.value);record(s,LogKind::Block,e.value);break;
        case EffectKind::Vulnerable:
            if(e.target>=0&&e.target<s.enemy_count&&s.enemies[e.target].hp>0)
                s.enemies[e.target].vulnerable=min(9999,s.enemies[e.target].vulnerable+e.value);
            break;
        case EffectKind::Weak:
            if(e.target>=0&&e.target<s.enemy_count&&s.enemies[e.target].hp>0){
                s.enemies[e.target].weak=min(9999,s.enemies[e.target].weak+e.value);
                record(s,LogKind::Weak,e.value,e.target);
            }break;
        case EffectKind::Poison:apply_poison(s,e.target,e.value);break;
        case EffectKind::RandomPoison:{
            int targets[MaxEnemies],count=0;
            for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp>0)targets[count++]=i;
            if(count)apply_poison(s,targets[random_below(s.rng_cards,count)],e.value);
            break;}
        case EffectKind::Dexterity:s.dexterity=(int16_t)min(9999,s.dexterity+e.value);record(s,LogKind::Dexterity,e.value);break;
        case EffectKind::Noxious:s.noxious_fumes=(int16_t)min(9999,s.noxious_fumes+e.value);record(s,LogKind::Noxious,e.value);break;
        case EffectKind::Accuracy:s.accuracy=(int16_t)min(9999,s.accuracy+e.value);record(s,LogKind::Accuracy,e.value);break;
        case EffectKind::CreateShivs:create_shivs(s,e.value);break;
        case EffectKind::ChooseDiscard:
            s.discard_required=(uint8_t)min(e.value,s.hand.count);
            if(!alive(s))s.discard_required=0;
            else if(s.discard_required)s.phase=Phase::DiscardChoice;
            break;
        case EffectKind::DrawIfNoAttacks:{
            bool attack=false;for(unsigned i=0;i<s.hand.count;i++)
                if(definitions[s.cards[s.hand.ids[i]].kind].type==CardType::Attack)attack=true;
            if(!attack)draw_cards(s,e.value);break;}
        case EffectKind::FillHand:draw_cards(s,MaxHand-s.hand.count);break;
        case EffectKind::ChooseExhaustMany:
            s.exhaust_limit=(uint8_t)min(e.value,s.hand.count);
            if(s.exhaust_limit)s.phase=Phase::ExhaustManyChoice;
            break;
        case EffectKind::NoBlock:s.no_block=(int16_t)min(9999,s.no_block+e.value);break;
        case EffectKind::NoDraw:s.no_draw=1;record(s,LogKind::NoDraw);break;
        case EffectKind::Energy:s.energy=min(999,s.energy+e.value);record(s,LogKind::Energy,e.value);break;
        case EffectKind::LoseHp:player_hp_loss(s,e.value,false,false);break;
        case EffectKind::ExhaustCard:
            // A captured instance, not a changing hand index. Draws from exhaust
            // hooks may enter the hand but must not join Fiend Fire's snapshot.
            for(int i=0;i<s.hand.count;i++)if(s.hand.ids[i]==e.value){exhaust(s,i);break;}
            break;
        case EffectKind::WhirlwindHits:
            if(e.target>0&&alive(s)){
                if(e.target>1)enqueue(s,EffectKind::WhirlwindHits,e.value,e.target-1,true);
                for(int i=s.enemy_count-1;i>=0;--i)if(s.enemies[i].hp>0)
                    enqueue(s,EffectKind::Damage,e.value,i,true);
            }break;
        case EffectKind::Draw:draw_cards(s,e.value);break;
        case EffectKind::ChooseExhaust:if(s.hand.count)s.phase=Phase::ExhaustChoice;break;
        case EffectKind::RandomExhaust:if(s.hand.count)exhaust(s,(int)random_below(s.rng_cards,s.hand.count));break;
        case EffectKind::CopyDiscard:{int id=make_card(s,s.cards[s.playing]);if(id>=0)put(s,s.discard,id,Zone::Discard);}break;
        case EffectKind::Strength:s.strength=min(9999,s.strength+e.value);record(s,LogKind::Strength,e.value);break;
        case EffectKind::Pain:
            if(!s.pain&&s.power_count<2)s.power_order[s.power_count++]=0;
            s.pain=min(9999,s.pain+e.value);record(s,LogKind::Pain,s.pain);break;
        case EffectKind::Embrace:
            if(!s.embrace&&s.power_count<2)s.power_order[s.power_count++]=1;
            s.embrace=min(99,s.embrace+e.value);record(s,LogKind::Embrace,s.embrace);break;
        case EffectKind::Finish:finish_extended_card(s);break;
        }
    }
}
void damage_unpowered(State&s,int target,int amount){
    if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0)return;
    auto&e=s.enemies[target];int absorbed=min(e.block,max(0,amount));e.block-=absorbed;
    e.hp=(int16_t)max(0,e.hp-max(0,amount)+absorbed);record(s,LogKind::Damage,amount,target);
}
void finish_combat_if_won(State&s){if(s.phase==Phase::Combat&&s.hp>0&&!alive(s))victory(s);}
