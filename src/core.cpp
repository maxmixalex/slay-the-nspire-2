#include "core.hpp"

namespace spire {
// Inspected against the user's unmodified v0.111.0 / 41cef1ea assembly.
// A rule subset, not a general C# interpreter and not the complete game.
#include "card_definitions.inc"
static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
void record(State&s,LogKind k,int a,int b){
    if(s.log_count==24){for(int i=1;i<24;i++)s.logs[i-1]=s.logs[i];s.log_count--;}
    s.logs[s.log_count++]={k,(int16_t)a,(int16_t)b};
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
static void draw(State&s,int n){
    if(s.no_draw)return;
    int got=0;
    while(n-->0&&s.hand.count<MaxHand){
        if(!s.draw.count){if(!s.discard.count)break;shuffle(s);}
        int id=remove(s.draw,s.draw.count-1);put(s,s.hand,id,Zone::Hand);got++;
    }
    if(got)record(s,LogKind::Draw,got);
}
int cost(Card c){if(c.kind>=CardKinds)return 99;return definitions[c.kind].cost-((c.kind==DarkEmbrace||c.kind==BodySlam)&&c.upgraded?1:0);}
int card_base_damage(const State&s,Card c){
    if(c.kind>=CardKinds)return 0;
    if(c.kind==BodySlam)return s.block;
    return definitions[c.kind].damage+(c.upgraded?definitions[c.kind].damage_up:0);
}
int damage(const State&s,Card c,int t){
    if(c.kind>=CardKinds)return 0;
    int d=card_base_damage(s,c);
    if(d<=0&&c.kind!=BodySlam)return 0;
    d=max(0,d+s.strength);
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
        do{k=3+(int)random_below(s.rng_world,CardKinds-3);duplicate=false;for(int j=0;j<i;j++)if(s.reward[j]==k)duplicate=true;}while(duplicate);
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
static void resolve(State&s){
    while(s.effect_count&&s.phase==Phase::Combat){
        Effect e=s.effects[0];for(int i=1;i<s.effect_count;i++)s.effects[i-1]=s.effects[i];s.effect_count--;
        switch(e.kind){
        case EffectKind::Damage:
            if(e.target>=0&&e.target<s.enemy_count&&s.enemies[e.target].hp>0){
                auto& t=s.enemies[e.target];int amount=max(0,e.value+s.strength)*relic_attack_multiplier(s);
                if(t.vulnerable)amount=amount*3/2;
                int absorb=min(t.block,amount);t.block-=absorb;t.hp=max(0,t.hp-amount+absorb);
                record(s,LogKind::Damage,amount,e.target);
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
        case EffectKind::NoDraw:s.no_draw=1;record(s,LogKind::NoDraw);break;
        case EffectKind::Energy:s.energy=min(999,s.energy+e.value);record(s,LogKind::Energy,e.value);break;
        case EffectKind::LoseHp:
            s.hp=max(0,s.hp-e.value);record(s,LogKind::LoseHp,e.value);
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
        case EffectKind::Draw:draw(s,e.value);break;
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
                if(definitions[s.cards[id].kind].type==CardType::Power)put(s,s.powers,id,Zone::Power);
                else if(definitions[s.cards[id].kind].exhausts)exhaust_id(s,id);
                else put(s,s.discard,id,Zone::Discard);s.playing=-1;}
            relic_attack_end(s);if(!alive(s))victory(s);break;
        }
    }
}
void enter_combat(State&s){
    s.phase=Phase::Combat;s.block=s.strength=s.pain=s.embrace=s.dexterity=0;s.double_attack=0;s.power_count=0;s.no_draw=0;
    s.turn=1;s.energy=3;s.playing=-1;s.effect_count=s.card_count=0;
    s.draw.count=s.hand.count=s.discard.count=s.exhaust.count=s.powers.count=0;
    s.enemy_count=s.encounter?2:1;
    for(int i=0;i<s.enemy_count;i++)s.enemies[i]={(int16_t)(42+random_below(s.rng_world,5)),0,0,0,0,(uint8_t)(s.enemy_count==1?0:(i==0?1:2))};
    for(int i=0;i<s.enemy_count;i++)s.enemies[i].max_hp=s.enemies[i].hp;
    for(int i=0;i<s.deck_count;i++){int id=make_card(s,s.deck[i]);if(id>=0)put(s,s.discard,id,Zone::Discard);}
    shuffle(s);
    if(s.practice&&s.floor==1){
        // Explicit practice opening: lets the player exercise two exhaust hooks.
        const uint8_t opening[5]={FeelNoPain,DarkEmbrace,BurningPact,TrueGrit,Strike};
        for(int k=0;k<5;k++)for(int j=0;j<s.draw.count;j++)if(s.cards[s.draw.ids[j]].kind==opening[k]){
            int id=remove(s.draw,j);put(s,s.hand,id,Zone::Hand);break;}
    }else draw(s,5+((s.relics&BagOfPreparation)?2:0));
    relic_hook(s,RelicHook::CombatStart);relic_hook(s,RelicHook::TurnStart);
    record(s,LogKind::Turn,1);s.revision++;
}
void new_run(State&s,uint32_t seed,bool practice){
    // Aggregate initialization is well-defined on every supported backend.
    s=State{};s.seed=seed?seed:1;s.rng_cards=s.seed^0xA341316Cu;s.rng_world=s.seed^0xC8013EA4u;
    s.hp=s.max_hp=80;s.floor=1;s.practice=practice;s.playing=-1;s.gold=99;s.current_node=-1;s.event_first=-1;
    if(practice){for(int i=0;i<CardKinds;i++)s.deck[s.deck_count++]={(uint8_t)i,(uint8_t)(i==DarkEmbrace||i==TrueGrit)};}
    else {for(int i=0;i<5;i++)s.deck[s.deck_count++]={Strike,0};for(int i=0;i<4;i++)s.deck[s.deck_count++]={Defend,0};s.deck[s.deck_count++]={Bash,0};}
    gain_relic(s,BurningBlood);
    create_map(s);
    if(practice){s.current_node=1;s.floor=1;s.encounter=0;enter_combat(s);}
}
bool play(State&s,int h,int target){
    if(s.phase!=Phase::Combat||s.effect_count||s.playing!=-1||h<0||h>=s.hand.count)return false;
    if(s.hand.ids[h]>=s.card_count){fail(s);return false;}
    Card c=s.cards[s.hand.ids[h]];if(c.kind>=CardKinds){fail(s);return false;}const Definition&d=definitions[c.kind];int n=cost(c);int energy=n<0?s.energy:n;
    if(energy>s.energy|| (d.targeted&&(target<0||target>=s.enemy_count||s.enemies[target].hp==0)))return false;
    if(c.kind==Anger&&s.card_count==MaxCards){fail(s);return false;}
    s.energy-=energy;s.playing=remove(s.hand,h);s.zones[s.playing]=Zone::Limbo;
    int hit=card_base_damage(s,c),block=d.block+(c.upgraded?d.block_up:0),cards=d.draw+(c.upgraded?d.draw_up:0),power=d.power+(c.upgraded?d.power_up:0);
    if(d.type==CardType::Attack)relic_attack_begin(s);
    if(block)block=max(0,block+s.dexterity);
    record(s,LogKind::Play,c.kind,c.upgraded);
    if(d.hp_loss)enqueue(s,EffectKind::LoseHp,d.hp_loss);
    if(d.energy_gain)enqueue(s,EffectKind::Energy,d.energy_gain+(c.upgraded?d.energy_up:0));
    if(block)enqueue(s,EffectKind::Block,block);
    if(c.kind==FiendFire){
        const int count=s.hand.count;
        for(int i=0;i<count;i++)enqueue(s,EffectKind::ExhaustCard,s.hand.ids[i]);
        for(int i=0;i<count;i++)enqueue(s,EffectKind::Damage,hit,target);
    }else if(c.kind==Whirlwind)enqueue(s,EffectKind::WhirlwindHits,hit,energy);
    else if(c.kind==Thunderclap){
        for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Damage,hit,i);
        for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Vulnerable,power,i);
    }else if(hit||c.kind==BodySlam){
        enqueue(s,EffectKind::Damage,hit,target);
        if(c.kind==TwinStrike)enqueue(s,EffectKind::Damage,hit,target);
    }
    if(c.kind==Uppercut){enqueue(s,EffectKind::Weak,power,target);enqueue(s,EffectKind::Vulnerable,power,target);}
    if(c.kind==Bash)enqueue(s,EffectKind::Vulnerable,power,target);
    if(c.kind==BurningPact||(c.kind==TrueGrit&&c.upgraded))enqueue(s,EffectKind::ChooseExhaust);
    if(c.kind==TrueGrit&&!c.upgraded)enqueue(s,EffectKind::RandomExhaust);
    if(cards)enqueue(s,EffectKind::Draw,cards);
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
bool end_turn(State&s){
    if(s.phase!=Phase::Combat||s.effect_count||s.playing!=-1)return false;
    relic_hook(s,RelicHook::TurnEnd);
    s.no_draw=0;
    while(s.hand.count){int id=remove(s.hand,0);put(s,s.discard,id,Zone::Discard);}
    for(int i=0;i<s.enemy_count;i++){
        auto&e=s.enemies[i];if(e.hp==0)continue;e.block=0;
        record(s,LogKind::Enemy,i,e.move);
        int amount=enemy_damage(e),absorb=min(s.block,amount);s.block-=absorb;s.hp=max(0,s.hp-amount+absorb);
        if(e.move==1)e.block=5;
        if(e.move==2)e.strength=min(9999,e.strength+2);
        if(e.vulnerable>0)e.vulnerable--;
        if(e.weak>0)e.weak--;
        e.move=(uint8_t)((e.move+1)%3);
        if(!s.hp){s.phase=Phase::Dead;break;}
    }
    if(s.hp){s.block=0;s.energy=3;s.turn++;draw(s,5);relic_hook(s,RelicHook::TurnStart);record(s,LogKind::Turn,s.turn);}
    s.revision++;return true;
}
bool choose_reward(State&s,int index){
    if(s.phase!=Phase::Reward||index< -1||index>2)return false;
    if(index>=0){if(s.deck_count==MaxDeck){fail(s);return false;}s.deck[s.deck_count++]={s.reward[index],0};record(s,LogKind::Reward,s.reward[index]);}
    finish_room(s);return true;
}
bool camp(State&s,bool upgrading){
    if(s.phase!=Phase::Camp)return false;
    if(upgrading){if(!has_upgrade(s))return false;s.phase=Phase::Upgrade;s.revision++;}
    else{heal(s,(s.max_hp*3)/10);finish_room(s);}
    return true;
}
bool upgrade_card(State&s,int index){
    if(s.phase!=Phase::Upgrade||index<0||index>=s.deck_count||s.deck[index].upgraded)return false;
    s.deck[index].upgraded=1;record(s,LogKind::Upgrade,s.deck[index].kind);finish_room(s);return true;
}
const Pile& pile(const State&s,Zone z){switch(z){case Zone::Draw:return s.draw;case Zone::Hand:return s.hand;case Zone::Discard:return s.discard;case Zone::Exhaust:return s.exhaust;default:return s.powers;}}
bool validate(const State&s){
    if((int)s.phase>(int)Phase::Error||s.practice>1||s.floor>MapRows||s.max_hp<1||s.max_hp>999||s.hp<0||s.hp>s.max_hp||s.energy<0||s.energy>999||s.turn<0||s.turn>20000)return false;
    if(s.no_draw>1)return false;
    if(s.block<0||s.block>9999||s.strength<0||s.strength>9999||s.pain<0||s.pain>9999||s.embrace<0||s.embrace>99)return false;
    if(s.deck_count<1||s.deck_count>MaxDeck||s.card_count>MaxCards||s.hand.count>MaxHand||s.effect_count>MaxEffects||s.log_count>24||s.power_count>2)return false;
    if(s.enemy_count>2||s.playing< -1||s.playing>=s.card_count)return false;
    if(s.current_node< -1||s.current_node>=MapNodes||s.next_row>MapRows||s.encounter>1||s.relics>=1024||s.gold<0||s.gold>9999||s.last_gold<0||s.last_gold>99)return false;
    if((int)s.event_id>2||s.event_page>4||s.event_remaining>2||s.event_first< -1||s.event_first>=s.deck_count||s.event_value<0||s.event_value>9999)return false;
    if(s.shop_sold>127||s.shop_relic>=1024||(s.shop_relic&&!relic_definition(s.shop_relic))||s.shop_potion>3)return false;
    if(s.relic_count>RelicKinds||s.dexterity<0||s.dexterity>9999||s.double_attack>1||s.relic_flash>=1024)return false;
    uint16_t owned=0;
    for(unsigned i=0;i<s.relic_count;i++){unsigned index=s.relic_order[i];if(index>=RelicKinds)return false;unsigned bit=relic_definitions[index].id;if(owned&bit)return false;owned|=bit;}
    if(owned!=s.relics)return false;
    for(int i=0;i<RelicKinds;i++)if(s.relic_counters[i]>(relic_definitions[i].id==PenNib?9:relic_definitions[i].id==HappyFlower?2:0))return false;
    for(auto p:s.potions)if((int)p>3)return false;
    for(auto c:s.shop_cards)if(c>=CardKinds)return false;
    for(auto p:s.shop_prices)if(p<0||p>9999)return false;
    for(int i=0;i<MapNodes;i++){auto n=s.map[i];if((int)n.room>(int)Room::Final||n.links>7||n.visited>1||n.event>2)return false;}
    if(s.phase==Phase::Map&&s.next_row>=MapRows)return false;
    if(s.phase==Phase::Transform&&(s.event_id!=EventId::Grove||s.event_remaining<1))return false;
    if((s.phase==Phase::Combat||s.phase==Phase::ExhaustChoice)&&(!s.enemy_count||s.turn<1))return false;
    for(int i=0;i<s.deck_count;i++)if(s.deck[i].kind>=CardKinds||s.deck[i].upgraded>1)return false;
    for(int i=0;i<s.card_count;i++)if(s.cards[i].kind>=CardKinds||s.cards[i].upgraded>1)return false;
    for(int i=0;i<3;i++)if(s.reward[i]>=CardKinds)return false;
    for(int i=0;i<s.power_count;i++)if(s.power_order[i]>1||(i&&s.power_order[0]==s.power_order[1]))return false;
    uint8_t seen[MaxCards]={};
    for(int z=0;z<5;z++){const Pile&p=pile(s,(Zone)z);if(p.count>MaxCards)return false;
        for(int i=0;i<p.count;i++){int id=p.ids[i];if(id>=s.card_count||seen[id]||s.zones[id]!=(Zone)z)return false;seen[id]=1;}}
    if(s.playing>=0){if(seen[s.playing]||s.zones[s.playing]!=Zone::Limbo)return false;seen[s.playing]=1;}
    for(int i=0;i<s.card_count;i++)if(!seen[i])return false;
    for(int i=0;i<s.enemy_count;i++){const auto&e=s.enemies[i];if(e.hp<0||e.hp>e.max_hp||e.max_hp<42||e.max_hp>46||e.block<0||e.block>9999||e.strength<0||e.strength>9999||e.vulnerable<0||e.vulnerable>9999||e.move>2||e.weak<0||e.weak>9999)return false;}
    for(int i=0;i<s.effect_count;i++){const auto&e=s.effects[i];if((int)e.kind>(int)EffectKind::WhirlwindHits||e.value<0||e.value>9999||e.target<0||e.target>=(e.kind==EffectKind::WhirlwindHits?1000:MaxEnemies))return false;
        if(e.kind==EffectKind::ExhaustCard&&e.value>=s.card_count)return false;}
    if(s.phase==Phase::ExhaustChoice&&(s.playing<0||!s.effect_count||!s.hand.count))return false;
    if(s.phase!=Phase::ExhaustChoice&&s.phase!=Phase::Error&&(s.playing>=0||s.effect_count))return false;
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
 F(s.relic_count);for(auto&v:s.relic_order)F(v);for(auto&v:s.relic_counters)F(v);
 F(s.dexterity);F(s.double_attack);F(s.relic_flash);F(s.relic_revision);
 F(s.current_node);F(s.next_row);F(s.encounter);F(s.relics);F(s.removals);F(s.gold);F(s.last_gold);
 for(auto&n:s.map){F(n.room);F(n.links);F(n.visited);F(n.event);}
 for(auto&p:s.potions)F(p);for(auto&x:s.shop_cards)F(x);F(s.shop_relic);F(s.shop_potion);F(s.shop_sold);for(auto&x:s.shop_prices)F(x);
 F(s.event_id);F(s.event_page);F(s.event_count);F(s.event_remaining);F(s.event_value);F(s.event_first);
 for(int i=0;i<3;i++)F(s.reward[i]);F(s.log_count);if(s.log_count>24){c.good=false;return;}for(int i=0;i<s.log_count;i++){auto&l=s.logs[i];F(l.kind);F(l.a);F(l.b);}
if(version>=4){F(s.no_draw);for(auto&e:s.enemies)F(e.weak);}
#undef F
}
bool save(const State&s,SaveData&out){
    if(!validate(s)||s.phase==Phase::Error)return false;
    State copy=s;Codec c{out.bytes,nullptr,0,sizeof(out.bytes),true};c.field(0x3156534E);c.field(4);c.field(0x41CEF1EA);code_state(copy,c,4);
    uint32_t h=checksum(out.bytes,c.at);c.field(h);out.size=c.at;return c.good;
}
bool load(State&s,const uint8_t*data,size_t len){
    if(len<16||len>16384||len%4)return false;
    Codec c{nullptr,data,0,len-4,true};if(c.field(0)!=0x3156534E)return false;
    const unsigned version=c.field(0);
    if((version!=3&&version!=4)||c.field(0)!=0x41CEF1EA)return false;
    State tmp{};code_state(tmp,c,version);if(!c.good||c.at!=len-4||!validate(tmp)||tmp.phase==Phase::Error)return false;
    uint32_t h=checksum(data,len-4),stored=0;for(int i=0;i<4;i++)stored|=(uint32_t)data[len-4+i]<<(8*i);if(h!=stored)return false;
    s=tmp;return true;
}
} // namespace spire
