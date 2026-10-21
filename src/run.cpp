#include "core.hpp"
#include "world_run.hpp"
#include "world_card_factory.hpp"
#include "original_events.hpp"
namespace spire {
namespace {static bool world_make_rewards(State&);static bool world_buy(State&,int);static bool world_open_treasure(State&);static bool world_take_treasure(State&);}
static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
void heal(State&s,int amount){int old=s.hp;s.hp=(int16_t)min(s.max_hp,s.hp+max(0,amount));record(s,LogKind::Heal,s.hp-old);if(s.world_relics.active&&s.hp!=old)world_relic_hook(s,WorldRelicHook::AfterHpChanged,old,s.hp,-1);}
void increase_max_hp(State&s,int amount){int old=s.hp;amount=min(amount,999-s.max_hp);s.max_hp+=amount;s.hp+=amount;if(s.world_relics.active&&s.hp!=old)world_relic_hook(s,WorldRelicHook::AfterHpChanged,old,s.hp,-1);}
static void lose_hp(State&s,int amount){player_hp_loss(s,amount,false,false);}
void gain_gold(State&s,int amount){
    if(s.world_relics.active&&amount>0){
        using reference::math::Real;Real value=Real::integer(amount);
        for(unsigned i=0;i<s.world_relics.count;++i){const auto&r=s.world_relics.entries[i];if(r.flags&WorldRelicMelted)continue;
            if(r.id==WorldRelicEctoplasm){if(Real::integer(0)<value)world_relic_flash(s,r.id);value=Real::integer(0);}
            else if(r.id==WorldRelicBowlerHat&&Real::integer(0)<value){value=value*(Real::integer(5)/Real::integer(4));world_relic_flash(s,r.id);}
        }
        amount=Real::integer(9999)<value?9999:world_float_truncate(value);
    }
    s.gold=int16_t(min(9999,max(0,s.gold+amount)));record(s,LogKind::Gold,amount);
    if(s.world_relics.active&&amount>0)world_relic_hook(s,WorldRelicHook::AfterGoldGained,amount);
}
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
    unsigned event_offset=random_below(s.rng_world,EventKinds);
    for(int row=0;row<MapRows;row++)for(int col=0;col<MapColumns;col++){
        auto&n=s.map[row*3+col];n.room=layout[row][col];n.visited=0;
        n.links=(uint8_t)(row==MapRows-1?0:col==0?3:col==2?6:7);
        n.event=(uint8_t)((row+col+event_offset)%EventKinds);
    }
    s.current_node=-1;s.next_row=0;s.floor=0;s.event_first=-1;s.phase=Phase::Map;
}
bool reachable(const State&s,int node){
    if(s.world.active)return node>=0&&world_reachable(s,unsigned(node));
    if(node<0||node>=MapNodes||s.next_row>=MapRows||node/3!=s.next_row||s.map[node].visited)return false;
    if(s.current_node<0)return s.next_row==0;
    return (s.map[s.current_node].links&(1u<<(node%3)))!=0;
}
void finish_room(State&s){
    if(s.world.active){world_finish_room(s);return;}
    s.treasure_state=0;s.treasure_relic=0;s.event_deck_action=EventDeckAction::None;s.necro.grimoire_pending=0;
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
        CardKind neutral[CardKinds];unsigned n_neutral=0;
        for(unsigned k=0;k<CardKinds;k++)if(card_colorless(k))neutral[n_neutral++]=(CardKind)k;
        do{card=i<5?(int)random_reward_card(s):neutral[random_below(s.rng_world,n_neutral)];
           repeated=false;for(int j=0;j<i;j++)if(s.shop_cards[j]==card)repeated=true;
        }while(repeated);
        s.shop_cards[i]=(CardKind)card;
        int rarity=card_rarity(card),base=rarity==4?150:rarity==3?75:50;
        if(card_colorless(card))base=base*6/5;
        s.shop_prices[i]=(int16_t)(base*(80+(int)random_below(s.rng_world,41))/100);
    }
    // Three distinct, unowned relics. Exhausted pools produce unavailable slots.
    RelicMask available[RelicKinds];unsigned n=0;
    for(const auto&d:relic_definitions)if(d.id!=BurningBlood&&d.id!=RingOfTheSnake&&d.id!=CrackedCore&&d.id!=DivineRight&&d.id!=BoundPhylactery&&!(s.relics&d.id))available[n++]=d.id;
    for(int i=0;i<3;i++){
        RelicMask id=0;if(n){unsigned j=random_below(s.rng_world,n);id=available[j];available[j]=available[--n];}
        s.shop_relics[i]=id;s.shop_prices[ShopRelicStart+i]=150;
        if(!id)s.shop_sold|=uint16_t(1u<<(ShopRelicStart+i));
        s.shop_potions[i]=(uint8_t)random_potion(s);
        s.shop_prices[ShopPotionStart+i]=potion_definitions[s.shop_potions[i]-1].rarity==3?100:
            potion_definitions[s.shop_potions[i]-1].rarity==2?75:50;
    }
}
bool choose_node(State&s,int column){
    if(s.world.active)return world_choose_column(s,column);
    int id=s.next_row*3+column;
    if(s.phase!=Phase::Map||column<0||column>=3||!reachable(s,id))return false;
    s.current_node=(int16_t)id;s.floor=(uint8_t)(s.next_row+1);s.encounter=(uint8_t)(s.floor>=6?1:0);
    switch(s.map[id].room){
    case Room::Combat:case Room::Final:enter_combat(s);break;
    case Room::Shop:enter_shop(s);break;
    case Room::Camp:s.phase=Phase::Camp;break;
    case Room::Treasure:s.phase=Phase::Treasure;s.treasure_state=0;s.treasure_relic=0;break;
    case Room::Event:
        s.event_deck_action=EventDeckAction::None;s.event_id=(EventId)s.map[id].event;s.event_page=s.event_count=s.event_remaining=0;s.event_first=-1;
        s.event_value=(int16_t)(s.event_id==EventId::Vegetation?61+random_below(s.rng_world,39):0);s.phase=Phase::Event;break;
    }
    s.revision++;return true;
}
void award_victory(State&s){
    relic_hook(s,RelicHook::Victory);
    if(s.world.active){world_make_rewards(s);return;}
    s.last_gold=(int16_t)(10+random_below(s.rng_world,11)+(s.enemy_count>1?8:0));
    s.reward_claimed=0;s.reward_potion=Potion::Empty;
    // Legacy expedition only: 40% initial, +/-10 percentage points after a roll.
    // It does not claim the original float RNG, encounter modifiers or item pools.
    bool dropped=random_below(s.rng_world,100)<s.potion_chance;
    s.potion_chance=(uint8_t)(dropped?max(0,s.potion_chance-10):min(100,s.potion_chance+10));
    if(dropped)s.reward_potion=random_potion(s);
}
bool claim_reward_gold(State&s){
    if(s.phase!=Phase::Reward||(s.reward_claimed&1))return false;
    gain_gold(s,s.last_gold);s.reward_claimed|=1;s.revision++;return true;
}
bool claim_reward_potion(State&s){
    if(s.phase!=Phase::Reward||(s.reward_claimed&4)||s.reward_potion==Potion::Empty)return false;
    if(procure_potion(s,s.reward_potion)){s.reward_claimed|=4;s.revision++;return true;}
    return false;
}
bool leave_rewards(State&s){if(s.phase!=Phase::Reward)return false;finish_room(s);return true;}
int rest_heal_amount(const State&s){return s.max_hp*3/10+((s.world.active?has_world_relic(s,WorldRelicRegalPillow):(s.relics&RegalPillow)!=0)?15:0);}
int remove_price(const State&s){return s.world.active?world_shop_price(s,ShopRemove):min(9999,75+s.removals*25);}
static bool has_removable(const State&s){for(unsigned i=0;i<s.deck_count;i++)if(card_removable(s.deck[i]))return true;return false;}
static unsigned transformable_count(const State&s){unsigned count=0;for(unsigned i=0;i<s.deck_count;i++)if(card_transformable(s.deck[i]))++count;return count;}
bool buy(State&s,int item){
    if(s.world.active)return world_buy(s,item);
    if(s.phase!=Phase::Shop||item<0||item>ShopRemove)return false;
    if(item==ShopRemove){
        if((s.shop_sold&(1u<<ShopRemove))||s.deck_count<=1||!has_removable(s)||s.gold<remove_price(s))return false;
        s.phase=Phase::Remove;s.revision++;return true;
    }
    if((s.shop_sold&(1u<<item))||s.gold<s.shop_prices[item])return false;
    if(item<ShopCardCount){
        if(s.deck_count>=MaxDeck)return false;
        notify_deck_replaced(s,s.deck_count);s.deck[s.deck_count++]={s.shop_cards[item],0};record(s,LogKind::Buy,s.shop_cards[item]);
    }else if(item<ShopPotionStart){
        int balance=s.gold;s.gold-=s.shop_prices[item];
        if(!gain_relic(s,s.shop_relics[item-ShopRelicStart])){s.gold=(int16_t)balance;return false;}
    }else{
        int slot=-1;for(unsigned i=0;i<potion_slot_count(s);i++)if(s.potions[i]==Potion::Empty){slot=i;break;}
        if(slot<0||!potion_definition((Potion)s.shop_potions[item-ShopPotionStart]))return false;
        s.potions[slot]=(Potion)s.shop_potions[item-ShopPotionStart];
    }
    if(item<ShopRelicStart||item>=ShopPotionStart)s.gold-=s.shop_prices[item];
    s.shop_sold|=(uint16_t)(1u<<item);s.revision++;return true;
}
bool remove_card(State&s,int index){
    if(s.world.active&&s.phase==Phase::Remove&&s.event_deck_action==EventDeckAction::None){
        if(index<0||index>=s.deck_count||s.deck_count<=1||!card_removable(s,s.deck[index],index)||(s.shop_sold&(1U<<ShopRemove)))return false;
        int price=s.world.shop_free_remove?0:remove_price(s);if(s.gold<price||!event_remove_deck_card(s,index))return false;
        s.gold-=int16_t(price);s.shop_sold|=1U<<ShopRemove;if(s.removals<255)++s.removals;s.phase=Phase::Shop;
        s.world.shop_free_remove=0;
        world_relic_hook(s,WorldRelicHook::AfterItemPurchased,ShopRemove,price);++s.revision;return true;
    }
    if(s.phase!=Phase::Remove||index<0||index>=s.deck_count||s.deck_count<=1||!card_removable(s.deck[index]))return false;
    const bool event=s.event_deck_action==EventDeckAction::Remove;
    if(!event&&s.gold<remove_price(s))return false;
    record(s,LogKind::Remove,s.deck[index].kind);notify_deck_removed(s,index);for(int i=index+1;i<s.deck_count;i++)s.deck[i-1]=s.deck[i];s.deck_count--;
    if(event){complete_event_selection(s);return true;}
    s.gold-=remove_price(s);s.shop_sold|=(1u<<ShopRemove);if(s.removals<255)s.removals++;s.phase=Phase::Shop;s.revision++;return true;
}
bool leave_shop(State&s){if(s.phase!=Phase::Shop)return false;finish_room(s);return true;}
bool has_upgrade(const State&s){for(int i=0;i<s.deck_count;i++)if(card_upgradeable(s.deck[i]))return true;return false;}
bool event_option_enabled(const State&s,int option){
    if(s.original_events.active)return original_event_option_enabled(s,option);
    if(s.phase!=Phase::Event||s.event_deck_action!=EventDeckAction::None||option<0||option>1)return false;
    if(s.event_page>=2)return option==0;
    if(s.event_id==EventId::Baths&&option==0)return s.max_hp<=997;
    if(s.event_id==EventId::Grove)return option==0?transformable_count(s)>=2:s.max_hp<=994;
    if(s.event_id==EventId::AromaOfChaos&&option==0)return transformable_count(s)>0;
    // The bounded core still requires at least one permanent card.
    if(s.event_id==EventId::DoorsOfLightAndDark&&option==1)return s.deck_count>1&&has_removable(s);
    return (unsigned)s.event_id<EventKinds;
}
void complete_event_selection(State&s){
    const auto action=s.event_deck_action;
    if(action==EventDeckAction::None)return;
    s.event_deck_action=EventDeckAction::None;s.event_first=-1;s.event_remaining=0;
    s.phase=Phase::Event;s.event_page=action==EventDeckAction::Transform?2:3;s.revision++;
}
bool cancel_event_selection(State&s){
    if(s.event_deck_action==EventDeckAction::None)return false;
    // Cancels only an uncommitted event selection, not Grove's paid two-card operation.
    s.event_deck_action=EventDeckAction::None;s.event_first=-1;s.event_remaining=0;
    s.phase=Phase::Event;s.event_page=0;s.revision++;return true;
}
bool event_choice(State&s,int option){
    if(s.original_events.active)return original_event_choose(s,option);
    if(!event_option_enabled(s,option))return false;
    if(s.event_page>=2){if(option!=0)return false;if(s.event_id==EventId::Vegetation&&s.event_page==4){s.encounter=1;enter_combat(s);}else finish_room(s);return true;}
    switch(s.event_id){
    case EventId::AromaOfChaos:
        if(option==0){s.event_deck_action=EventDeckAction::Transform;s.event_remaining=1;s.event_first=-1;s.phase=Phase::Transform;}
        else if(has_upgrade(s)){s.event_deck_action=EventDeckAction::Upgrade;s.phase=Phase::Upgrade;}
        else s.event_page=3; // No eligible cards: still finish the original narrative.
        break;
    case EventId::DoorsOfLightAndDark:
        if(option==0){
            uint16_t eligible[MaxDeck];unsigned n=0;
            for(unsigned i=0;i<s.deck_count;i++)if(card_upgradeable(s.deck[i]))eligible[n++]=(uint16_t)i;
            // Distinct eligible instances, shuffled before taking at most two.
            // This expedition's RNG is not the original event RNG stream.
            for(unsigned i=n;i>1;--i){unsigned j=random_below(s.rng_world,i);auto t=eligible[i-1];eligible[i-1]=eligible[j];eligible[j]=t;}
            for(unsigned i=0;i<n&&i<2;i++){auto&c=s.deck[eligible[i]];c.upgraded=1;record(s,LogKind::Upgrade,c.kind);}
            s.event_page=2;
        }else{s.event_deck_action=EventDeckAction::Remove;s.phase=Phase::Remove;}
        break;
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
    if(s.phase!=Phase::Transform||index<0||index>=s.deck_count||index==s.event_first||!s.event_remaining||!card_transformable(s.deck[index]))return false;
    int old=s.deck[index].kind,k;do{k=(int)random_reward_card(s);}while(k==old);
    notify_deck_replaced(s,index);s.deck[index]={(CardKind)k,0};record(s,LogKind::Transform,old,k);
    s.event_first=(int16_t)index;s.event_remaining--;
    if(!s.event_remaining){
        if(s.event_deck_action==EventDeckAction::Transform)complete_event_selection(s);
        else{s.phase=Phase::Event;s.event_page=2;}
    }
    s.revision++;return true;
}
bool open_treasure(State&s){
    if(s.world.active)return world_open_treasure(s);
    if(s.phase!=Phase::Treasure||s.treasure_state)return false;
    s.treasure_relic=random_relic(s);s.treasure_state=1;s.revision++;return true;
}
bool take_treasure(State&s){
    if(s.world.active)return world_take_treasure(s);
    if(s.phase!=Phase::Treasure||s.treasure_state!=1)return false;
    if(s.treasure_relic){if(!gain_relic(s,s.treasure_relic))return false;}
    else gain_gold(s,50);
    s.treasure_state=2;s.revision++;return true;
}
bool leave_treasure(State&s){
    if(s.phase!=Phase::Treasure)return false;
    if(s.world.active)return s.world.room_open&&s.world.room_type==uint8_t(WorldRoom::Treasure)&&world_finish_room(s);
    if(!s.treasure_state)return false;
    finish_room(s);return true;
}
#include "world_run.inc"
} // namespace spire
