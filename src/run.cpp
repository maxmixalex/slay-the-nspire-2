#include "core.hpp"
namespace spire {
static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
void heal(State&s,int amount){int old=s.hp;s.hp=(int16_t)min(s.max_hp,s.hp+max(0,amount));record(s,LogKind::Heal,s.hp-old);}
void increase_max_hp(State&s,int amount){amount=min(amount,999-s.max_hp);s.max_hp+=amount;s.hp+=amount;}
static void lose_hp(State&s,int amount){s.hp=(int16_t)max(0,s.hp-amount);if(!s.hp)s.phase=Phase::Dead;}
void gain_gold(State&s,int amount){s.gold=(int16_t)min(9999,s.gold+amount);record(s,LogKind::Gold,amount);}
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
    s.treasure_state=0;s.treasure_relic=0;
    if(s.hp<=0){s.phase=Phase::Dead;return;}
    if(s.practice){s.phase=Phase::Complete;return;}
    if(s.current_node<0||s.current_node>=MapNodes){s.phase=Phase::Error;return;}
    s.event_first=-1;s.event_remaining=0;s.map[s.current_node].visited=1;s.next_row=(uint8_t)(s.current_node/3+1);
    s.phase=s.next_row>=MapRows?Phase::Complete:Phase::Map;s.revision++;
}
static void enter_shop(State&s){
    s.phase=Phase::Shop;s.shop_sold=0;
    if(s.relics&MealTicket)heal(s,15);
    // Preserve original shelf categories/counts; limited implemented pools and
    // legacy RNG are explicit, not the original factory's probabilities.
    for(int i=0;i<ShopCardCount;i++){
        int card;bool repeated;
        uint8_t neutral[CardKinds];unsigned n_neutral=0;
        for(unsigned k=0;k<CardKinds;k++)if(card_colorless(k))neutral[n_neutral++]=(uint8_t)k;
        do{card=i<5?(int)random_reward_card(s):neutral[random_below(s.rng_world,n_neutral)];
           repeated=false;for(int j=0;j<i;j++)if(s.shop_cards[j]==card)repeated=true;
        }while(repeated);
        s.shop_cards[i]=(uint8_t)card;
        int rarity=card_rarity(card),base=rarity==4?150:rarity==3?75:50;
        if(card_colorless(card))base=base*6/5;
        s.shop_prices[i]=(int16_t)(base*(80+(int)random_below(s.rng_world,41))/100);
    }
    // Three distinct, unowned relics. Exhausted pools produce unavailable slots.
    unsigned available[RelicKinds],n=0;
    for(const auto&d:relic_definitions)if(d.id!=BurningBlood&&d.id!=RingOfTheSnake&&!(s.relics&d.id))available[n++]=d.id;
    for(int i=0;i<3;i++){
        unsigned id=0;if(n){unsigned j=random_below(s.rng_world,n);id=available[j];available[j]=available[--n];}
        s.shop_relics[i]=(uint32_t)id;s.shop_prices[ShopRelicStart+i]=150;
        if(!id)s.shop_sold|=uint16_t(1u<<(ShopRelicStart+i));
        s.shop_potions[i]=(uint8_t)(1+random_below(s.rng_world,PotionKinds));
        s.shop_prices[ShopPotionStart+i]=potion_definitions[s.shop_potions[i]-1].rarity==3?100:
            potion_definitions[s.shop_potions[i]-1].rarity==2?75:50;
    }
}
bool choose_node(State&s,int column){
    int id=s.next_row*3+column;
    if(s.phase!=Phase::Map||column<0||column>=3||!reachable(s,id))return false;
    s.current_node=(int16_t)id;s.floor=(uint8_t)(s.next_row+1);s.encounter=(uint8_t)(s.floor>=6?1:0);
    switch(s.map[id].room){
    case Room::Combat:case Room::Final:enter_combat(s);break;
    case Room::Shop:enter_shop(s);break;
    case Room::Camp:s.phase=Phase::Camp;break;
    case Room::Treasure:s.phase=Phase::Treasure;s.treasure_state=0;s.treasure_relic=0;break;
    case Room::Event:
        s.event_id=(EventId)s.map[id].event;s.event_page=s.event_count=s.event_remaining=0;s.event_first=-1;
        s.event_value=(int16_t)(s.event_id==EventId::Vegetation?61+random_below(s.rng_world,39):0);s.phase=Phase::Event;break;
    }
    s.revision++;return true;
}
void award_victory(State&s){
    relic_hook(s,RelicHook::Victory);
    s.last_gold=(int16_t)(10+random_below(s.rng_world,11)+(s.enemy_count>1?8:0));
    s.reward_claimed=0;s.reward_potion=Potion::Empty;
    // Legacy expedition only: 40% initial, +/-10 percentage points after a roll.
    // It does not claim the original float RNG, encounter modifiers or item pools.
    bool dropped=random_below(s.rng_world,100)<s.potion_chance;
    s.potion_chance=(uint8_t)(dropped?max(0,s.potion_chance-10):min(100,s.potion_chance+10));
    if(dropped)s.reward_potion=(Potion)(1+random_below(s.rng_world,PotionKinds));
}
bool claim_reward_gold(State&s){
    if(s.phase!=Phase::Reward||(s.reward_claimed&1))return false;
    gain_gold(s,s.last_gold);s.reward_claimed|=1;s.revision++;return true;
}
bool claim_reward_potion(State&s){
    if(s.phase!=Phase::Reward||(s.reward_claimed&4)||s.reward_potion==Potion::Empty)return false;
    for(auto& p:s.potions)if(p==Potion::Empty){p=s.reward_potion;s.reward_claimed|=4;s.revision++;return true;}
    return false;
}
bool leave_rewards(State&s){if(s.phase!=Phase::Reward)return false;finish_room(s);return true;}
int rest_heal_amount(const State&s){return s.max_hp*3/10+((s.relics&RegalPillow)?15:0);}
int remove_price(const State&s){return min(9999,75+s.removals*25);}
bool buy(State&s,int item){
    if(s.phase!=Phase::Shop||item<0||item>ShopRemove)return false;
    if(item==ShopRemove){
        if((s.shop_sold&(1u<<ShopRemove))||s.deck_count<=1||s.gold<remove_price(s))return false;
        s.phase=Phase::Remove;s.revision++;return true;
    }
    if((s.shop_sold&(1u<<item))||s.gold<s.shop_prices[item])return false;
    if(item<ShopCardCount){
        if(s.deck_count>=MaxDeck)return false;
        s.deck[s.deck_count++]={s.shop_cards[item],0};record(s,LogKind::Buy,s.shop_cards[item]);
    }else if(item<ShopPotionStart){
        int balance=s.gold;s.gold-=s.shop_prices[item];
        if(!gain_relic(s,s.shop_relics[item-ShopRelicStart])){s.gold=(int16_t)balance;return false;}
    }else{
        int slot=-1;for(int i=0;i<2;i++)if(s.potions[i]==Potion::Empty){slot=i;break;}
        if(slot<0||!potion_definition((Potion)s.shop_potions[item-ShopPotionStart]))return false;
        s.potions[slot]=(Potion)s.shop_potions[item-ShopPotionStart];
    }
    if(item<ShopRelicStart||item>=ShopPotionStart)s.gold-=s.shop_prices[item];
    s.shop_sold|=(uint16_t)(1u<<item);s.revision++;return true;
}
bool remove_card(State&s,int index){
    if(s.phase!=Phase::Remove||index<0||index>=s.deck_count||s.deck_count<=1||s.gold<remove_price(s))return false;
    record(s,LogKind::Remove,s.deck[index].kind);for(int i=index+1;i<s.deck_count;i++)s.deck[i-1]=s.deck[i];s.deck_count--;
    s.gold-=remove_price(s);s.shop_sold|=(1u<<ShopRemove);if(s.removals<255)s.removals++;s.phase=Phase::Shop;s.revision++;return true;
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
            increase_max_hp(s,2);lose_hp(s,damage);if(s.phase==Phase::Dead)break;
            if(s.event_count<250)s.event_count++;s.event_page=1;
        }else{
            if(s.event_page==0){heal(s,10);s.event_page=2;}else s.event_page=3;
        }break;
    case EventId::Vegetation:
        if(option==0){lose_hp(s,8);if(s.phase==Phase::Dead)break;gain_gold(s,s.event_value);s.event_page=2;}
        else{heal(s,(s.max_hp*3)/10);s.event_page=4;} // Supported encounter substitutes Nibbits.
        break;
    case EventId::Grove:
        if(option==0){if(s.deck_count<2)return false;s.gold=0;s.event_remaining=2;s.event_first=-1;s.phase=Phase::Transform;}
        else{if(s.max_hp>994)return false;increase_max_hp(s,5);s.event_page=3;}break;
    }
    record(s,LogKind::Event,(int)s.event_id,option);s.revision++;return true;
}
bool transform_card(State&s,int index){
    if(s.phase!=Phase::Transform||index<0||index>=s.deck_count||index==s.event_first||!s.event_remaining)return false;
    int old=s.deck[index].kind,k;do{k=(int)random_reward_card(s);}while(k==old);
    s.deck[index]={(uint8_t)k,0};record(s,LogKind::Transform,old,k);
    s.event_first=(int16_t)index;s.event_remaining--;
    if(!s.event_remaining){s.phase=Phase::Event;s.event_page=2;}
    s.revision++;return true;
}
bool open_treasure(State&s){
    if(s.phase!=Phase::Treasure||s.treasure_state)return false;
    s.treasure_relic=random_relic(s);s.treasure_state=1;s.revision++;return true;
}
bool take_treasure(State&s){
    if(s.phase!=Phase::Treasure||s.treasure_state!=1)return false;
    if(s.treasure_relic){if(!gain_relic(s,s.treasure_relic))return false;}
    else gain_gold(s,50);
    s.treasure_state=2;s.revision++;return true;
}
bool leave_treasure(State&s){
    if(s.phase!=Phase::Treasure||!s.treasure_state)return false;
    finish_room(s);return true;
}
} // namespace spire
