#include "core.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static State s,loaded,original;
static SaveData encoded,roundtrip;
static unsigned checks=0;
static void require(bool okay,const char*message){++checks;if(!okay){std::fprintf(stderr,"world relics: %s\n",message);std::exit(1);}}
static WorldRelicInstance& relic(RelicId id){for(unsigned i=0;i<s.world_relics.count;i++)if(s.world_relics.entries[i].id==id)return s.world_relics.entries[i];std::abort();}
static void reset(){new_run(s,0x5a8317,false,Character::Ironclad);init_world_relics(s);s.phase=Phase::Map;s.world_relics.count=0;s.relic_count=0;s.relics=0;for(auto&r:s.world_relics.entries)r=WorldRelicInstance{};}
static void add(RelicId id){require(gain_world_relic(s,id),"obtain native relic");require(!world_relic_pending(s),"fixture relic has no unresolved acquisition");}
static void combat(){enter_combat(s);resolve_pending(s);require(s.phase==Phase::Combat,"fixture combat starts");for(unsigned i=0;i<s.enemy_count;i++)s.enemies[i].hp=s.enemies[i].max_hp=2000;}
static void hook(WorldRelicHook h,int a=0,int b=0,int c=0){world_relic_hook(s,h,a,b,c);resolve_pending(s);require(s.phase!=Phase::Error,"listener did not overflow or reject continuation");}
static void save_pending(){require(validate(s),"pending world relic state validates");require(save(s,encoded),"serialize pending world relic");require(load(loaded,encoded.bytes,encoded.size),"load pending world relic");require(save(loaded,roundtrip),"reserialize pending world relic");require(encoded.size==roundtrip.size&&!std::memcmp(encoded.bytes,roundtrip.bytes,encoded.size),"pending hook serials and counters are byte exact");s=loaded;}
static unsigned card_in_hand(CardType type){for(unsigned i=0;i<s.hand.count;i++)if(card_type(s,s.cards[s.hand.ids[i]],s.hand.ids[i])==type)return s.hand.ids[i];std::abort();}
int main(){
 reset();add(WorldRelicAnchor);add(WorldRelicVajra);combat();require(s.block==10&&s.strength==1,"native first-33 relics are not also executed by legacy hooks");
 reset();add(WorldRelicShuriken);add(WorldRelicRuinedHelmet);add(WorldRelicNunchaku);combat();unsigned attack=card_in_hand(CardType::Attack);
 for(unsigned i=0;i<3;i++)hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0);
 require(s.strength==2&&relic(WorldRelicRuinedHelmet).values[0]==1,"first positive Strength gain is doubled once");
 for(unsigned i=0;i<3;i++)hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0);
 require(s.strength==3,"later Strength gains keep their original amount");
 require(relic(WorldRelicNunchaku).values[0]==6,"attack counter advances for repeated CardPlay listeners");
 hook(WorldRelicHook::AfterCombatEnd);require(relic(WorldRelicNunchaku).values[0]==6&&relic(WorldRelicShuriken).values[0]==0&&relic(WorldRelicRuinedHelmet).values[0]==0,"combat cleanup preserves run counters and clears combat counters");
 reset();add(WorldRelicGalacticDust);combat();int block=s.block;hook(WorldRelicHook::AfterStarsSpent,25);require(s.block==block+20&&relic(WorldRelicGalacticDust).values[0]==5,"large star payments award every full threshold and retain remainder");
 reset();add(WorldRelicSelfFormingClay);add(WorldRelicCentennialPuzzle);combat();int initial=s.hand.count;
 hook(WorldRelicHook::AfterHpLost,3,4,-1);require(s.world_relics.self_forming_clay==3&&relic(WorldRelicCentennialPuzzle).values[0]==1,"HP loss creates delayed block and one puzzle draw");
 hook(WorldRelicHook::AfterHpLost,2,4,-1);require(s.world_relics.self_forming_clay==6&&s.hand.count>=initial,"further losses stack delayed block without rearming puzzle");
 reset();add(WorldRelicToastyMittens);add(WorldRelicBurningSticks);
 // Enter without opening AfterPlayerTurnStart, then dispatch the explicit hook.
 s.phase=Phase::Combat;s.turn=1;s.hp=s.max_hp=80;s.enemy_count=1;s.enemies[0].hp=s.enemies[0].max_hp=100;
 s.playing=-1;s.card_count=1;s.cards[0]={Defend,0};s.zones[0]=Zone::Hand;s.hand.count=1;s.hand.ids[0]=0;
 hook(WorldRelicHook::AfterHandDraw);require(s.phase==Phase::WorldRelicChoice&&s.world_relics.choice==WorldRelicChoice::Hand,"Toasty pauses its ordered hook at a real hand selection");save_pending();
 uint16_t chosen=0;require(world_relic_select(s,&chosen,1),"resolve Toasty selection");resolve_pending(s);
 require(s.strength==1&&s.exhaust.count==1&&s.hand.count==1&&s.cards[s.hand.ids[0]].kind==Defend,"Burning Sticks exhaust callback runs before Toasty's Strength continuation");
 for(const auto&f:s.world_relics.hook_frames)require(!f.active,"finished nested hook frame released");
 reset();add(WorldRelicToastyMittens);add(WorldRelicMrStruggles);
 s.phase=Phase::Combat;s.turn=1;s.enemy_count=1;s.enemies[0].hp=s.enemies[0].max_hp=100;s.playing=-1;s.card_count=1;s.cards[0]={Defend,0};s.zones[0]=Zone::Hand;s.hand.count=1;s.hand.ids[0]=0;
 hook(WorldRelicHook::AfterHandDraw);require(s.phase==Phase::WorldRelicChoice,"listener snapshot pauses");
 require(lose_world_relic(s,WorldRelicMrStruggles),"remove later listener while paused");require(gain_world_relic(s,WorldRelicMercuryHourglass),"append another listener while paused");
 require(world_relic_select(s,&chosen,1),"resume saved listener snapshot");resolve_pending(s);
 require(s.enemies[0].hp==100,"removed serial is skipped and newly acquired listener is excluded from old snapshot");
 reset();add(WorldRelicBeltBuckle);combat();require(s.dexterity==2,"empty potion inventory grants buckle Dexterity");
 require(procure_potion(s,Potion::Strength),"procure during combat");resolve_pending(s);require(s.dexterity==0,"procurement removes exactly buckle's Dexterity");
 require(discard_potion(s,0),"discard last potion");resolve_pending(s);require(s.dexterity==2,"discarding final potion restores buckle Dexterity");
 reset();add(WorldRelicBrilliantScarf);combat();attack=card_in_hand(CardType::Attack);
 for(unsigned i=0;i<4;i++)hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0);
 require(cost(s,int(attack))==0,"fifth manual play has zero energy cost");
 hook(WorldRelicHook::AfterCardPlayed,int(attack),1,0x10000);require(relic(WorldRelicBrilliantScarf).values[0]==4,"automatic CardPlay does not consume Scarf's manual-play counter");
 hook(WorldRelicHook::AfterCardPlayed,int(attack),0,0);require(cost(s,int(attack))>0,"Scarf discount ends after fifth manual play");
 original=s;s.world_relics.hook_frames[0].active=1;s.world_relics.hook_frames[0].count=1;s.world_relics.hook_frames[0].serials[0]=uint16_t(s.world_relics.last_serial+1);require(!valid_world_relic_state(s),"forged listener serial rejected");s=original;
 std::printf("world relic runtime checks: %u\n",checks);return 0;
}
