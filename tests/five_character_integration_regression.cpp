#include "core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static SaveData snapshot(const State&s){SaveData out{};CHECK(validate(s)&&save(s,out));return out;}
static void same(const State&s,const SaveData&before){auto after=snapshot(s);CHECK(after.size==before.size&&!std::memcmp(after.bytes,before.bytes,before.size));}
static void roundtrip(const State&s){auto b=snapshot(s);State copy{};CHECK(load(copy,b.bytes,b.size));same(copy,b);}
static uint32_t word(const std::vector<uint8_t>&b,size_t at){CHECK(at+4<=b.size());return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
static void write_word(std::vector<uint8_t>&b,size_t at,uint32_t value){CHECK(at+4<=b.size());for(int i=0;i<4;i++)b[at+i]=(uint8_t)(value>>(8*i));}
static void rehash(std::vector<uint8_t>&b){uint32_t h=2166136261u;for(size_t i=0;i<b.size()-4;i++){h^=b[i];h*=16777619u;}write_word(b,b.size()-4,h);}
static void genuine_v13(const char*path){
 // This file was emitted by the unchanged 7f486bdf writer. Its checksum,
 // provenance and private execution journals are recorded beside the fixture.
 FILE*f=std::fopen(path,"r");CHECK(f);std::vector<uint8_t>b;unsigned value;
 while(std::fscanf(f,"%2x",&value)==1)b.push_back((uint8_t)value);CHECK(!std::fclose(f));
 CHECK(b.size()==41820&&word(b,4)==13);State s{};CHECK(load(s,b.data(),b.size()));
 CHECK(s.phase==Phase::ReturnChoice&&s.playing==13&&s.expansion.depth==1&&s.expansion.frames[0].playing==14);
 CHECK(s.cards[13].kind==Headbutt&&s.cards[13].upgraded&&s.cards[14].kind==Havoc);
 CHECK(s.expansion.after_play_order[0]==(unsigned)PostPlayActor::Afterimage&&s.expansion.after_play_order[1]==(unsigned)PostPlayActor::SerpentForm);
 CHECK(s.silent.nightmare_active[0]&&s.silent.nightmares[0].card.kind==UpMySleeve&&s.silent.nightmares[0].discount==2&&s.silent.nightmares[0].sly==1&&s.silent.nightmares[0].retain==1);
 CHECK(!s.defect.power_count&&!s.regent.power_count&&!s.necro.power_count&&!s.silent.nightmares[0].necro_ethereal);
 auto migrated=snapshot(s);CHECK(migrated.bytes[4]==15&&migrated.size>b.size());
 // Every v13 payload byte stays at the same offset, including the old 22
 // listener fields and the original smaller Nightmare snapshot records.
 CHECK(!std::memcmp(migrated.bytes+8,b.data()+8,b.size()-12));
 // A dead opponent is an observable migration baseline. It must not become
 // a fresh death for a Necro card generated only after loading an old save.
 CHECK(word(b,364)==42);auto old_dead=b;write_word(old_dead,364,0);rehash(old_dead);
 State dead_copy{};CHECK(load(dead_copy,old_dead.data(),old_dead.size()));
 CHECK(dead_copy.necro.enemy_dead_mask==1&&!dead_copy.necro.creature_deaths);roundtrip(dead_copy);
 // Rechecksummed v13 variant: the old Silent directory records Envenom,
 // while the old shared AfterCardPlayed directory correctly has no entry.
 // The new reader must reject forged new actors before registering this
 // legitimate historical power in the expanded directory.
 CHECK(word(b,23488)==0&&word(b,23556)==(unsigned)SPower::Afterimage&&word(b,23560)==(unsigned)SPower::SerpentForm&&word(b,23564)==SPowerCount&&word(b,23568)==0&&word(b,23708)==3);
 auto old_envenom=b;write_word(old_envenom,23488,1);write_word(old_envenom,23568,(unsigned)SPower::Envenom);write_word(old_envenom,23708,4);rehash(old_envenom);
 State envenom{};CHECK(load(envenom,old_envenom.data(),old_envenom.size()));
 CHECK(envenom.silent.powers[(unsigned)SPower::Envenom]==1&&envenom.expansion.after_play_order[2]==(unsigned)PostPlayActor::SilentEnvenom);roundtrip(envenom);
 auto missing_actor=envenom;missing_actor.expansion.after_play_order[2]=0;CHECK(!validate(missing_actor));
 // Derive a valid old BulletTime state for the existing discounted card and
 // its Nightmare snapshot. Preserve the known Silent boundary while moving
 // both representations to the original until-play expiry.
 CHECK(word(b,23808)==0&&word(b,23812)==2&&word(b,23824)==0&&word(b,40488)==0&&word(b,40492)==2&&word(b,40504)==0);
 auto old_bullet=b;write_word(old_bullet,23808,1);write_word(old_bullet,23824,1);write_word(old_bullet,40488,1);write_word(old_bullet,40504,1);rehash(old_bullet);
 State bullet{};CHECK(load(bullet,old_bullet.data(),old_bullet.size()));
 CHECK(bullet.expansion.turn_cost[2]==1&&bullet.expansion.star_turn_free[2]&&!bullet.silent.bullet_free[2]&&!bullet.silent.bullet_origin[2]&&bullet.silent.turn_origin[2]==1);
 const auto&night=bullet.silent.nightmares[0];CHECK(night.turn_cost==1&&night.star_turn_free&&night.turn_origin==1&&!night.bullet_free&&!night.bullet_origin);roundtrip(bullet);
 bullet.necro.cost_delta[2]=2;CHECK(cost(bullet,2)==1);roundtrip(bullet);
 State resumed=s,copy{};CHECK(load(copy,migrated.bytes,migrated.size));
 CHECK(choose_return_to_draw(resumed,4)&&choose_return_to_draw(copy,4));
 CHECK(resumed.phase==Phase::Combat&&resumed.playing==-1&&!resumed.effect_count&&!resumed.expansion.depth);
 CHECK(resumed.hp==70&&resumed.energy==22&&resumed.block==4&&resumed.enemies[0].hp==42&&resumed.enemies[1].hp==26);
 CHECK(resumed.hand.count==1&&resumed.draw.count==9&&resumed.discard.count==1&&resumed.exhaust.count==2);
 CHECK(resumed.expansion.cards_finished==5&&resumed.silent.attacks_finished==1&&resumed.silent.skills_finished==2&&resumed.silent.nightmare_active[0]);
 same(copy,snapshot(resumed));roundtrip(resumed);
 // These exact offsets belong to this fixed fixture and are independently
 // checked against semantic fields before constructing rechecksummed forgeries.
 CHECK(word(b,92)==DefendSilent&&word(b,416)==(unsigned)EffectKind::Finish&&word(b,1528)==(unsigned)Character::Silent&&word(b,1708)==0);
 auto protected_state=resumed;auto before=snapshot(protected_state);
 auto reject=[&](std::vector<uint8_t>bad){rehash(bad);CHECK(!load(protected_state,bad.data(),bad.size()));same(protected_state,before);};
 auto bad=b;write_word(bad,92,NecroAfterlife);reject(bad); // New model fits uint16, but cannot be from v13.
 bad=b;write_word(bad,1528,(unsigned)Character::Defect);reject(bad);
 bad=b;write_word(bad,416,(unsigned)EffectKind::Extended);write_word(bad,424,(unsigned)XOp::AllCharactersBefore);reject(bad);
 bad=b;write_word(bad,416,(unsigned)EffectKind::Extended);write_word(bad,420,1);write_word(bad,424,(unsigned)XOp::Create);write_word(bad,1708,card_token({NecroAfterlife,0}));reject(bad);
 bad=b;write_word(bad,444,1);write_word(bad,448,31);write_word(bad,556,(uint32_t)DivineRight);reject(bad);
 bad=b;CHECK(word(bad,41736)==0);write_word(bad,41736,(unsigned)PostPlayActor::SilentEnvenom);reject(bad);
 bad=b;write_word(bad,23808,2);reject(bad);
 bad=b;write_word(bad,23808,1);write_word(bad,23824,3);reject(bad);
 bad=b;bad[0]^=1;CHECK(!load(protected_state,bad.data(),bad.size()));same(protected_state,before);
 CHECK(!load(protected_state,b.data(),b.size()-4));same(protected_state,before);
 std::printf("genuine v13 migration: %zu -> %zu bytes, paused and resumed payload preserved\n",b.size(),migrated.size);
}
static State bounded(){
 State s{};new_run(s,0x41cef1ea,false,Character::Necrobinder);s.phase=Phase::ExtendedChoice;s.floor=s.turn=1;s.enemy_count=2;s.current_node=0;
 s.relics=s.relic_count=s.relic_flash=0;for(const auto&r:relic_definitions)CHECK(gain_relic(s,r.id));
 s.deck_count=MaxDeck;s.card_count=MaxCards;
 for(unsigned i=0;i<s.deck_count;i++)s.deck[i]={(CardKind)(i%CardKinds),0};
 for(auto&e:s.enemies)e={9999,9999,0,0,0,2};
 s.playing=MaxCards-1;s.cards[s.playing]={SecretWeapon,0};s.zones[s.playing]=Zone::Limbo;
 auto&x=s.expansion;x.depth=MaxAutoFrames;x.choice=XChoice::Draw;x.choice_min=x.choice_max=1;
 constexpr unsigned available=MaxCards-MaxAutoFrames-1;
 for(unsigned i=0;i<available;i++){
  CardKind k=(CardKind)(i%CardKinds);s.cards[i]={k,(uint8_t)(i%2&&card_upgradeable({k,0}))};s.zones[i]=Zone::Draw;
  s.draw.ids[s.draw.count++]=(uint16_t)i;x.choices.ids[x.choices.count++]=(uint16_t)i;
  x.star_turn_free[i]=(uint8_t)(i%2);x.star_combat_free[i]=(uint8_t)(i%3==0);
 }
 for(unsigned i=0;i<MaxAutoFrames;i++){unsigned id=available+i;s.cards[id]={Havoc,0};s.zones[id]=Zone::Limbo;x.frames[i].playing=(int16_t)id;}
 s.effect_count=MaxEffects;for(auto&e:s.effects)e={EffectKind::Extended,0,(int16_t)XOp::Noop,0};
 s.log_count=24;for(auto&l:s.logs)l={LogKind::Turn,1,0};
 for(unsigned slot=0;slot<MaxNightmares;slot++){
  auto&n=s.silent.nightmares[slot];s.silent.nightmare_active[slot]=1;s.silent.power_order[slot]=(uint8_t)(SPowerCount+slot);++s.silent.power_count;
  n.necro_ethereal=1;n.star_turn_free=(uint8_t)(slot%2);n.star_combat_free=(uint8_t)(slot%3==0);
  if(slot%4==0){n.card={DefectModded,0};n.defect_cost_delta=2;n.defect_turn_origin=1;}
  else if(slot%4==1){n.card={RegentKinglyKick,1};n.regent_kick_discount=5;n.regent_kick_turn_origin=3;n.regent_kick_combat_origin=2;}
  else if(slot%4==2){n.card={NecroTheScythe,1};n.necro_scythe_value=17;n.necro_cost_delta=2;}
  else{n.card={RegentKinglyPunch,0};n.regent_punch_damage=31;}
 }
 return s;
}
static void capacity_and_ownership(){
 auto s=bounded();auto b=snapshot(s);roundtrip(s);
 CHECK((s.relics&BoundPhylactery)&&(s.relics&DivineRight));
 CHECK(b.size+4*(MaxAutoFrames+1)<=SaveCapacity);
 std::printf("v15 bounded save: %zu bytes, conservative ceiling %zu / %u\n",b.size,b.size+4*(MaxAutoFrames+1),SaveCapacity);
 auto bad=s;bad.expansion.depth=MaxAutoFrames+1;CHECK(!validate(bad));
 bad=s;bad.relics|=RelicMask(1)<<40;CHECK(!validate(bad));
 bad=s;bad.silent.nightmares[0].necro_flatten_zero=1;CHECK(!validate(bad));
 bad=s;bad.silent.nightmares[1].regent_kick_turn_origin=6;CHECK(!validate(bad));
 bad=s;bad.silent.nightmares[2].necro_scythe_value=9987;CHECK(!validate(bad));
 bad=s;bad.expansion.star_turn_free[0]=2;CHECK(!validate(bad));
 bad=s;bad.silent.nightmares[0].star_combat_free=2;CHECK(!validate(bad));
 // Returning an active card changes its pile owner without removing its
 // frame. A second frame claiming that same instance remains invalid.
 int current=s.playing;s.hand.ids[s.hand.count++]=(uint16_t)current;s.zones[current]=Zone::Hand;roundtrip(s);
 bad=s;bad.expansion.frames[0].playing=(int16_t)current;CHECK(!validate(bad));
 s.effects[0]={EffectKind::Extended,(int16_t)current,(int16_t)XOp::AutoInstance,0};roundtrip(s);
 // A planned follow-up can also reference an active card still in Limbo.
 --s.hand.count;s.zones[current]=Zone::Limbo;roundtrip(s);
 bad=s;bad.effects[0]={EffectKind::Damage,1,0,3};CHECK(!validate(bad));
 s.effects[0]={EffectKind::Damage,1,0,2};roundtrip(s);
}
static void eternal_deck_choices(){
 State s{};new_run(s,73,false,Character::Necrobinder);s.deck_count=2;
 s.deck[0]={NecroForbiddenGrimoire,0};s.deck[1]={NecroForbiddenGrimoire,1};s.gold=123;
 s.phase=Phase::Event;s.current_node=0;s.floor=1;s.map[0].room=Room::Event;
 for(EventId event:{EventId::Grove,EventId::AromaOfChaos,EventId::DoorsOfLightAndDark}){
  s.event_id=event;s.map[0].event=(uint8_t)event;const int option=event==EventId::DoorsOfLightAndDark?1:0;
  auto before=snapshot(s);CHECK(!event_option_enabled(s,option)&&!event_choice(s,option));same(s,before);
 }
 s.phase=Phase::Shop;auto before=snapshot(s);CHECK(!buy(s,ShopRemove));same(s,before);
 s.phase=Phase::Event;s.event_id=EventId::Grove;s.map[0].event=(uint8_t)EventId::Grove;
 s.deck[0]={NecroStrikeNecrobinder,0};CHECK(!event_option_enabled(s,0));
 s.deck[1]={NecroDefendNecrobinder,0};CHECK(event_option_enabled(s,0)&&event_choice(s,0));
 CHECK(s.gold==0&&s.phase==Phase::Transform&&s.event_remaining==2);
 CHECK(transform_card(s,0)&&transform_card(s,1));CHECK(s.phase==Phase::Event&&s.event_page==2);roundtrip(s);
}
int main(int argc,char**argv){CHECK(argc==2);genuine_v13(argv[1]);capacity_and_ownership();eternal_deck_choices();std::printf("five-character integration: %u checks passed\n",checks);}
