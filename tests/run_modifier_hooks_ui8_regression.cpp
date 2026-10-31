#include "core.hpp"
#include "original_events.hpp"
#include "run_options.hpp"
#include "world_card_factory.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static State game,other,loaded;
static SaveData bytes,again;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"run_modifier_hooks_ui8:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t bit(RunModifier m){return run_modifier_bit(m);}
static void persisted(){
    CHECK(validate(game)&&save(game,bytes));CHECK(load(loaded,bytes.bytes,bytes.size)&&save(loaded,again));
    CHECK(bytes.size==again.size&&!std::memcmp(bytes.bytes,again.bytes,bytes.size));game=loaded;
}
static void unchanged(const State&before){CHECK(save(game,bytes)&&save(before,again));CHECK(bytes.size==again.size&&!std::memcmp(bytes.bytes,again.bytes,bytes.size));}
static unsigned begin(uint32_t modifiers){
    new_run(game,0x7195c3,false,Character::Ironclad);const unsigned cards=game.deck_count;
    CHECK(configure_run_options(game,RunMode::Custom,modifiers));CHECK(world_set_seed(game,"NSPIRE-WORLD-017")&&world_begin_run(game));
    CHECK(game.original_events.finished&&world_can_travel(game));persisted();return cards;
}
// Supply outcomes only for intermediate rooms so a test can reach its real
// merchant/rest/unknown node. Target room construction and actions are real.
static void complete_routing_fixture(){
    game.original_events=OriginalEventState{};game.hp=game.max_hp;
    if(combat_phase(game.phase)||game.phase==Phase::OriginalEventChoice)game.phase=Phase::Reward;
    CHECK(world_finish_room(game));
}
static unsigned first(){uint8_t ids[reference::Columns]{};CHECK(world_available_points(game,ids,reference::Columns));CHECK(world_choose_point(game,ids[0]));return ids[0];}
static unsigned route(reference::PointType wanted,bool force_event=false,bool force_elite=false,State*before_target=nullptr){
    const auto&map=game.world.maps[game.world.act_index];uint8_t parents[reference::MaxPoints],queue[reference::MaxPoints];
    for(auto&v:parents)v=reference::NoPoint;parents[map.root]=map.root;queue[0]=map.root;unsigned head=0,count=1,target=reference::NoPoint;
    while(head<count){const unsigned p=queue[head++];if(map.points[p].type==wanted){target=p;break;}
        for(unsigned i=0;i<map.points[p].children.used;++i){const unsigned next=map.points[p].children.slots[i];if(next!=reference::NoPoint&&parents[next]==reference::NoPoint){parents[next]=uint8_t(p);queue[count++]=uint8_t(next);}}}
    CHECK(target!=reference::NoPoint);uint8_t path[reference::MaxRows+2];unsigned length=0;
    for(unsigned p=target;p!=map.root;p=parents[p])path[length++]=uint8_t(p);
    for(unsigned i=length;i>0;--i){if(!world_can_travel(game))complete_routing_fixture();
        if(i==1&&(force_event||force_elite)){game.world.unknown_monster_bits=game.world.unknown_shop_bits=game.world.unknown_treasure_bits=0;game.world.unknown_elite_bits=force_elite?0x3f800000u:0xbf800000u;}
        if(i==1&&before_target)*before_target=game;
        CHECK(world_choose_point(game,path[i-1]));}
    return target;
}
static void kill_combat(){
    CHECK(game.phase==Phase::Combat);
    for(unsigned guard=0;guard<64&&game.phase==Phase::Combat;++guard){for(unsigned i=0;i<game.enemy_count;++i)if(enemy_allow_target(game,int(i)))damage_unpowered(game,int(i),9999);resolve_pending(game);}
    CHECK(game.phase==Phase::WorldRelicChoice&&game.world_relics.choice==WorldRelicChoice::OfferReward);persisted();
}
static unsigned room_elites(){unsigned count=0;const auto&m=game.world.maps[game.world.act_index];for(unsigned i=0;i<m.count;++i)count+=m.points[i].active&&m.points[i].type==reference::PointType::Elite;return count;}
static void flight(){
    begin(bit(RunModifier::Flight));CHECK(gain_world_relic(game,WorldRelicWingedBoots));first();complete_routing_fixture();
    unsigned target=reference::NoPoint;const auto&m=game.world.maps[0];const unsigned current=game.world.current_point;
    for(unsigned i=0;i<m.count;++i)if(m.points[i].active&&m.points[i].row==m.points[current].row+1&&!m.points[current].children.contains(uint8_t(i))){target=i;break;}
    CHECK(target!=reference::NoPoint&&world_reachable(game,target));other=game;other.run_options.modifiers=0;
    // An exhausted Boots keeps its owned display without providing a second
    // source of free travel in the negative control.
    for(unsigned i=0;i<other.world_relics.count;++i)if(other.world_relics.entries[i].id==WorldRelicWingedBoots)other.world_relics.entries[i].values[0]=3;
    CHECK(!world_reachable(other,target));const int boots=world_relic_counter(game,WorldRelicWingedBoots);
    CHECK(world_choose_point(game,target)&&world_relic_counter(game,WorldRelicWingedBoots)==boots);persisted();
}
static void deadly_and_hunter(){
    begin(bit(RunModifier::DeadlyEvents));CHECK(game.world.unknown_elite_bits==0x3dcccccdU);
    for(unsigned owner=0;owner<2;++owner)for(unsigned rarity=0;rarity<4;++rarity)for(unsigned i=0;i<game.world_relics.bag_sizes[owner][rarity];++i)
        CHECK(game.world_relics.bags[owner][rarity][i]!=WorldRelicJuzuBracelet);
    const unsigned node=route(reference::PointType::Unknown,false,true);
    CHECK(game.world.room_type==unsigned(WorldRoom::Elite)&&game.world.visited_rooms[0][node]==unsigned(WorldRoom::Elite));
    CHECK(game.world.unknown_elite_bits==0x3dcccccdU&&game.world.unknown_treasure_bits==0x3d23d70aU);persisted();
    begin(bit(RunModifier::Flight));const unsigned initial=room_elites();
    begin(bit(RunModifier::Flight)|bit(RunModifier::BigGameHunter));unsigned expected=initial*5/2;if((initial&1)&&(expected&1))++expected;
    // ignore_elite_rules is a constructor-only override, intentionally absent
    // from a saved map. Verify the generated/restored rooms and quota instead.
    CHECK(game.world.maps[0].config.elites==expected&&room_elites()==expected&&room_elites()>initial);persisted();
    WorldCardOptions options;options.source=WorldCardSource::Encounter;options.odds=WorldCardOdds::EliteEncounter;options.flags=WorldNoUpgradeRoll|WorldIsCardReward|WorldIsFromCombat;
    Card cards[3]{};EventCardFacts facts[3]{};CHECK(world_create_reward(game,world_rng(game,WorldRng::Rewards),3,options,cards,facts));
    for(const Card card:cards)CHECK(card_rarity(card.kind)==4);persisted();
}
static void cursed(){
    const unsigned initial=begin(bit(RunModifier::CursedRun));CHECK(game.deck_count==initial+1&&world_card_pool(game.deck[initial].kind)==WorldCardPool::Curse);
    for(unsigned act=1;act<3;++act){
        for(unsigned guard=0;guard<24&&game.world.act_index<act;++guard){if(!world_can_travel(game))complete_routing_fixture();if(game.world.act_index<act)first();}
        CHECK(game.world.act_index==act&&game.deck_count==initial+act+1&&world_card_pool(game.deck[game.deck_count-1].kind)==WorldCardPool::Curse);persisted();
    }
}
static void murderous_terminal(){
    begin(bit(RunModifier::Murderous));first();CHECK(game.strength==3);
    for(unsigned i=0;i<game.enemy_count;++i)CHECK(enemy_power_value(game,int(i),EnemyPower::Strength)==3);
    // The spawn effect is the same public dispatcher used by the original
    // enemy moves, and must apply the modifier once to the new identity.
    const unsigned old=game.enemy_count;queue_extension(game,XOp::EnemySpawn,int(EnemyId::Wriggler),7);resolve_pending(game);
    CHECK(game.enemy_count==old+1&&enemy_power_value(game,int(old),EnemyPower::Strength)==3&&game.strength==3);persisted();
    begin(bit(RunModifier::Terminal));CHECK(game.max_hp==79&&game.hp==79);first();CHECK(game.max_hp==78&&power_value(game,XPower::Plating)==5);persisted();
    begin(bit(RunModifier::Terminal));route(reference::PointType::Unknown,true);CHECK(game.world.room_type==unsigned(WorldRoom::Event));const int maximum=game.max_hp;
    game.original_events=OriginalEventState{};game.phase=Phase::Event;CHECK(original_event_begin(game,OriginalEventId::BattlewornDummy,world_run_seed(game),0));
    CHECK(world_enter_event_combat(game,"BattlewornDummyEventV1Encounter",true));
    CHECK(game.world.event_combat_parent&&game.max_hp==maximum&&power_value(game,XPower::Plating)==5);persisted();
}
struct HealObservation {int hp=0,maximum=0,healed=0;};
static void observe_heal(void*context,const State&s,Log log){if(log.kind==LogKind::Heal){auto&v=*static_cast<HealObservation*>(context);v.hp=s.hp;v.maximum=s.max_hp;v.healed=log.a;}}
static void rest_modifiers(){
    begin(bit(RunModifier::Midas));route(reference::PointType::Rest);CHECK(game.phase==Phase::Camp&&world_rest_option_count(game)==1&&!std::strcmp(world_rest_option_key(game,0),"HEAL"));persisted();
    begin(bit(RunModifier::NightTerrors));route(reference::PointType::Rest);CHECK(gain_world_relic(game,WorldRelicDreamCatcher));game.hp=10;const int old=game.max_hp;
    HealObservation heal{};const auto observer=set_core_observer({&heal,observe_heal});CHECK(rest_heal_amount(game)==old&&world_rest_choose(game,0));set_core_observer(observer);
    CHECK(heal.hp==old&&heal.maximum==old&&heal.healed==old-10);CHECK(game.max_hp==old-5&&game.hp==old-5&&world_relic_pending(game));persisted();
    CHECK(world_relic_select(game,nullptr,0));CHECK(game.max_hp==old-5&&game.hp==old-5&&world_rest_can_proceed(game));persisted();
    begin(bit(RunModifier::NightTerrors));route(reference::PointType::Rest);game.max_hp=3;game.hp=1;CHECK(world_rest_choose(game,0));CHECK(game.max_hp==1&&game.hp==1);persisted();
}
static void hoarder(){
    begin(bit(RunModifier::Hoarder));CHECK(gain_world_relic(game,WorldRelicBingBong)&&gain_world_relic(game,WorldRelicBingBong)&&gain_world_relic(game,WorldRelicLuckyFysh));
    EventCardFacts facts{};facts.enchantment=EventEnchantment::Nimble;facts.enchantment_amount=3;EventDeckSnapshot card{{DefectGeneticAlgorithm,1},facts,37,0};
    const unsigned deck=game.deck_count;const int gold=game.gold,index=event_restore_deck_card(game,card);CHECK(index==int(deck));event_deck_acquired(game,index);
    CHECK(game.deck_count==deck+5&&game.gold==gold+75);
    for(unsigned i=deck;i<game.deck_count;++i)CHECK(game.deck[i].kind==DefectGeneticAlgorithm&&game.deck[i].upgraded&&game.defect.genetic_deck[i]==37&&game.event_cards.deck[i].enchantment==EventEnchantment::Nimble);
    CHECK(event_add_deck_card(game,{Strike,0},nullptr,true)>=0&&game.deck_count==deck+6&&game.gold==gold+90);persisted();
    begin(bit(RunModifier::Hoarder));route(reference::PointType::Shop);CHECK(game.phase==Phase::Shop&&(game.shop_sold&(1u<<ShopRemove)));game.gold=9999;other=game;
    CHECK(!buy(game,ShopRemove));unchanged(other);const unsigned before=game.deck_count;const Card offered=game.world.shop_cards[0];const int price=world_shop_price(game,0);
    CHECK(buy(game,0)&&game.deck_count==before+3&&game.gold==9999-price);for(unsigned i=before;i<game.deck_count;++i)CHECK(game.deck[i].kind==offered.kind);persisted();
}
static void reward_modifiers(){
    begin(bit(RunModifier::Flight));first();queue_extension(game,XOp::RPower,int(RPower::Royalties),9);resolve_pending(game);kill_combat();
    unsigned gold_count=0;uint16_t gold[MaxWorldRelicRewards]{};for(const auto&r:game.world_relics.rewards)if(r.group&&r.kind==4)gold[gold_count++]=r.value;CHECK(gold_count>=2);
    begin(bit(RunModifier::Flight)|bit(RunModifier::Midas));first();queue_extension(game,XOp::RPower,int(RPower::Royalties),9);resolve_pending(game);kill_combat();
    unsigned index=0;for(const auto&r:game.world_relics.rewards)if(r.group&&r.kind==4){CHECK(index<gold_count&&r.value==gold[index]*2);++index;}CHECK(index==gold_count);
    const int before=game.gold;gain_gold(game,11);CHECK(game.gold==before+11);persisted();
    begin(bit(RunModifier::Vintage));CHECK(gain_world_relic(game,WorldRelicLastingCandy));first();encounter_add_card_reward(game);encounter_add_card_reward(game);kill_combat();
    unsigned relics=0;for(const auto&r:game.world_relics.rewards)if(r.group){CHECK(r.kind!=1);if(r.kind==2){CHECK(r.value<WorldRelicKinds);++relics;}}
    CHECK(relics==3&&world_relic_counter(game,WorldRelicLastingCandy)==0);persisted();
}
static void character_cards(){
    begin(bit(RunModifier::SilentCards)|bit(RunModifier::DefectCards));WorldCardOptions options;options.odds=WorldCardOdds::Uniform;options.flags=WorldNoUpgradeRoll|WorldIsCardReward;
    Card cards[30]{};EventCardFacts facts[30]{};CHECK(world_create_reward(game,world_rng(game,WorldRng::Rewards),30,options,cards,facts));unsigned seen=0;
    for(const Card card:cards){const unsigned pool=unsigned(world_card_pool(card.kind));CHECK(pool<=unsigned(WorldCardPool::Defect));seen|=1u<<pool;}CHECK(seen==7);
    options.flags|=WorldNoCardPoolModifications;CHECK(world_create_reward(game,world_rng(game,WorldRng::Rewards),30,options,cards,facts));for(const Card card:cards)CHECK(world_card_pool(card.kind)==WorldCardPool::Ironclad);
    options={};world_modifier_card_pools(game,options,true);CHECK(options.pool_count==2&&options.pools[1]==WorldCardPool::Silent);
    route(reference::PointType::Shop);for(unsigned i=0;i<5;++i)CHECK(world_card_pool(game.world.shop_cards[i].kind)==WorldCardPool::Ironclad||world_card_pool(game.world.shop_cards[i].kind)==WorldCardPool::Silent);
    for(unsigned i=5;i<ShopCardCount;++i)CHECK(world_card_pool(game.world.shop_cards[i].kind)==WorldCardPool::Colorless);persisted();
}
// Independent direct translation of MerchantCardEntry.Except followed by
// CharacterCards.Concat. This oracle preserves sequence/multiplicity; it does
// not call the runtime merchant pool builder under test.
static unsigned source_merchant_candidates(const State&s,WorldCardPool initial,const CardKind*blacklist,unsigned blacklisted,CardKind*out){
    CardKind pool[CardKinds];const unsigned count=world_card_pool_models(initial,pool,CardKinds);unsigned n=0;
    for(unsigned i=0;i<count;++i){bool excluded=false;for(unsigned j=0;j<blacklisted;++j)if(pool[i]==blacklist[j])excluded=true;if(!excluded)out[n++]=pool[i];}
    for(unsigned i=0;i<run_modifier_count(s.run_options);++i){const auto modifier=run_modifier_at(s.run_options,i);WorldCardPool added;
        switch(modifier){case RunModifier::IroncladCards:added=WorldCardPool::Ironclad;break;case RunModifier::SilentCards:added=WorldCardPool::Silent;break;
        case RunModifier::DefectCards:added=WorldCardPool::Defect;break;case RunModifier::RegentCards:added=WorldCardPool::Regent;break;case RunModifier::NecrobinderCards:added=WorldCardPool::Necrobinder;break;default:continue;}
        bool foreign=false;for(unsigned j=0;j<n;++j)if(unsigned(world_card_pool(out[j]))!=unsigned(s.character))foreign=true;
        if(!foreign){const unsigned extra=world_card_pool_models(added,pool,CardKinds);for(unsigned j=0;j<extra;++j){CHECK(n<CardKinds);out[n++]=pool[j];}}
    }
    return n;
}
static CardKind source_merchant_roll(const State&s,unsigned slot,unsigned rarity,const CardKind*blacklist,unsigned blacklisted,reference::Random&rng){
    CardKind candidates[CardKinds];unsigned n=source_merchant_candidates(s,slot<5?WorldCardPool(s.character):WorldCardPool::Colorless,blacklist,blacklisted,candidates),count=0;
    const CardType types[]={CardType::Attack,CardType::Attack,CardType::Skill,CardType::Skill,CardType::Power};
    for(unsigned i=0;i<n;++i)if(unsigned(card_rarity(candidates[i]))==rarity&&(slot>=5||definitions[candidates[i]].type==types[slot]))candidates[count++]=candidates[i];
    CHECK(count);return candidates[rng.below(count)];
}
static void character_card_merchant_multiplicity(){
    begin(bit(RunModifier::IroncladCards));WorldCardOptions options;options.rarity_mask=world_rarity_mask(2)|world_rarity_mask(3)|world_rarity_mask(4);
    CardKind own[CardKinds],weighted[CardKinds];const unsigned n=world_card_candidates(game,options,own,CardKinds);CHECK(n>10);
    CHECK(world_merchant_card_candidates(game,options,nullptr,0,weighted,CardKinds)==n*2);
    CHECK(!std::memcmp(weighted,own,n*sizeof(CardKind))&&!std::memcmp(weighted+n,own,n*sizeof(CardKind)));
    CHECK(world_merchant_card_candidates(game,options,own,1,weighted,CardKinds)==n*2-1);
    unsigned occurrence=0;for(unsigned i=0;i<n*2-1;++i)occurrence+=weighted[i]==own[0];CHECK(occurrence==1);
    WorldCardOptions reward=options;world_modifier_card_pools(game,reward);CHECK(reward.pool_count==1); // Reward uses Union, not merchant Concat.
    other=game;other.run_options.modifiers=bit(RunModifier::Flight);Card cards[3],control[3];EventCardFacts facts[3],control_facts[3];reward.flags=WorldIsCardReward|WorldNoUpgradeRoll;
    CHECK(world_create_reward(game,world_rng(game,WorldRng::Rewards),3,reward,cards,facts)&&world_create_reward(other,world_rng(other,WorldRng::Rewards),3,reward,control,control_facts));
    // Card has trailing padding. Independently generated cards must match
    // their complete fields, not indeterminate padding left by assignment.
    for(unsigned i=0;i<3;++i)CHECK(cards[i].kind==control[i].kind&&cards[i].upgraded==control[i].upgraded);
    CHECK(world_rng(game,WorldRng::Rewards).calls==world_rng(other,WorldRng::Rewards).calls);
    begin(bit(RunModifier::IroncladCards)|bit(RunModifier::SilentCards)|bit(RunModifier::DefectCards));
    WorldCardOptions merchant;world_modifier_card_pools(game,merchant,true);CHECK(merchant.pool_count==3&&merchant.pools[1]==WorldCardPool::Ironclad&&merchant.pools[2]==WorldCardPool::Silent);
    route(reference::PointType::Shop,false,false,&other);CHECK(game.phase==Phase::Shop);
    auto shops=world_rng(other,WorldRng::Shops);const unsigned sale=shops.below(5);CHECK(game.world.shop_sale==sale);CardKind inventory[ShopCardCount];unsigned stocked=0;
    for(unsigned slot=0;slot<ShopCardCount;++slot){
        const Card actual=game.world.shop_cards[slot];const CardKind expected=source_merchant_roll(other,slot,unsigned(card_rarity(actual.kind)),inventory,stocked,shops);
        CHECK(actual.kind==expected);inventory[stocked++]=expected;shops.next();if(slot==sale)shops.next();
    }
    persisted();CHECK(gain_world_relic(game,WorldRelicTheCourier));game.gold=9999;other=game;shops=world_rng(other,WorldRng::Shops);
    CHECK(buy(game,0)&&!(game.shop_sold&1));stocked=0;
    for(unsigned slot=0;slot<ShopCardCount;++slot)if(!(other.shop_sold&(1u<<slot)))inventory[stocked++]=other.world.shop_cards[slot].kind;
    const CardKind replacement=source_merchant_roll(other,0,unsigned(card_rarity(game.world.shop_cards[0].kind)),inventory,stocked,shops);
    CHECK(game.world.shop_cards[0].kind==replacement);shops.next();CHECK(world_rng(game,WorldRng::Shops).calls==shops.calls);
    CHECK(!std::memcmp(world_rng(game,WorldRng::Shops).words,shops.words,sizeof shops.words));persisted();
}
int main(){
    flight();deadly_and_hunter();cursed();murderous_terminal();rest_modifiers();hoarder();reward_modifiers();character_cards();character_card_merchant_multiplicity();
    std::printf("Original map, room, rewards, clones, character pools and modifier hook order: %u checks\n",checks);
}
