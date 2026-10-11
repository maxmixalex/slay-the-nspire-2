#include "core.hpp"

namespace spire {
// Inspected against the user's unmodified v0.111.0 / 41cef1ea assembly.
// A rule subset, not a general C# interpreter and not the complete game.
#include "card_definitions.inc"
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
    record(s,LogKind::Shuffle);
}
void draw_cards(State&s,int n){
    if(s.no_draw)return;
    int got=0;
    while(n-->0&&s.hand.count<MaxHand){
        if(!s.draw.count){if(!s.discard.count)break;shuffle(s);}
        int id=remove(s.draw,s.draw.count-1);put(s,s.hand,id,Zone::Hand);got++;
    }
    if(got)record(s,LogKind::Draw,got);
}
bool card_retains(Card c){return c.kind==Purity||(c.kind==Scrawl&&c.upgraded);}
bool card_innate(Card c){return c.kind==Backstab||(c.kind==InfiniteBlades&&c.upgraded);}
bool card_strike_tag(unsigned kind){
    return kind==Strike||kind==StrikeSilent||kind==PommelStrike||kind==TwinStrike||
           kind==LeadingStrike||kind==PerfectedStrike;
}
const Pile& choice_pile(const State&s){return s.phase==Phase::ReturnChoice?s.discard:s.hand;}
// First-turn innate cards are placed above the shuffled ordinary cards. They
// occupy normal hand slots; excess innate cards stay in draw under the hand cap.
static int prioritize_innate(State&s){
    uint16_t sorted[MaxCards];unsigned count=0,innate=0;
    for(unsigned i=0;i<s.draw.count;i++)if(!card_innate(s.cards[s.draw.ids[i]]))sorted[count++]=s.draw.ids[i];
    for(unsigned i=0;i<s.draw.count;i++)if(card_innate(s.cards[s.draw.ids[i]])){sorted[count++]=s.draw.ids[i];++innate;}
    for(unsigned i=0;i<count;i++)s.draw.ids[i]=sorted[i];
    return (int)innate;
}
int cost(Card c){if(c.kind>=CardKinds)return 99;return definitions[c.kind].cost-((c.kind==DarkEmbrace||c.kind==BodySlam)&&c.upgraded?1:0);}
int card_base_damage(const State&s,Card c){
    if(c.kind>=CardKinds)return 0;
    if(c.kind==BodySlam)return s.block;
    if(c.kind==PerfectedStrike){
        int count=0;for(unsigned i=0;i<s.card_count;i++)if(card_strike_tag(s.cards[i].kind))++count;
        return 6+count*(c.upgraded?3:2);
    }
    if(c.kind==Shiv)return definitions[Shiv].damage+(c.upgraded?definitions[Shiv].damage_up:0)+s.accuracy;
    return definitions[c.kind].damage+(c.upgraded?definitions[c.kind].damage_up:0);
}
int damage(const State&s,Card c,int t){
    if(c.kind>=CardKinds)return 0;
    int d=card_base_damage(s,c);
    if(d<=0&&c.kind!=BodySlam)return 0;
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
    s.effect_count=0;s.playing=-1;
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
    while(s.effect_count&&s.phase==Phase::Combat){
        Effect e=s.effects[0];for(int i=1;i<s.effect_count;i++)s.effects[i-1]=s.effects[i];s.effect_count--;
        switch(e.kind){
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
        case EffectKind::EscapeSkillBlock:{
            unsigned before=s.hand.count;draw_cards(s,1);
            if(s.hand.count>before&&definitions[s.cards[s.hand.ids[before]].kind].type==CardType::Skill)
                enqueue(s,EffectKind::Block,e.value,0,true);
            break;}
        case EffectKind::Damage:case EffectKind::GreedDamage:
            if(e.target>=0&&e.target<s.enemy_count&&s.enemies[e.target].hp>0){
                auto& t=s.enemies[e.target];if(!s.attack_vigor&&s.vigor){s.attack_vigor=s.vigor;s.vigor=0;}
                int amount=max(0,e.value+s.strength+s.attack_vigor)*relic_attack_multiplier(s);
                if(t.vulnerable)amount=amount*3/2;
                int absorb=min(t.block,amount);t.block-=absorb;t.hp=max(0,t.hp-amount+absorb);
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
        case EffectKind::LoseHp:
            {int old=s.hp;s.hp=max(0,s.hp-e.value);record(s,LogKind::LoseHp,old-s.hp);
             relic_after_hp_loss(s,old-s.hp);}
            if(!s.hp){
                // Stop the queued card immediately on death. Keep the zone partition
                // serializable, but do not heal, draw, or gain energy after dying.
                if(s.playing>=0)put(s,s.discard,s.playing,Zone::Discard);
                s.playing=-1;s.effect_count=0;s.phase=Phase::Dead;relic_attack_end(s);
            }break;
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
        case EffectKind::Finish:
            if(s.playing>=0){int id=s.playing;
                relic_card_played(s,s.cards[id]);
                if(definitions[s.cards[id].kind].type==CardType::Attack)s.attacked_this_turn=1;
                if(definitions[s.cards[id].kind].type==CardType::Power)put(s,s.powers,id,Zone::Power);
                else if(definitions[s.cards[id].kind].exhausts)exhaust_id(s,id);
                else put(s,s.discard,id,Zone::Discard);s.playing=-1;}
            relic_attack_end(s);if(!alive(s))victory(s);break;
        }
    }
}
void damage_unpowered(State&s,int target,int amount){
    if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0)return;
    auto&e=s.enemies[target];int absorbed=min(e.block,max(0,amount));e.block-=absorbed;
    e.hp=(int16_t)max(0,e.hp-max(0,amount)+absorbed);record(s,LogKind::Damage,amount,target);
}
void finish_combat_if_won(State&s){if(s.phase==Phase::Combat&&s.hp>0&&!alive(s))victory(s);}
void enter_combat(State&s){
    s.relic_flash=0;relic_reset_turn_counters(s);
    s.phase=Phase::Combat;s.block=s.strength=s.pain=s.embrace=s.dexterity=0;s.double_attack=0;s.power_count=0;s.no_draw=0;s.temporary_strength=s.temporary_dexterity=s.vigor=s.attack_vigor=s.thorns=0;
    s.noxious_fumes=s.accuracy=s.discard_required=0;
    s.no_block=s.exhaust_limit=s.attacked_this_turn=0;s.relic_counters[24]=0;
    s.demon_form=s.infinite_blades=s.block_next_turn=s.draw_next_turn=s.flame_barrier=0;
    s.turn=1;s.energy=3;s.playing=-1;s.effect_count=s.card_count=0;
    s.draw.count=s.hand.count=s.discard.count=s.exhaust.count=s.powers.count=0;
    s.enemy_count=s.encounter?2:1;
    for(int i=0;i<s.enemy_count;i++)s.enemies[i]={(int16_t)(42+random_below(s.rng_world,5)),0,0,0,0,(uint8_t)(s.enemy_count==1?0:(i==0?1:2))};
    for(int i=0;i<s.enemy_count;i++)s.enemies[i].max_hp=s.enemies[i].hp;
    for(int i=0;i<s.deck_count;i++){int id=make_card(s,s.deck[i]);if(id>=0)put(s,s.discard,id,Zone::Discard);}
    shuffle(s);
    if(s.practice&&s.floor==1){
        // Explicit practice opening: lets the player exercise two exhaust hooks.
        const uint8_t ironclad[5]={FeelNoPain,DarkEmbrace,BurningPact,TrueGrit,Strike};
        const uint8_t silent[7]={DeadlyPoison,Acrobatics,BladeDance,Neutralize,Survivor,Footwork,PoisonedStab};
        const uint8_t* opening=s.character==Character::Silent?silent:ironclad;
        for(int k=0;k<(s.character==Character::Silent?7:5);k++)for(int j=0;j<s.draw.count;j++)if(s.cards[s.draw.ids[j]].kind==opening[k]){
            int id=remove(s.draw,j);put(s,s.hand,id,Zone::Hand);break;}
    }else{
        int innate=prioritize_innate(s);
        draw_cards(s,min(MaxHand,max(innate,5+((s.relics&BagOfPreparation)?2:0)+((s.relics&RingOfTheSnake)?2:0))));
    }
    relic_hook(s,RelicHook::CombatStart);relic_hook(s,RelicHook::TurnStart);
    record(s,LogKind::Turn,1);finish_combat_if_won(s);s.revision++;
}
void new_run(State&s,uint32_t seed,bool practice,Character character){
    s=State{};
    if((unsigned)character>=CharacterKinds){s.phase=Phase::Error;return;}
    s.character=character;s.seed=seed?seed:1;
    s.rng_cards=s.seed^0xA341316Cu;s.rng_world=s.seed^0xC8013EA4u;
    s.hp=s.max_hp=(int16_t)character_definitions[(unsigned)character].starting_hp;
    s.potion_chance=40;s.floor=1;s.practice=practice;s.playing=-1;s.gold=99;s.current_node=-1;s.event_first=-1;
    if(practice){
        for(unsigned i=0;i<CardKinds;i++)if(card_in_pool(i,character)||card_colorless(i))
            s.deck[s.deck_count++]={(uint8_t)i,(uint8_t)(i==DarkEmbrace||i==TrueGrit)};
    }else if(character==Character::Silent){
        for(int i=0;i<5;i++)s.deck[s.deck_count++]={StrikeSilent,0};
        for(int i=0;i<5;i++)s.deck[s.deck_count++]={DefendSilent,0};
        s.deck[s.deck_count++]={Neutralize,0};s.deck[s.deck_count++]={Survivor,0};
    }else{
        for(int i=0;i<5;i++)s.deck[s.deck_count++]={Strike,0};
        for(int i=0;i<4;i++)s.deck[s.deck_count++]={Defend,0};s.deck[s.deck_count++]={Bash,0};
    }
    gain_relic(s,character_definitions[(unsigned)character].starting_relic);
    create_map(s);
    if(practice){s.current_node=1;s.floor=1;s.encounter=0;enter_combat(s);}
}
bool play(State&s,int h,int target){
    if(s.phase!=Phase::Combat||s.effect_count||s.playing!=-1||h<0||h>=s.hand.count)return false;
    if(s.hand.ids[h]>=s.card_count){fail(s);return false;}
    Card c=s.cards[s.hand.ids[h]];if(c.kind>=CardKinds){fail(s);return false;}const Definition&d=definitions[c.kind];int n=cost(c);int energy=n<0?s.energy:n;
    if(energy>s.energy|| (d.targeted&&(target<0||target>=s.enemy_count||s.enemies[target].hp==0)))return false;
    if((c.kind==Anger&&s.card_count==MaxCards)||s.card_count+d.creates+(c.upgraded?d.creates_up:0)>MaxCards){fail(s);return false;}
    s.relic_flash=0;s.energy-=energy;s.playing=remove(s.hand,h);s.zones[s.playing]=Zone::Limbo;
    int hit=card_base_damage(s,c),block=d.block+(c.upgraded?d.block_up:0),cards=d.draw+(c.upgraded?d.draw_up:0),power=d.power+(c.upgraded?d.power_up:0);
    if(d.type==CardType::Attack)relic_attack_begin(s);
    if(block)block=s.no_block?0:max(0,block+s.dexterity);
    record(s,LogKind::Play,c.kind,c.upgraded);
    if(d.hp_loss)enqueue(s,EffectKind::LoseHp,d.hp_loss);
    if(d.energy_gain)enqueue(s,EffectKind::Energy,d.energy_gain+(c.upgraded?d.energy_up:0));
    if(c.kind==LegSweep)enqueue(s,EffectKind::Weak,power,target);
    if(c.kind==DodgeAndRoll)enqueue(s,EffectKind::DodgeBlock,block);
    else if(c.kind==EscapePlan)enqueue(s,EffectKind::EscapeSkillBlock,block);
    else if(block)enqueue(s,EffectKind::Block,block);
    if(c.kind==FlameBarrier)s.flame_barrier=(int16_t)min(9999,s.flame_barrier+power);
    if(c.kind==DemonForm)enqueue(s,EffectKind::Demon,power);
    if(c.kind==InfiniteBlades)enqueue(s,EffectKind::Blades,1);
    if(c.kind==FiendFire){
        const int count=s.hand.count;
        for(int i=0;i<count;i++)enqueue(s,EffectKind::ExhaustCard,s.hand.ids[i]);
        for(int i=0;i<count;i++)enqueue(s,EffectKind::Damage,hit,target);
    }else if(c.kind==SwordBoomerang){
        for(int i=0;i<power;i++)enqueue(s,EffectKind::RandomDamage,hit);
    }else if(c.kind==DaggerSpray){
        for(int repeat=0;repeat<2;repeat++)for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Damage,hit,i);
    }else if(c.kind==Whirlwind)enqueue(s,EffectKind::WhirlwindHits,hit,energy);
    else if(c.kind==Thunderclap){
        for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Damage,hit,i);
        for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Vulnerable,power,i);
    }else if(hit||c.kind==BodySlam){
        enqueue(s,c.kind==HandOfGreed?EffectKind::GreedDamage:EffectKind::Damage,hit,target);
        if(c.kind==TwinStrike)enqueue(s,EffectKind::Damage,hit,target);
    }
    if(c.kind==Headbutt)enqueue(s,EffectKind::ChooseReturn);
    if(c.kind==Predator)enqueue(s,EffectKind::DrawNextTurn,2);
    if(c.kind==Neutralize||c.kind==SuckerPunch)enqueue(s,EffectKind::Weak,power,target);
    if(c.kind==DeadlyPoison||c.kind==PoisonedStab)enqueue(s,EffectKind::Poison,power,target);
    if(c.kind==BouncingFlask)for(int i=0;i<(c.upgraded?4:3);i++)enqueue(s,EffectKind::RandomPoison,power);
    if(d.creates)enqueue(s,EffectKind::CreateShivs,d.creates+(c.upgraded?d.creates_up:0));
    if(c.kind==Footwork)enqueue(s,EffectKind::Dexterity,power);
    if(c.kind==NoxiousFumes)enqueue(s,EffectKind::Noxious,power);
    if(c.kind==Accuracy)enqueue(s,EffectKind::Accuracy,power);
    if(c.kind==Uppercut){enqueue(s,EffectKind::Weak,power,target);enqueue(s,EffectKind::Vulnerable,power,target);}
    if(c.kind==Bash)enqueue(s,EffectKind::Vulnerable,power,target);
    if(c.kind==BurningPact||(c.kind==TrueGrit&&c.upgraded))enqueue(s,EffectKind::ChooseExhaust);
    if(c.kind==TrueGrit&&!c.upgraded)enqueue(s,EffectKind::RandomExhaust);
    if(c.kind==Impatience)enqueue(s,EffectKind::DrawIfNoAttacks,cards);
    else if(c.kind==Purity)enqueue(s,EffectKind::ChooseExhaustMany,cards);
    else if(c.kind==Scrawl)enqueue(s,EffectKind::FillHand);
    else if(cards)enqueue(s,EffectKind::Draw,cards);
    if(c.kind==PanicButton)enqueue(s,EffectKind::NoBlock,2);
    if(c.kind==Shockwave)for(int i=0;i<s.enemy_count;i++){
        enqueue(s,EffectKind::Weak,power,i);enqueue(s,EffectKind::Vulnerable,power,i);
    }
    if(c.kind==Survivor||c.kind==Acrobatics||c.kind==DaggerThrow)enqueue(s,EffectKind::ChooseDiscard,1);
    if(c.kind==Prepared)enqueue(s,EffectKind::ChooseDiscard,cards);
    if(c.kind==BattleTrance)enqueue(s,EffectKind::NoDraw);
    if(c.kind==Anger)enqueue(s,EffectKind::CopyDiscard);
    if(c.kind==Inflame)enqueue(s,EffectKind::Strength,power);
    if(c.kind==FeelNoPain)enqueue(s,EffectKind::Pain,power);
    if(c.kind==DarkEmbrace)enqueue(s,EffectKind::Embrace,1);
    enqueue(s,EffectKind::Finish);resolve(s);s.revision++;return true;
}
bool choose_exhaust(State&s,int index){
    if(s.phase!=Phase::ExhaustChoice||index<0||index>=s.hand.count)return false;
    s.phase=Phase::Combat;exhaust(s,index);resolve(s);s.revision++;return true;
}
bool choose_discard(State&s,const uint16_t* ids,unsigned count){
    if(s.phase!=Phase::DiscardChoice||!ids||count!=s.discard_required||!count||count>MaxHand)return false;
    // Validate the entire choice before moving any instance; invalid requests
    // cannot partially discard cards or consume RNG.
    for(unsigned i=0;i<count;i++){
        bool in_hand=false;for(unsigned j=0;j<s.hand.count;j++)if(s.hand.ids[j]==ids[i])in_hand=true;
        if(!in_hand)return false;
        for(unsigned j=0;j<i;j++)if(ids[i]==ids[j])return false;
    }
    for(unsigned i=0;i<count;i++)for(int j=0;j<s.hand.count;j++)if(s.hand.ids[j]==ids[i]){
        int id=remove(s.hand,j);put(s,s.discard,id,Zone::Discard);record(s,LogKind::Discard,s.cards[id].kind);break;
    }
    s.discard_required=0;s.phase=Phase::Combat;resolve(s);s.revision++;return true;
}
unsigned choice_min(const State&s){
    return s.phase==Phase::ExhaustManyChoice?0:s.phase==Phase::DiscardChoice?s.discard_required:1;
}
unsigned choice_max(const State&s){return s.phase==Phase::ExhaustManyChoice?s.exhaust_limit:choice_min(s);}
bool choose_exhaust_many(State&s,const uint16_t* ids,unsigned count){
    if(s.phase!=Phase::ExhaustManyChoice||count>s.exhaust_limit||(count&&!ids))return false;
    for(unsigned i=0;i<count;i++){
        bool found=false;for(unsigned j=0;j<s.hand.count;j++)if(s.hand.ids[j]==ids[i])found=true;
        if(!found)return false;for(unsigned j=0;j<i;j++)if(ids[i]==ids[j])return false;
    }
    if(s.effect_count+count>MaxEffects)return false;
    // Prepend captured instances in selection order. Each exhaust's hooks resolve
    // before the next captured card, without including newly drawn cards.
    for(unsigned i=count;i>0;i--)enqueue(s,EffectKind::ExhaustCard,ids[i-1],0,true);
    s.exhaust_limit=0;s.phase=Phase::Combat;resolve(s);s.revision++;return true;
}
bool choose_return_to_draw(State&s,int card_id){
    if(s.phase!=Phase::ReturnChoice||card_id<0||card_id>=s.card_count)return false;
    int index=-1;for(int i=0;i<s.discard.count;i++)if(s.discard.ids[i]==card_id){index=i;break;}
    if(index<0)return false;
    int id=remove(s.discard,index);put(s,s.draw,id,Zone::Draw);
    s.phase=Phase::Combat;resolve(s);s.revision++;return true;
}
bool end_turn(State&s){
    if(s.phase!=Phase::Combat||s.effect_count||s.playing!=-1)return false;
    s.relic_flash=0;relic_hook(s,RelicHook::TurnEnd);
    s.no_draw=0;
    s.strength=max(0,s.strength-s.temporary_strength);s.temporary_strength=0;
    s.dexterity=max(0,s.dexterity-s.temporary_dexterity);s.temporary_dexterity=0;
    for(int i=0;i<s.hand.count;){
        if(card_retains(s.cards[s.hand.ids[i]])){++i;continue;}
        int id=remove(s.hand,i);put(s,s.discard,id,Zone::Discard);
    }
    // Poison resolves for the enemy side before any enemy begins its move.
    // It bypasses block and all attack-only modifiers in the supported model set.
    for(int i=0;i<s.enemy_count;i++){
        auto&e=s.enemies[i];if(e.hp>0&&e.poison>0){
            int amount=e.poison;e.hp=(int16_t)max(0,e.hp-amount);
            if(e.hp>0)--e.poison;
            record(s,LogKind::PoisonDamage,amount,i);
        }
    }
    if(!alive(s)){victory(s);s.revision++;return true;}
    for(int i=0;i<s.enemy_count;i++){
        auto&e=s.enemies[i];if(e.hp==0)continue;e.block=0;
        record(s,LogKind::Enemy,i,e.move);
        int amount=enemy_damage(e),absorb=min(s.block,amount);
        // Thorns is BeforeDamageReceived in the pinned build. The already-started
        // incoming hit still resolves even if its dealer dies to retaliation.
        if(e.move!=2&&s.thorns)damage_unpowered(s,i,s.thorns);
        s.block-=absorb;int old_hp=s.hp;s.hp=max(0,s.hp-amount+absorb);
        relic_after_hp_loss(s,old_hp-s.hp);
        if(s.hp>0&&e.move!=2&&s.flame_barrier)damage_unpowered(s,i,s.flame_barrier);
        if(e.move==1)e.block=5;
        if(e.move==2)e.strength=min(9999,e.strength+2);
        if(e.vulnerable>0)e.vulnerable--;
        if(e.weak>0)e.weak--;
        e.move=(uint8_t)((e.move+1)%3);
        if(!s.hp){s.phase=Phase::Dead;break;}
    }
    s.flame_barrier=0;
    if(s.no_block>0)--s.no_block; // NoBlockPower.AfterSideTurnEnd: enemy side.
    if(s.hp&&!alive(s)){victory(s);s.revision++;return true;}
    if(s.hp){relic_reset_turn_counters(s);s.block=s.block_next_turn;s.block_next_turn=0;s.energy=3;s.turn++;
        if(s.block)record(s,LogKind::Block,s.block);
        if(s.demon_form){s.strength=(int16_t)min(9999,s.strength+s.demon_form);record(s,LogKind::Strength,s.demon_form);}
        // Infinite Blades runs BeforeHandDraw; created cards occupy hand space
        // before the standard draw and Predator's one-turn draw modifier.
        if(s.infinite_blades){
            if(s.infinite_blades>MaxCards-s.card_count){fail(s);s.revision++;return true;}
            create_shivs(s,s.infinite_blades);
        }
        for(int i=0;i<s.enemy_count;i++)if(s.noxious_fumes)apply_poison(s,i,s.noxious_fumes);
        int draw=5+s.draw_next_turn;s.draw_next_turn=0;
        draw_cards(s,draw);relic_hook(s,RelicHook::TurnStart);record(s,LogKind::Turn,s.turn);finish_combat_if_won(s);}
    s.revision++;return true;
}
bool choose_reward(State&s,int index){
    if(s.phase!=Phase::Reward||(s.reward_claimed&2)||index< -1||index>2)return false;
    if(index>=0){if(s.deck_count==MaxDeck){fail(s);return false;}s.deck[s.deck_count++]={s.reward[index],0};record(s,LogKind::Reward,s.reward[index]);}
    s.reward_claimed|=2;s.revision++;return true;
}
bool camp(State&s,bool upgrading){
    if(s.phase!=Phase::Camp)return false;
    if(upgrading){if(!has_upgrade(s))return false;s.phase=Phase::Upgrade;s.revision++;}
    else{heal(s,rest_heal_amount(s));finish_room(s);}
    return true;
}
bool upgrade_card(State&s,int index){
    if(s.phase!=Phase::Upgrade||index<0||index>=s.deck_count||s.deck[index].upgraded)return false;
    s.deck[index].upgraded=1;record(s,LogKind::Upgrade,s.deck[index].kind);finish_room(s);return true;
}
const Pile& pile(const State&s,Zone z){switch(z){case Zone::Draw:return s.draw;case Zone::Hand:return s.hand;case Zone::Discard:return s.discard;case Zone::Exhaust:return s.exhaust;default:return s.powers;}}
bool validate(const State&s){
    if((int)s.phase>(int)Phase::ReturnChoice||s.practice>1||s.floor>MapRows||s.max_hp<1||s.max_hp>999||s.hp<0||s.hp>s.max_hp||s.energy<0||s.energy>999||s.turn<0||s.turn>20000)return false;
    if((unsigned)s.character>=CharacterKinds||s.noxious_fumes<0||s.noxious_fumes>9999||s.accuracy<0||s.accuracy>9999||s.discard_required>MaxHand)return false;
    if(s.phase!=Phase::DiscardChoice&&s.discard_required)return false;
    if(s.phase==Phase::DiscardChoice&&(!s.discard_required||s.discard_required>s.hand.count))return false;
    if(s.exhaust_limit>MaxHand||s.no_block<0||s.no_block>9999||s.attacked_this_turn>1)return false;
    if(s.phase!=Phase::ExhaustManyChoice&&s.exhaust_limit)return false;
    if(s.phase==Phase::ExhaustManyChoice&&(!s.exhaust_limit||s.exhaust_limit>s.hand.count))return false;
    if(s.demon_form<0||s.demon_form>9999||s.infinite_blades<0||s.infinite_blades>MaxCards||
       s.block_next_turn<0||s.block_next_turn>9999||s.draw_next_turn<0||s.draw_next_turn>MaxCards||
       s.flame_barrier<0||s.flame_barrier>9999)return false;
    if(s.relic_flash&~((1u<<RelicKinds)-1))return false;
    if(s.no_draw>1||s.reward_claimed>7||(unsigned)s.reward_potion>PotionKinds||s.potion_chance>100)return false;
    if(s.temporary_strength<0||s.temporary_strength>s.strength||s.temporary_dexterity<0||s.temporary_dexterity>s.dexterity||
       s.vigor<0||s.vigor>9999||s.attack_vigor<0||s.attack_vigor>9999||s.thorns<0||s.thorns>9999)return false;
    if(s.block<0||s.block>9999||s.strength<0||s.strength>9999||s.pain<0||s.pain>9999||s.embrace<0||s.embrace>99)return false;
    if(s.deck_count<1||s.deck_count>MaxDeck||s.card_count>MaxCards||s.hand.count>MaxHand||s.effect_count>MaxEffects||s.log_count>24||s.power_count>2)return false;
    if(s.enemy_count>2||s.playing< -1||s.playing>=s.card_count)return false;
    if(s.current_node< -1||s.current_node>=MapNodes||s.next_row>MapRows||s.encounter>1||s.gold<0||s.gold>9999||s.last_gold<0||s.last_gold>99)return false;
    if((int)s.event_id>2||s.event_page>4||s.event_remaining>2||s.event_first< -1||s.event_first>=s.deck_count||s.event_value<0||s.event_value>9999)return false;
    if(s.shop_sold>16383)return false;
    for(auto id:s.shop_relics)if(id&&!relic_definition(id))return false;
    for(auto p:s.shop_potions)if(p>PotionKinds)return false;
    if(s.relic_count>RelicKinds||s.dexterity<0||s.dexterity>9999||s.double_attack>1)return false;
    uint32_t owned=0;
    for(unsigned i=0;i<s.relic_count;i++){unsigned index=s.relic_order[i];if(index>=RelicKinds)return false;unsigned bit=relic_definitions[index].id;if(owned&bit)return false;owned|=bit;}
    if(owned!=s.relics)return false;
    for(int i=0;i<RelicKinds;i++){
        unsigned id=relic_definitions[i].id,limit=relic_counter_limit(id);
        unsigned maximum=id==CentennialPuzzle?1:limit?limit-1:0;
        if(s.relic_counters[i]>maximum||(!(s.relics&id)&&s.relic_counters[i]))return false;
    }
    if(s.treasure_state>2)return false;
    if(s.phase!=Phase::Treasure&&(s.treasure_state||s.treasure_relic))return false;
    if(!s.treasure_state&&s.treasure_relic)return false;
    if(s.treasure_relic&&(!relic_definition(s.treasure_relic)||s.treasure_relic==BurningBlood||s.treasure_relic==RingOfTheSnake))return false;
    if(s.treasure_relic&&s.treasure_state==1&&(s.relics&s.treasure_relic))return false;
    if(s.treasure_relic&&s.treasure_state==2&&!(s.relics&s.treasure_relic))return false;
    for(auto p:s.potions)if((unsigned)p>PotionKinds)return false;
    for(auto c:s.shop_cards)if(c>=CardKinds)return false;
    for(auto p:s.shop_prices)if(p<0||p>9999)return false;
    for(int i=0;i<MapNodes;i++){auto n=s.map[i];if((int)n.room>(int)Room::Final||n.links>7||n.visited>1||n.event>2)return false;}
    if(s.phase==Phase::Map&&s.next_row>=MapRows)return false;
    if(s.phase==Phase::Transform&&(s.event_id!=EventId::Grove||s.event_remaining<1))return false;
    if(combat_phase(s.phase)&&(!s.enemy_count||s.turn<1))return false;
    for(int i=0;i<s.deck_count;i++)if(s.deck[i].kind>=CardKinds||s.deck[i].upgraded>1)return false;
    for(int i=0;i<s.card_count;i++)if(s.cards[i].kind>=CardKinds||s.cards[i].upgraded>1)return false;
    for(int i=0;i<3;i++)if(s.reward[i]>=CardKinds)return false;
    for(int i=0;i<s.power_count;i++)if(s.power_order[i]>1||(i&&s.power_order[0]==s.power_order[1]))return false;
    uint8_t seen[MaxCards]={};
    for(int z=0;z<5;z++){const Pile&p=pile(s,(Zone)z);if(p.count>MaxCards)return false;
        for(int i=0;i<p.count;i++){int id=p.ids[i];if(id>=s.card_count||seen[id]||s.zones[id]!=(Zone)z)return false;seen[id]=1;}}
    if(s.playing>=0){if(seen[s.playing]||s.zones[s.playing]!=Zone::Limbo)return false;seen[s.playing]=1;}
    for(int i=0;i<s.card_count;i++)if(!seen[i])return false;
    for(int i=0;i<s.enemy_count;i++){const auto&e=s.enemies[i];if(e.hp<0||e.hp>e.max_hp||e.max_hp<42||e.max_hp>46||e.block<0||e.block>9999||e.strength<0||e.strength>9999||e.vulnerable<0||e.vulnerable>9999||e.move>2||e.weak<0||e.weak>9999||e.poison<0||e.poison>9999)return false;}
    for(int i=0;i<s.effect_count;i++){const auto&e=s.effects[i];if((int)e.kind>(int)EffectKind::EscapeSkillBlock||e.value<0||e.value>9999||e.target<0||e.target>=(e.kind==EffectKind::WhirlwindHits?1000:MaxEnemies))return false;
        if(e.kind==EffectKind::ExhaustCard&&e.value>=s.card_count)return false;}
    if(choice_phase(s.phase)&&(s.playing<0||!s.effect_count||!choice_pile(s).count))return false;
    if(!choice_phase(s.phase)&&s.phase!=Phase::Error&&(s.playing>=0||s.effect_count))return false;
    return true;
}
// Versioned little-endian field encoding, not a raw C++ object image.
struct Codec{
    uint8_t* out; const uint8_t* in; size_t at,limit; bool good;
    uint32_t field(uint32_t v){if(at+4>limit){good=false;return 0;}uint32_t r=0;for(int i=0;i<4;i++){if(out)out[at+i]=(uint8_t)(v>>(8*i));else r|=(uint32_t)in[at+i]<<(8*i);}at+=4;return out?v:r;}
};
static uint32_t checksum(const uint8_t*p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;i++){h^=p[i];h*=16777619u;}return h;}
template<class T> static void field(T& v,Codec&c){uint32_t raw=c.field((uint32_t)v);v=static_cast<T>(raw);if(c.in&&(uint32_t)v!=raw)c.good=false;}
static void code_state(State&s,Codec&c,unsigned version){
#define F(x) field(x,c)
 F(s.phase);F(s.practice);F(s.floor);F(s.enemy_count);F(s.hp);F(s.max_hp);F(s.energy);F(s.block);F(s.strength);F(s.pain);F(s.embrace);F(s.turn);F(s.power_count);
 for(int i=0;i<2;i++)F(s.power_order[i]);F(s.rng_cards);F(s.rng_world);F(s.seed);F(s.revision);F(s.deck_count);
 if(s.deck_count>MaxDeck){c.good=false;return;}for(int i=0;i<s.deck_count;i++){F(s.deck[i].kind);F(s.deck[i].upgraded);}
 F(s.card_count);if(s.card_count>MaxCards){c.good=false;return;}for(int i=0;i<s.card_count;i++){F(s.cards[i].kind);F(s.cards[i].upgraded);F(s.zones[i]);}
 Pile*ps[]={&s.draw,&s.hand,&s.discard,&s.exhaust,&s.powers};for(int z=0;z<5;z++){auto&p=*ps[z];F(p.count);if(p.count>MaxCards){c.good=false;return;}for(int i=0;i<p.count;i++)F(p.ids[i]);}
 for(auto&e:s.enemies){F(e.hp);F(e.max_hp);F(e.block);F(e.strength);F(e.vulnerable);F(e.move);}
 F(s.effect_count);if(s.effect_count>MaxEffects){c.good=false;return;}for(int i=0;i<s.effect_count;i++){auto&e=s.effects[i];F(e.kind);F(e.value);F(e.target);}
 F(s.playing);
 F(s.relic_count);for(int i=0;i<10;i++)F(s.relic_order[i]);for(int i=0;i<10;i++)F(s.relic_counters[i]);
 F(s.dexterity);F(s.double_attack);F(s.relic_flash);F(s.relic_revision);
 F(s.current_node);F(s.next_row);F(s.encounter);F(s.relics);F(s.removals);F(s.gold);F(s.last_gold);
 for(auto&n:s.map){F(n.room);F(n.links);F(n.visited);F(n.event);}
 for(auto&p:s.potions)F(p);for(int i=0;i<4;i++)F(s.shop_cards[i]);F(s.shop_relics[0]);F(s.shop_potions[0]);F(s.shop_sold);
 const int legacy_prices[]={0,1,2,3,7,10};for(int i:legacy_prices)F(s.shop_prices[i]);
 F(s.event_id);F(s.event_page);F(s.event_count);F(s.event_remaining);F(s.event_value);F(s.event_first);
 for(int i=0;i<3;i++)F(s.reward[i]);F(s.log_count);if(s.log_count>24){c.good=false;return;}for(int i=0;i<s.log_count;i++){auto&l=s.logs[i];F(l.kind);F(l.a);F(l.b);}
if(version>=4){F(s.no_draw);for(auto&e:s.enemies)F(e.weak);}
 if(version>=5){
  for(int i=10;i<16;i++)F(s.relic_order[i]);
  for(int i=10;i<16;i++)F(s.relic_counters[i]);
  for(int i=4;i<ShopCardCount;i++)F(s.shop_cards[i]);
  for(int i=1;i<3;i++){F(s.shop_relics[i]);F(s.shop_potions[i]);}
  const int extra_prices[]={4,5,6,8,9,11,12};for(int i:extra_prices)F(s.shop_prices[i]);
  F(s.temporary_strength);F(s.temporary_dexterity);F(s.vigor);F(s.attack_vigor);F(s.thorns);
  F(s.reward_claimed);F(s.reward_potion);F(s.potion_chance);
 }else if(c.in){
  const unsigned old=s.shop_sold;
  if(old>127||s.relic_count>10||s.relics>=1024||s.relic_flash>=1024||s.shop_relics[0]>=1024||s.shop_potions[0]>3)c.good=false;
  for(auto p:s.potions)if((unsigned)p>3)c.good=false;
  s.shop_sold=uint16_t((old&15)|((old&16)<<3)|((old&32)<<5)|((old&64)<<7)|
                     (1<<4)|(1<<5)|(1<<6)|(1<<8)|(1<<9)|(1<<11)|(1<<12));
  s.reward_claimed=1;s.potion_chance=40;
 }
 if(version>=6){
  F(s.character);F(s.noxious_fumes);F(s.accuracy);F(s.discard_required);
  for(auto&e:s.enemies)F(e.poison);
  for(int i=16;i<17;i++){F(s.relic_order[i]);F(s.relic_counters[i]);}
 }else if(c.in){
  if(s.phase==Phase::DiscardChoice||s.relic_count>(version>=5?16:10)||s.relics>65535||s.relic_flash>65535)c.good=false;
  const unsigned max_kind=version>=5?26:version==4?24:14;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=max_kind)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=max_kind)c.good=false;
  for(auto&l:s.logs)if(l.kind==LogKind::Relic&&l.a<0)l.a=(uint16_t)l.a;
 }
 if(version>=7){
  F(s.exhaust_limit);F(s.no_block);F(s.attacked_this_turn);
  for(int i=17;i<25;i++){F(s.relic_order[i]);F(s.relic_counters[i]);}
 }else if(c.in){
  if(s.phase==Phase::ExhaustManyChoice||s.relic_count>17||s.relics>=1u<<17||s.relic_flash>=1u<<17)c.good=false;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=51)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=51)c.good=false;
  for(unsigned i=0;i<s.effect_count;i++)if((unsigned)s.effects[i].kind>(unsigned)EffectKind::RandomPoison)c.good=false;
 }
 if(version>=8){
  F(s.demon_form);F(s.infinite_blades);F(s.block_next_turn);F(s.draw_next_turn);F(s.flame_barrier);
 }else if(c.in){
  if(s.phase==Phase::ReturnChoice)c.good=false;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=58)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=58)c.good=false;
  for(auto k:s.shop_cards)if(k>=58)c.good=false;
  for(auto k:s.reward)if(k>=58)c.good=false;
  for(unsigned i=0;i<s.effect_count;i++)if((unsigned)s.effects[i].kind>(unsigned)EffectKind::GreedDamage)c.good=false;
 }
 if(version>=9){
  // Freeze v9 widths just as v7/v8 retain their 25-relic directory.
  for(int i=25;i<30;i++){F(s.relic_order[i]);F(s.relic_counters[i]);}
  F(s.treasure_state);F(s.treasure_relic);
 }else if(c.in){
  const uint32_t old_mask=(1u<<25)-1;
  if(s.relic_count>25||(s.relics&~old_mask)||(s.relic_flash&~old_mask))c.good=false;
  for(unsigned id:s.shop_relics)if(id&~old_mask)c.good=false;
 }
#undef F
}
bool save(const State&s,SaveData&out){
    if(!validate(s)||s.phase==Phase::Error)return false;
    State copy=s;Codec c{out.bytes,nullptr,0,sizeof(out.bytes),true};c.field(0x3156534E);c.field(9);c.field(0x41CEF1EA);code_state(copy,c,9);
    uint32_t h=checksum(out.bytes,c.at);c.field(h);out.size=c.at;return c.good;
}
bool load(State&s,const uint8_t*data,size_t len){
    if(!data||len<16||len>16384||len%4)return false;
    Codec c{nullptr,data,0,len-4,true};if(c.field(0)!=0x3156534E)return false;
    const unsigned version=c.field(0);
    if((version!=3&&version!=4&&version!=5&&version!=6&&version!=7&&version!=8&&version!=9)||c.field(0)!=0x41CEF1EA)return false;
    State tmp{};code_state(tmp,c,version);if(!c.good||c.at!=len-4||!validate(tmp)||tmp.phase==Phase::Error)return false;
    uint32_t h=checksum(data,len-4),stored=0;for(int i=0;i<4;i++)stored|=(uint32_t)data[len-4+i]<<(8*i);if(h!=stored)return false;
    s=tmp;return true;
}
} // namespace spire
