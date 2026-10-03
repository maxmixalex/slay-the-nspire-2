#include "core.hpp"
namespace spire {
static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
void heal(State&s,int amount){int old=s.hp;s.hp=(int16_t)min(s.max_hp,s.hp+max(0,amount));record(s,LogKind::Heal,s.hp-old);}
static void max_hp(State&s,int amount){amount=min(amount,999-s.max_hp);s.max_hp+=amount;s.hp+=amount;}
static void lose_hp(State&s,int amount){s.hp=(int16_t)max(0,s.hp-amount);if(!s.hp)s.phase=Phase::Dead;}
static void add_gold(State&s,int amount){s.gold=(int16_t)min(9999,s.gold+amount);record(s,LogKind::Gold,amount);}
void create_map(State&s){
    // A finite expedition graph, not the original game's map generator.
    const Room layout[MapRows][MapColumns]={
        {Room::Combat,Room::Combat,Room::Combat},
        {Room::Event,Room::Shop,Room::Event},
        {Room::Camp,Room::Combat,Room::Shop},
        {Room::Combat,Room::Event,Room::Camp},
        {Room::Shop,Room::Camp,Room::Event},
        {Room::Combat,Room::Combat,Room::Combat},
        {Room::Event,Room::Treasure,Room::Event},
        {Room::Camp,Room::Shop,Room::Combat},
        {Room::Combat,Room::Event,Room::Shop},
        {Room::Shop,Room::Event,Room::Camp},
        {Room::Camp,Room::Camp,Room::Camp},
        {Room::Final,Room::Final,Room::Final}
    };
    unsigned event_offset=random_below(s.rng_world,3);
    for(int row=0;row<MapRows;row++)for(int col=0;col<MapColumns;col++){
        auto&n=s.map[row*3+col];n.room=layout[row][col];n.visited=0;
        n.links=(uint8_t)(row==MapRows-1?0:col==0?3:col==2?6:7);
        n.event=(uint8_t)((row+col+event_offset)%3);
    }
    s.current_node=-1;s.next_row=0;s.floor=0;s.event_first=-1;s.phase=Phase::Map;
}
bool reachable(const State&s,int node){
    if(node<0||node>=MapNodes||s.next_row>=MapRows||node/3!=s.next_row||s.map[node].visited)return false;
    if(s.current_node<0)return s.next_row==0;
    return (s.map[s.current_node].links&(1u<<(node%3)))!=0;
}
void finish_room(State&s){
    if(s.hp<=0){s.phase=Phase::Dead;return;}
    if(s.practice){s.phase=Phase::Complete;return;}
    if(s.current_node<0||s.current_node>=MapNodes){s.phase=Phase::Error;return;}
    s.event_first=-1;s.event_remaining=0;s.map[s.current_node].visited=1;s.next_row=(uint8_t)(s.current_node/3+1);
    s.phase=s.next_row>=MapRows?Phase::Complete:Phase::Map;s.revision++;
}
static void enter_shop(State&s){
    s.phase=Phase::Shop;s.shop_sold=0;
    for(int i=0;i<4;i++){
        int card;bool repeated;
        do{card=3+(int)random_below(s.rng_world,CardKinds-3);repeated=false;for(int j=0;j<i;j++)if(s.shop_cards[j]==card)repeated=true;}while(repeated);
        s.shop_cards[i]=(uint8_t)card;
        int base=card==DarkEmbrace?150:card==FeelNoPain||card==Whirlwind||card==Inflame||card==BurningPact?75:50;
        s.shop_prices[i]=(int16_t)(base*(80+(int)random_below(s.rng_world,41))/100);
    }
    s.shop_relic=0;int available[3],count=0;
    for(int i=0;i<3;i++)if(!(s.relics&(1<<i)))available[count++]=1<<i;
    if(count)s.shop_relic=(uint8_t)available[random_below(s.rng_world,(uint32_t)count)];
    if(!s.shop_relic)s.shop_sold|=1<<4;
    s.shop_potion=(uint8_t)(1+random_below(s.rng_world,3));s.shop_prices[4]=150;s.shop_prices[5]=50;
}
bool choose_node(State&s,int column){
    int id=s.next_row*3+column;
    if(s.phase!=Phase::Map||column<0||column>=3||!reachable(s,id))return false;
    s.current_node=(int16_t)id;s.floor=(uint8_t)(s.next_row+1);s.encounter=(uint8_t)(s.floor>=6?1:0);
    switch(s.map[id].room){
    case Room::Combat:case Room::Final:enter_combat(s);break;
    case Room::Shop:enter_shop(s);break;
    case Room::Camp:s.phase=Phase::Camp;break;
    case Room::Treasure:s.phase=Phase::Treasure;break;
    case Room::Event:
        s.event_id=(EventId)s.map[id].event;s.event_page=s.event_count=s.event_remaining=0;s.event_first=-1;
        s.event_value=(int16_t)(s.event_id==EventId::Vegetation?61+random_below(s.rng_world,39):0);s.phase=Phase::Event;break;
    }
    s.revision++;return true;
}
void award_victory(State&s){
    heal(s,6);s.last_gold=(int16_t)(10+random_below(s.rng_world,11)+(s.enemy_count>1?8:0));add_gold(s,s.last_gold);
}
int remove_price(const State&s){return min(9999,75+s.removals*25);}
bool buy(State&s,int item){
    if(s.phase!=Phase::Shop||item<0||item>=7)return false;
    if(item==6){if((s.shop_sold&64)||s.deck_count<=1||s.gold<remove_price(s))return false;s.phase=Phase::Remove;s.revision++;return true;}
    if(s.shop_sold&(1u<<item)||s.gold<s.shop_prices[item])return false;
    if(item<4){if(s.deck_count>=MaxDeck)return false;s.deck[s.deck_count++]={s.shop_cards[item],0};record(s,LogKind::Buy,s.shop_cards[item]);}
    else if(item==4){if(!s.shop_relic||s.relics&s.shop_relic)return false;s.relics|=s.shop_relic;record(s,LogKind::Relic,s.shop_relic);}
    else{int slot=-1;for(int i=0;i<2;i++)if(s.potions[i]==Potion::Empty){slot=i;break;}if(slot<0)return false;s.potions[slot]=(Potion)s.shop_potion;}
    s.gold-=s.shop_prices[item];s.shop_sold|=(uint8_t)(1u<<item);s.revision++;return true;
}
bool remove_card(State&s,int index){
    if(s.phase!=Phase::Remove||index<0||index>=s.deck_count||s.deck_count<=1||s.gold<remove_price(s))return false;
    record(s,LogKind::Remove,s.deck[index].kind);for(int i=index+1;i<s.deck_count;i++)s.deck[i-1]=s.deck[i];s.deck_count--;
    s.gold-=remove_price(s);s.shop_sold|=64;if(s.removals<255)s.removals++;s.phase=Phase::Shop;s.revision++;return true;
}
bool leave_shop(State&s){if(s.phase!=Phase::Shop)return false;finish_room(s);return true;}
bool has_upgrade(const State&s){for(int i=0;i<s.deck_count;i++)if(!s.deck[i].upgraded)return true;return false;}
bool event_choice(State&s,int option){
    if(s.phase!=Phase::Event||option<0||option>1)return false;
    if(s.event_page>=2){if(option!=0)return false;if(s.event_id==EventId::Vegetation&&s.event_page==4){s.encounter=1;enter_combat(s);}else finish_room(s);return true;}
    switch(s.event_id){
    case EventId::Baths:
        if(option==0){
            int damage=3+s.event_count;if(s.max_hp>997)return false;
            max_hp(s,2);lose_hp(s,damage);if(s.phase==Phase::Dead)break;
            if(s.event_count<250)s.event_count++;s.event_page=1;
        }else{
            if(s.event_page==0){heal(s,10);s.event_page=2;}else s.event_page=3;
        }break;
    case EventId::Vegetation:
        if(option==0){lose_hp(s,8);if(s.phase==Phase::Dead)break;add_gold(s,s.event_value);s.event_page=2;}
        else{heal(s,(s.max_hp*3)/10);s.event_page=4;} // Supported encounter substitutes Nibbits.
        break;
    case EventId::Grove:
        if(option==0){if(s.deck_count<2)return false;s.gold=0;s.event_remaining=2;s.event_first=-1;s.phase=Phase::Transform;}
        else{if(s.max_hp>994)return false;max_hp(s,5);s.event_page=3;}break;
    }
    record(s,LogKind::Event,(int)s.event_id,option);s.revision++;return true;
}
bool transform_card(State&s,int index){
    if(s.phase!=Phase::Transform||index<0||index>=s.deck_count||index==s.event_first||!s.event_remaining)return false;
    int old=s.deck[index].kind,k;do{k=3+(int)random_below(s.rng_world,CardKinds-3);}while(k==old);
    s.deck[index]={(uint8_t)k,0};record(s,LogKind::Transform,old,k);
    s.event_first=(int16_t)index;s.event_remaining--;
    if(!s.event_remaining){s.phase=Phase::Event;s.event_page=2;}
    s.revision++;return true;
}
bool take_treasure(State&s){
    if(s.phase!=Phase::Treasure)return false;
    bool got=false;for(int r=0;r<3;r++)if(!(s.relics&(1<<r))){s.relics|=(uint8_t)(1<<r);record(s,LogKind::Relic,1<<r);got=true;break;}
    if(!got)add_gold(s,50);finish_room(s);return true;
}
bool use_potion(State&s,int slot,int target){
    if(slot<0||slot>=2||s.potions[slot]==Potion::Empty||s.phase==Phase::ExhaustChoice||s.phase==Phase::Dead||s.phase==Phase::Complete||s.phase==Phase::Error)return false;
    auto p=s.potions[slot];
    if(p!=Potion::Blood&&s.phase!=Phase::Combat)return false;
    if(p==Potion::Fire){
        if(target<0||target>=s.enemy_count||s.enemies[target].hp<=0)return false;
        auto&e=s.enemies[target];int blocked=min(e.block,20);e.block-=blocked;e.hp=(int16_t)max(0,e.hp-20+blocked);record(s,LogKind::Damage,20,target);
        bool alive=false;for(int i=0;i<s.enemy_count;i++)alive|=s.enemies[i].hp>0;
        if(!alive){
            award_victory(s);for(int i=0;i<3;i++){int k;bool dup;do{k=3+(int)random_below(s.rng_world,CardKinds-3);dup=false;for(int j=0;j<i;j++)dup|=s.reward[j]==k;}while(dup);s.reward[i]=(uint8_t)k;}s.phase=Phase::Reward;
        }
    }else if(p==Potion::Block)s.block=(int16_t)min(9999,s.block+12);
    else heal(s,s.max_hp/5);
    s.potions[slot]=Potion::Empty;record(s,LogKind::Potion,(int)p);s.revision++;return true;
}
} // namespace spire
