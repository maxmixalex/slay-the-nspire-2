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
    int id=s.card_count++;s.cards[id]=c;
    ironclad_card_created(s,id);silent_card_created(s,id);defect_card_created(s,id);regent_card_created(s,id);necro_card_created(s,id);event_card_created(s,id);enemy_card_created(s,id);return id;
}
static void shuffle(State&s,bool initial=false){
    while(s.discard.count){int id=remove(s.discard,s.discard.count-1);put(s,s.draw,id,Zone::Draw);}
    for(int i=s.draw.count-1;i>0;i--){int j=(int)random_below(s.rng_cards,i+1);auto t=s.draw.ids[i];s.draw.ids[i]=s.draw.ids[j];s.draw.ids[j]=t;}
    record(s,LogKind::Shuffle);shuffle_hook(s);
    event_card_after_shuffle(s,initial);
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
           kind==LeadingStrike||kind==PerfectedStrike||kind==UltimateStrike||kind==SeekerStrike||
           kind==AshenStrike||kind==SetupStrike||kind==DefectStrike||kind==DefectMeteorStrike||kind==DefectFocusedStrike||kind==DefectAdaptiveStrike||kind==RegentStrike||kind==RegentSolarStrike||kind==RegentShiningStrike||kind==MinionStrike||kind==NecroStrikeNecrobinder||kind==NecroSculptingStrike;
}
const Pile& choice_pile(const State&s){return s.phase==Phase::ExtendedChoice?s.expansion.choices:s.phase==Phase::ReturnChoice?s.discard:s.hand;}
// First-turn innate cards are placed above the shuffled ordinary cards. They
// occupy normal hand slots; excess innate cards stay in draw under the hand cap.
static int prioritize_innate(State&s){
    uint16_t sorted[MaxCards];unsigned count=0,innate=0;
    for(unsigned i=0;i<s.draw.count;i++)if(!card_innate(s,s.cards[s.draw.ids[i]],s.draw.ids[i]))sorted[count++]=s.draw.ids[i];
    for(unsigned i=0;i<s.draw.count;i++)if(card_innate(s,s.cards[s.draw.ids[i]],s.draw.ids[i])){sorted[count++]=s.draw.ids[i];++innate;}
    for(unsigned i=0;i<count;i++)s.draw.ids[i]=sorted[i];
    return (int)innate;
}
int cost(Card c){if(c.kind>=CardKinds)return 99;return definitions[c.kind].cost+(c.upgraded?definitions[c.kind].cost_up:0)-((c.kind==DarkEmbrace||c.kind==BodySlam)&&c.upgraded?1:0);}
int card_base_damage(const State&s,Card c,int target,int instance){
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
    int base=definitions[c.kind].damage+(c.upgraded?definitions[c.kind].damage_up:0);
    if(c.kind==Shiv)base+=s.accuracy;
    base=ironclad_base_damage(s,c,target,instance,base);
    base=silent_base_damage(s,c,target,instance,base);base=defect_base_damage(s,c,target,instance,base);
    base=regent_base_damage(s,c,target,instance,base);base=necro_base_damage(s,c,target,instance,base);return event_card_base_damage(s,c,instance,base);
}
int damage(const State&s,Card c,int t,int instance){
    if(c.kind>=CardKinds)return 0;
    int d=card_base_damage(s,c,t,instance);
    if(c.kind==SovereignBlade&&instance>=0&&instance<s.card_count&&s.cards[instance].kind==SovereignBlade)
        d=max(10,s.expansion.blade_damage[instance]);
    if(c.kind==Rend&&t>=0&&t<s.enemy_count){
        const auto&e=s.enemies[t];const auto&x=s.expansion;
        int debuffs=(e.strength<0)+(e.weak>0)+(e.vulnerable>0)+(e.poison>0)+(x.beetle[t]>0)+(x.demise[t]>0)+(x.doom[t]>0);
        d+=debuffs*(definitions[c.kind].power+(c.upgraded?definitions[c.kind].power_up:0));
    }
    if(d<=0&&card_type(s,c)!=CardType::Attack)return 0;
    d+=event_enchantment_damage_add(s,c,instance);
    bool active=s.playing>=0&&instance==s.playing;
    bool pet=necro_osty_attack(c);if(pet&&!s.expansion.osty_hp)return 0;
    d=max(0,d+(pet?necro_osty_bonus(s):s.strength+(active&&s.attack_vigor?s.attack_vigor:s.vigor)));
    if(active)d*=(s.double_attack?2:1)*((s.expansion.triple||power_value(s,XPower::Gigantic))?3:1);
    else d*=((s.relics&PenNib)&&relic_counter(s,PenNib)==9?2:1)*(power_value(s,XPower::Gigantic)?3:1);
    d=silent_attack_factor(s,t,d,c,instance);
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
static bool alive(const State&s){return enemy_keeps_combat_alive(s);}
static void victory(State&s){
    if(s.phase==Phase::Reward||s.phase==Phase::Complete)return;
    if(!s.world.active)award_victory(s);
    // A lethal nested autoplay cannot leave its caller orphaned in Limbo.
    for(unsigned id=0;id<s.card_count;id++)if(s.zones[id]==Zone::Limbo){
        Card c=s.cards[id];Zone z=card_type(s,c)==CardType::Power?Zone::Power:(card_exhausts(s,c,id)||ironclad_exhausts(s,id))?Zone::Exhaust:Zone::Discard;
        put(s,z==Zone::Power?s.powers:z==Zone::Exhaust?s.exhaust:s.discard,id,z);
    }
    ironclad_combat_end(s);silent_combat_end(s);defect_combat_end(s);regent_combat_end(s,true);necro_combat_end(s);event_card_combat_end(s,true);
    s.effect_count=0;s.playing=-1;s.expansion.depth=0;s.expansion.potion_active=0;s.expansion.enemy_side=0;
    s.discard_required=s.exhaust_limit=0;clear_extended_choice(s);relic_attack_end(s);s.expansion.triple=0;
    // Bounded prototype reward pool, not the original game's rarity/odds system.
    for(int i=0;!s.world.active&&i<3;i++){
        int k;bool duplicate;
        do{k=(int)random_reward_card(s);duplicate=false;for(int j=0;j<i;j++)if(s.reward[j]==k)duplicate=true;}while(duplicate);
        s.reward[i]=(CardKind)k;
    }
    s.phase=Phase::Reward;
    if(s.world.active)award_victory(s);
}
static void exhaust_id(State&s,int id){
    put(s,s.exhaust,id,Zone::Exhaust);
    record(s,LogKind::Exhaust,s.cards[id].kind);
    ironclad_after_exhaust(s,id);
    // Exhaust hooks run before the suspended card's remaining commands.
    // A power card is removed from combat when played, not exhausted.
    for(int i=s.power_count-1;i>=0;i--){
        if(s.power_order[i]==0&&s.pain)enqueue(s,EffectKind::Block,s.pain,0,true);
        if(s.power_order[i]==1&&s.embrace)enqueue(s,EffectKind::Draw,s.embrace,0,true);
    }
}
static void exhaust(State&s,int index){exhaust_id(s,remove(s.hand,index));}
void apply_vulnerable(State&s,int target,int count){
    if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0||count<=0)return;
    if(enemy_blocks_debuff(s,target)||silent_blocks_debuff(s,target))return;
    auto&e=s.enemies[target];int added=min(9999-e.vulnerable,count);
    e.vulnerable+=(int16_t)added;ironclad_after_vulnerable(s,target,added);
    if(added){necro_note_debuff(s,target,NDebuff::Vulnerable);necro_debuff_applied(s,target,added);}
}
void apply_weak(State&s,int target,int count){
    if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0||count<=0)return;
    if(enemy_blocks_debuff(s,target)||silent_blocks_debuff(s,target))return;
    auto&e=s.enemies[target];int added=min(9999-e.weak,count);e.weak+=(int16_t)added;
    record(s,LogKind::Weak,added,target);if(added){necro_note_debuff(s,target,NDebuff::Weak);necro_debuff_applied(s,target,added);}
}
void apply_poison(State&s,int target,int count){
    if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0||count<=0)return;
    if(enemy_blocks_debuff(s,target)||silent_blocks_debuff(s,target))return;
    auto&e=s.enemies[target];int added=min(9999-e.poison,count);e.poison+=(int16_t)added;
    record(s,LogKind::Poison,added,target);if(added){necro_note_debuff(s,target,NDebuff::Poison);necro_debuff_applied(s,target,added);}
}
static void create_shivs(State&s,int count){
    for(int i=0;i<count;i++){
        int id=make_card(s,{Shiv,0});if(id<0)return;
        // Newly created cards overflow to discard, not draw. They are not drawn,
        // so Battle Trance's No Draw must not suppress creation.
        if(s.hand.count<MaxHand)put(s,s.hand,id,Zone::Hand);else put(s,s.discard,id,Zone::Discard);
        notify_card_generated(s,id);
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
            int before=s.block;gain_card_block(s,e.value);int gained=s.block-before;
            s.block_next_turn=(int16_t)min(9999,s.block_next_turn+gained);
            break;}
        case EffectKind::DrawNextTurn:s.draw_next_turn=(int16_t)min(MaxCards,s.draw_next_turn+e.value);break;
        case EffectKind::EscapeSkillBlock:
            s.expansion.last_drawn=-1;
            queue_extension(s,XOp::EscapeBlock,e.value,0,true);
            queue_extension(s,XOp::Draw,1,0,true);break;
        case EffectKind::Damage:case EffectKind::GreedDamage:
            if(e.target>=0&&e.target<s.enemy_count&&s.enemies[e.target].hp>0){
                auto& t=s.enemies[e.target];int old_hp=t.hp;bool pet=e.aux==2;
                if(!pet&&!s.attack_vigor&&s.vigor){s.attack_vigor=s.vigor;s.vigor=0;}
                if(!s.expansion.triple&&power_value(s,XPower::Gigantic)){s.expansion.triple=1;gain_power(s,XPower::Gigantic,-1);}
                Card card=s.playing>=0?s.cards[s.playing]:Card{Strike,0};
                int amount=max(0,e.value+event_enchantment_damage_add(s,card,s.playing)+(pet?necro_osty_bonus(s):s.strength+s.attack_vigor))*relic_attack_multiplier(s);
                amount=silent_attack_factor(s,e.target,amount,card,s.playing);
                amount=enemy_damage_cap(s,e.target,amount,true);enemy_before_damage(s,e.target,amount,true,pet);
                int absorb=min(t.block,amount);t.block-=absorb;int hp_loss=enemy_hp_loss_cap(s,e.target,amount-absorb);t.hp=max(0,t.hp-hp_loss);
                // Fisticuffs and Omnislice use TotalDamage + OverkillDamage.
                // Their follow-up keeps damage beyond the target's remaining HP.
                s.expansion.last_damage=(int16_t)min(9999,amount);
                record(s,LogKind::Damage,amount,e.target);
                all_characters_attack_damage(s,e.target,absorb+min(old_hp,hp_loss),hp_loss,pet);
                enemy_after_damage(s,e.target,absorb,min(old_hp,hp_loss),true,pet);
                if(e.kind==EffectKind::GreedDamage&&!t.hp&&s.playing>=0&&!s.expansion.attack_observed[s.expansion.depth]){
                    Card card=s.cards[s.playing];const auto& d=definitions[card.kind];
                    gain_gold(s,d.power+(card.upgraded?d.power_up:0));
                }
                if(!t.hp){
                    if(s.expansion.attack_observed[s.expansion.depth])s.expansion.attack_killed[s.expansion.depth]|=uint8_t(1U<<e.target);
                    else {silent_attack_killed(s,e.target);defect_attack_killed(s,e.target);}
                }
            }break;
        case EffectKind::Block:
            if(e.aux==1)gain_card_block(s,e.value);else gain_unpowered_block(s,e.value);
            break;
        case EffectKind::Vulnerable:
            apply_vulnerable(s,e.target,e.value);
            break;
        case EffectKind::Weak:
            apply_weak(s,e.target,e.value);break;
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
                if(card_type(s,s.cards[s.hand.ids[i]],s.hand.ids[i])==CardType::Attack)attack=true;
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
        case EffectKind::CopyDiscard:{int id=make_card(s,s.cards[s.playing]);if(id>=0){copy_card_modifiers(s,id,s.playing);put(s,s.discard,id,Zone::Discard);notify_card_generated(s,id);}}break;
        case EffectKind::Strength:s.strength=min(9999,s.strength+e.value);record(s,LogKind::Strength,e.value);break;
        case EffectKind::Pain:
            if(!s.pain&&s.power_count<2)s.power_order[s.power_count++]=0;
            s.pain=min(9999,s.pain+e.value);record(s,LogKind::Pain,s.pain);break;
        case EffectKind::Embrace:
            if(!s.embrace&&s.power_count<2)s.power_order[s.power_count++]=1;
            s.embrace=min(99,s.embrace+e.value);record(s,LogKind::Embrace,s.embrace);break;
        case EffectKind::Finish:finish_extended_card(s);break;
        }
        enemy_check_deaths(s);necro_observe_deaths(s);
    }
}
void damage_unpowered(State&s,int target,int amount){
    if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0)return;
    amount=enemy_damage_cap(s,target,max(0,amount),false);enemy_before_damage(s,target,amount,false);
    auto&e=s.enemies[target];int absorbed=min(e.block,amount);e.block-=absorbed;
    int loss=min(e.hp,enemy_hp_loss_cap(s,target,amount-absorbed));e.hp-=loss;
    record(s,LogKind::Damage,amount,target);enemy_after_damage(s,target,absorbed,loss,false);enemy_check_deaths(s);necro_observe_deaths(s);
}
static void enemy_reset_shared_slot(State&s,int t){
 if(t<0||t>=MaxEnemies)return;
 s.enemies[t]={};auto&x=s.expansion;auto&v=s.silent;auto&r=s.regent;auto&n=s.necro;
 x.restore_strength[t]=x.demise[t]=x.beetle[t]=x.doom[t]=0;
 v.strangle[t]=v.enemy_artifact[t]=0;r.hits_turn[t]=r.conqueror[t]=0;
 n.debilitate[t]=n.hang[t]=n.oblivion[t]=n.sic_em[t]=0;n.debuff_count[t]=0;
 for(auto&entry:n.debuff_order[t])entry=0;for(auto&entry:n.temp_strength[t])entry=0;
 for(unsigned f=0;f<=MaxAutoFrames;++f){v.strangle_before[f][t]=n.oblivion_before[f][t]=0;}
 n.enemy_dead_mask&=uint8_t(~(1U<<t));
}

void finish_combat_if_won(State&s){if(s.phase==Phase::Combat&&s.hp>0&&!alive(s))victory(s);}
