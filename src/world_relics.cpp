#include "core.hpp"
#include "world_card_factory.hpp"
#include "original_events.hpp"

namespace spire {
#include "world_relic_definitions.inc"
namespace {
#include "world_relic_pools.inc"
bool w_equal(const char* a,const char* b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
int w_cmp(const char* a,const char* b){while(*a&&*a==*b){++a;++b;}return (unsigned char)*a-(unsigned char)*b;}
template<class T,unsigned N> constexpr unsigned w_count(const T(&)[N]){return N;}
int w_min(int a,int b){return a<b?a:b;}
int w_max(int a,int b){return a>b?a:b;}
void w_error(State& s){s.phase=Phase::Error;record(s,LogKind::Capacity,WorldRelicCapacity);}
bool w_present(const WorldRelicInstance& r){return !(r.flags&WorldRelicMelted);}
int w_first(const State& s,RelicId id,bool active=false){
 for(unsigned i=0;i<s.world_relics.count;i++)if(s.world_relics.entries[i].id==id&&(!active||w_present(s.world_relics.entries[i])))return int(i);
 return -1;
}
bool w_before_treasure(RelicId id){switch(id){
 case WorldRelicAmethystAubergine:case WorldRelicBookOfFiveRings:case WorldRelicBowlerHat:
 case WorldRelicDragonFruit:case WorldRelicFrozenEgg:case WorldRelicGirya:case WorldRelicJuzuBracelet:
 case WorldRelicLastingCandy:case WorldRelicLuckyFysh:case WorldRelicMealTicket:case WorldRelicMoltenEgg:
 case WorldRelicOldCoin:case WorldRelicPlanisphere:case WorldRelicShovel:case WorldRelicToxicEgg:
 case WorldRelicWhiteBeastStatue:case WorldRelicWhiteStar:return true;
 default:return false;}}
bool w_allowed(const State& s,RelicId id,bool shop){
 return id<WorldRelicKinds&&(!w_before_treasure(id)||world_total_floor(s)<41)
  &&(!shop||!(world_relic_definitions[id].flags&2));
}
bool w_used(const WorldRelicInstance& r){switch(r.id){
 case WorldRelicBoneTea:case WorldRelicEmberTea:case WorldRelicTeaOfDiscourtesy:return r.values[0]<=0;
 case WorldRelicWingedBoots:return r.values[0]>=3;
 case WorldRelicToyBox:return r.values[0]>=15;
 case WorldRelicSilkenTress:case WorldRelicLizardTail:case WorldRelicMawBank:return r.values[0]!=0;
 case WorldRelicSilverCrucible:return r.values[0]>=3&&r.values[1];
 case WorldRelicWongosMysteryTicket:return r.values[1]!=0;
 default:return false;}}
const RelicId* w_character_pool(Character c,unsigned& n){n=8;switch(c){
 case Character::Ironclad:return WIroncladRelicPool;case Character::Silent:return WSilentRelicPool;
 case Character::Defect:return WDefectRelicPool;case Character::Regent:return WRegentRelicPool;
 case Character::Necrobinder:return WNecrobinderRelicPool;}n=0;return nullptr;}
void w_bag_remove(State& s,RelicId id,int bag=-1){auto& w=s.world_relics;
 for(unsigned b=0;b<2;b++)if(bag<0||unsigned(bag)==b)for(unsigned r=0;r<4;r++){
  unsigned out=0;for(unsigned i=0;i<w.bag_sizes[b][r];i++)if(w.bags[b][r][i]!=id)w.bags[b][r][out++]=w.bags[b][r][i];
  for(unsigned i=out;i<w.bag_sizes[b][r];i++)w.bags[b][r][i]=0;w.bag_sizes[b][r]=uint16_t(out);
 }
}
void w_bags(State& s){auto& w=s.world_relics;if(w.bags_ready)return;
 for(unsigned b=0;b<2;b++){
  unsigned order[4]{},used=0,cpn=0;const RelicId* cp=w_character_pool(s.character,cpn);
  unsigned total=w_count(WSharedRelicPool)+(b?cpn:0);
  for(unsigned i=0;i<total;i++){
   RelicId id=i<w_count(WSharedRelicPool)?WSharedRelicPool[i]:cp[i-w_count(WSharedRelicPool)];
   unsigned rarity=world_relic_definitions[id].rarity;if(rarity<2||rarity>5)continue;
   unsigned r=rarity-2;auto& n=w.bag_sizes[b][r];if(!n)order[used++]=r;
   if(n>=WorldRelicBagSize){w_error(s);return;}w.bags[b][r][n++]=id;
  }
  for(unsigned i=0;i<used;i++){unsigned r=order[i];reference::shuffle(w.bags[b][r],w.bag_sizes[b][r],world_rng(s,WorldRng::UpFront));}
 }
 w.bags_ready=1;for(unsigned i=0;i<w.count;i++)if(!(world_relic_definitions[w.entries[i].id].flags&8))w_bag_remove(s,w.entries[i].id);
}
void w_clean_bags(State& s){auto& w=s.world_relics;for(unsigned b=0;b<2;b++)for(unsigned r=0;r<4;r++){
 unsigned n=0;for(unsigned i=0;i<w.bag_sizes[b][r];i++)if(w_allowed(s,w.bags[b][r][i],false))w.bags[b][r][n++]=w.bags[b][r][i];
 for(unsigned i=n;i<w.bag_sizes[b][r];i++)w.bags[b][r][i]=0;w.bag_sizes[b][r]=uint16_t(n);
}}
bool w_enqueue(State& s,WorldRelicOp op,unsigned owner,int a=0,int b=0,int c=0){auto& w=s.world_relics;
 if(w.step_count>=MaxWorldRelicSteps){w_error(s);return false;}
 // Insertion at the current cursor gives awaited child acquisitions precedence
 // over the continuation of the reward/event that caused them.
 for(unsigned i=w.step_count;i>w.step_cursor;i--)w.steps[i]=w.steps[i-1];
 w.steps[w.step_cursor]={op,uint16_t(owner),a,b,c};++w.step_count;return true;
}
void w_pump(State&);
void w_obtained(State&,WorldRelicInstance&);
void w_select_apply(State&,const uint16_t*,unsigned);
void w_group_show(State&,unsigned);
void w_step(State&,const WorldRelicStep&);
void w_apply_reward_relic(State&,WorldRelicInstance&,Card*,EventCardFacts*,unsigned&,unsigned,const WorldCardOptions&,bool callbacks);
void w_initial_values(WorldRelicInstance& r){
 switch(r.id){
 case WorldRelicBoneTea:case WorldRelicTeaOfDiscourtesy:r.values[0]=1;break;
 case WorldRelicEmberTea:r.values[0]=5;break;
 case WorldRelicFurCoat:case WorldRelicGoldenCompass:r.values[0]=-1;break;
 case WorldRelicPaelsEye:r.values[1]=1;break;
 default:break;
 }
}
void w_pump(State& s){auto& w=s.world_relics;if(w.running||w.choice!=WorldRelicChoice::None)return;
 w.running=1;
 while(w.step_cursor<w.step_count&&w.choice==WorldRelicChoice::None&&s.phase!=Phase::Error&&s.phase!=Phase::Dead){
  WorldRelicStep step=w.steps[w.step_cursor++];w_step(s,step);
 }
 w.running=0;
 if(w.step_cursor==w.step_count&&w.choice==WorldRelicChoice::None){
  for(unsigned i=0;i<w.step_count;i++)w.steps[i]={};w.step_count=w.step_cursor=0;
 }
}
void w_pause(State& s,WorldRelicChoice choice,unsigned serial,unsigned minimum,unsigned maximum,unsigned filter=0){auto& w=s.world_relics;
 if(s.phase!=Phase::WorldRelicChoice)w.return_phase=uint8_t(s.phase);
 w.choice=choice;w.choice_owner=uint16_t(serial);w.choice_min=uint16_t(minimum);w.choice_max=uint16_t(maximum);w.choice_filter=uint8_t(filter);
 w.selected_count=0;s.phase=Phase::WorldRelicChoice;++s.revision;
}
bool w_basic_strike(CardKind k){return k==Strike||k==StrikeSilent||k==DefectStrike||k==RegentStrike||k==NecroStrikeNecrobinder;}
bool w_basic_defend(CardKind k){return k==Defend||k==DefendSilent||k==DefectDefend||k==RegentDefend||k==NecroDefendNecrobinder;}
CardKind w_transcend(CardKind k){switch(k){case Bash:return Break;case Neutralize:return Suppress;case NecroUnleash:return NecroProtector;case RegentFallingStar:return RegentMeteorShower;case DefectDualcast:return DefectQuadcast;default:return CardKinds;}}
CardKind w_strike(Character c){const CardKind ids[]={Strike,StrikeSilent,DefectStrike,RegentStrike,NecroStrikeNecrobinder};return ids[unsigned(c)];}
CardKind w_defend(Character c){const CardKind ids[]={Defend,DefendSilent,DefectDefend,RegentDefend,NecroDefendNecrobinder};return ids[unsigned(c)];}
void w_sort_deck(const State& s,uint16_t* ids,unsigned count){for(unsigned i=1;i<count;i++){uint16_t id=ids[i];unsigned j=i;while(j&&w_cmp(definitions[s.deck[ids[j-1]].kind].key,definitions[s.deck[id].kind].key)>0){ids[j]=ids[j-1];--j;}ids[j]=id;}}
void w_random_upgrade(State& s,unsigned count,unsigned type=0){uint16_t ids[MaxDeck];unsigned n=0;
 for(unsigned i=0;i<s.deck_count;i++)if(card_upgradeable(s.deck[i])&&(!type||unsigned(definitions[s.deck[i].kind].type)+1==type))ids[n++]=uint16_t(i);
 w_sort_deck(s,ids,n);reference::shuffle(ids,n,world_rng(s,WorldRng::Niche));
 for(unsigned i=0;i<n&&i<count;i++){s.deck[ids[i]].upgraded=1;record(s,LogKind::Upgrade,s.deck[ids[i]].kind);}
}
} // namespace

RelicId world_relic_id(const char* key){for(unsigned i=0;i<WorldRelicKinds;i++)if(w_equal(key,world_relic_definitions[i].key)||w_equal(key,world_relic_definitions[i].model))return RelicId(i);return NoWorldRelic;}
const char* world_relic_key(RelicId id){return id<WorldRelicKinds?world_relic_definitions[id].key:nullptr;}
int world_relic_instance_index(const State& s,unsigned serial){if(!serial)return -1;for(unsigned i=0;i<s.world_relics.count;i++)if(s.world_relics.entries[i].serial==serial)return int(i);return -1;}
bool has_world_relic(const State& s,RelicId id){if(!s.world_relics.active&&id<RelicKinds)return (s.relics&(RelicMask(1)<<id))!=0;return w_first(s,id)>=0;}
bool world_relic_available(const State& s,RelicId id,bool shop){return w_allowed(s,id,shop)&&(!has_world_relic(s,id)||(world_relic_definitions[id].flags&8));}
bool world_relic_allowed_at_neow(const State& s,RelicId id){return w_allowed(s,id,false);}
bool world_has_event_pet(const State& s){return has_world_relic(s,WorldRelicByrdpip)||has_world_relic(s,WorldRelicPaelsLegion);}
bool world_relic_tradable(const State& s,RelicId id){int at=w_first(s,id);if(at<0)return false;const auto& r=s.world_relics.entries[at];const auto& d=world_relic_definitions[id];
 return w_present(r)&&!w_used(r)&&!(d.flags&5)&&d.rarity!=1&&d.rarity!=6&&d.rarity!=7;
}
unsigned world_relic_list(const State& s,RelicId* out,unsigned capacity,bool tradable){unsigned n=0;
 if(!s.world_relics.active){for(unsigned i=0;i<s.relic_count;i++){RelicId id=s.relic_order[i];if(!tradable||(world_relic_definitions[id].rarity>=2&&world_relic_definitions[id].rarity<=5&&!(world_relic_definitions[id].flags&5))){if(out&&n<capacity)out[n]=id;++n;}}return n;}
 for(unsigned i=0;i<s.world_relics.count;i++){RelicId id=s.world_relics.entries[i].id;if(!tradable||world_relic_tradable(s,id)){if(out&&n<capacity)out[n]=id;++n;}}return n;
}
unsigned world_relic_candidates(const State& s,RelicId* out,unsigned capacity,WorldRelicPool pool,bool shop){unsigned n=0,cpn=0;const RelicId* cp=w_character_pool(s.character,cpn);
 for(unsigned i=0;i<w_count(WSharedRelicPool)+cpn;i++){
  RelicId id=i<w_count(WSharedRelicPool)?WSharedRelicPool[i]:cp[i-w_count(WSharedRelicPool)];unsigned rarity=world_relic_definitions[id].rarity;
  if(pool==WorldRelicPool::Reward?(rarity<2||rarity>4):rarity!=unsigned(pool))continue;
  if(!world_relic_available(s,id,shop))continue;if(out&&n<capacity)out[n]=id;++n;
 }return n;
}
void init_world_relics(State& s){auto& w=s.world_relics;if(w.active)return;w.active=1;
 for(unsigned i=0;i<s.relic_count;i++){
  RelicId id=s.relic_order[i];if(id>=RelicKinds||w.count>=WorldRelicCapacity){w_error(s);return;}
  auto& r=w.entries[w.count++];r={};r.id=id;r.serial=++w.last_serial;w_initial_values(r);r.values[0]=s.relic_counters[id];
 }
 w_bags(s);
}
RelicId world_relic_pull(State& s,WorldRelicPool pool,bool back,bool shop){init_world_relics(s);w_bags(s);w_clean_bags(s);auto& w=s.world_relics;
 if(pool==WorldRelicPool::Reward)return random_world_relic(s,pool);
 unsigned rarity=unsigned(pool);while(rarity>=2&&rarity<=5){unsigned r=rarity-2,n=w.bag_sizes[1][r];
  for(unsigned t=0;t<n;t++){unsigned at=back?n-1-t:t;RelicId id=w.bags[1][r][at];if(!w_allowed(s,id,shop))continue;
   for(unsigned j=at+1;j<n;j++)w.bags[1][r][j-1]=w.bags[1][r][j];w.bags[1][r][n-1]=0;--w.bag_sizes[1][r];w_bag_remove(s,id,0);return id;
  }
  rarity=rarity==5?2:rarity==2?3:rarity==3?4:0;
 }return WorldRelicCirclet;
}
RelicId random_world_relic(State& s,WorldRelicPool pool){
 if(pool==WorldRelicPool::Reward){auto v=world_random_float(world_rng(s,WorldRng::Rewards));pool=v<world_float(0x3f000000)?WorldRelicPool::Common:v<world_float(0x3f547ae1)?WorldRelicPool::Uncommon:WorldRelicPool::Rare;}
 return world_relic_pull(s,pool);
}
void world_relic_flash(State& s,RelicId id){s.world_relics.last_flash=id;++s.world_relics.flash_revision;record(s,LogKind::RelicIndex,id);}
bool gain_world_relic(State& s,RelicId id){if(id>=WorldRelicKinds)return false;init_world_relics(s);if(!w_enqueue(s,WorldRelicOp::Obtain,0,id))return false;w_pump(s);++s.revision;return s.phase!=Phase::Error;}
bool lose_world_relic(State& s,RelicId id){init_world_relics(s);int at=w_first(s,id);if(at<0)return false;auto& w=s.world_relics;
 for(unsigned i=unsigned(at)+1;i<w.count;i++)w.entries[i-1]=w.entries[i];w.entries[--w.count]={};
 if(id<RelicKinds&&w_first(s,id)<0){s.relics&=~(RelicMask(1)<<id);for(unsigned i=0;i<s.relic_count;i++)if(s.relic_order[i]==id){for(unsigned j=i+1;j<s.relic_count;j++)s.relic_order[j-1]=s.relic_order[j];s.relic_order[--s.relic_count]=0;break;}s.relic_counters[id]=0;}
 ++s.revision;return true;
}
bool world_relic_pending(const State& s){return s.world_relics.choice!=WorldRelicChoice::None||s.world_relics.step_cursor<s.world_relics.step_count;}
int world_relic_counter(const State& s,RelicId id){int at=w_first(s,id);if(at<0)return 0;const auto& r=s.world_relics.entries[at];
 switch(id){case WorldRelicPaelsTooth:return s.world_relics.tooth_count;case WorldRelicToyBox:case WorldRelicFishingRod:return r.values[0]%3;case WorldRelicWingedBoots:return w_max(0,3-r.values[0]);default:return r.values[0];}
}

bool world_relic_golden_path(const State& s,unsigned act){for(unsigned i=0;i<s.world_relics.count;i++){const auto& r=s.world_relics.entries[i];if(r.id==WorldRelicGoldenCompass&&w_present(r)&&r.values[0]==int(act))return true;}return false;}
void world_relic_map_generated(State& s){const auto* map=world_current_map(s);if(!map)return;
 for(unsigned ri=0;ri<s.world_relics.count;ri++){auto& r=s.world_relics.entries[ri];if(r.id!=WorldRelicFurCoat||!w_present(r)||r.values[0]!=int(world_act_index(s)))continue;
  bool valid=r.values[1]!=0;
  for(unsigned i=0;valid&&i<unsigned(r.values[2]);i++){unsigned p=r.marked_points[i];valid=p<map->count&&map->points[p].active&&(map->points[p].type==reference::PointType::Monster||map->points[p].type==reference::PointType::Elite);}
  if(valid)continue;
  uint8_t points[reference::MaxPoints];unsigned n=0;
  for(unsigned i=0;i<map->count;i++){const auto& p=map->points[i];if(p.active&&(p.type==reference::PointType::Monster||p.type==reference::PointType::Elite)){
   bool marked=false;for(unsigned j=0;j<ri;j++){const auto& other=s.world_relics.entries[j];if(other.id==WorldRelicFurCoat&&other.values[0]==r.values[0]&&other.values[1])for(unsigned k=0;k<unsigned(other.values[2]);k++)if(other.marked_points[k]==i)marked=true;}
   if(!marked)points[n++]=uint8_t(i);
  }}
  reference::Random rng;rng.named(world_run_seed(s),"FUR_COAT");reference::shuffle(points,n,rng);r.values[1]=1;r.values[2]=int(w_min(8,int(n)));
  for(unsigned i=0;i<8;i++)r.marked_points[i]=i<unsigned(r.values[2])?points[i]:0;
 }
}
bool world_relic_fur_room(const State& s){const auto* map=world_current_map(s);if(!map)return false;
 for(unsigned i=0;i<s.world_relics.count;i++){const auto& r=s.world_relics.entries[i];if(r.id!=WorldRelicFurCoat||!w_present(r)||r.values[0]!=int(world_act_index(s))||!r.values[1])continue;
  for(unsigned j=0;j<unsigned(r.values[2]);j++){unsigned p=r.marked_points[j];if(p<map->count&&map->points[p].col==world_map_column(s)&&map->points[p].row==world_map_row(s))return true;}
 }return false;
}

int world_relic_prepared_value(const State& s,RelicId id,unsigned which){if(which>1)return -1;for(unsigned i=0;i<s.world_relics.prepared_count;i++)if(s.world_relics.prepared[i].id==id)return s.world_relics.prepared[i].values[which];return -1;}
void world_relic_clear_offers(State& s){for(auto& p:s.world_relics.prepared)p={};s.world_relics.prepared_count=0;}
bool world_relic_prepare(State& s,RelicId id,int value,int second){
 if(id>=WorldRelicKinds)return false;
 bool needs=id==WorldRelicDustyTome||id==WorldRelicArchaicTooth||id==WorldRelicTouchOfOrobas||id==WorldRelicSeaGlass;
 if(!needs)return true;
 for(unsigned i=0;i<s.world_relics.prepared_count;i++)if(s.world_relics.prepared[i].id==id)return true;
 if(value<0){switch(id){
 case WorldRelicDustyTome:{WorldCardOptions o;o.rarity_mask=world_rarity_mask(5);CardKind cards[CardKinds];unsigned n=world_card_candidates(s,o,cards,CardKinds),out=0;
  for(unsigned i=0;i<n;i++)if(cards[i]!=Break&&cards[i]!=Suppress&&cards[i]!=NecroProtector&&cards[i]!=RegentMeteorShower&&cards[i]!=DefectQuadcast)cards[out++]=cards[i];
  if(!out)return false;value=cards[world_rng(s,WorldRng::Rewards).below(out)];break;}
 case WorldRelicArchaicTooth:for(unsigned i=0;i<s.deck_count;i++)if(w_transcend(s.deck[i].kind)<CardKinds){value=int(i);second=w_transcend(s.deck[i].kind);break;}if(value<0)return false;break;
 case WorldRelicTouchOfOrobas:{for(unsigned i=0;i<s.world_relics.count;i++)if(world_relic_definitions[s.world_relics.entries[i].id].rarity==1){value=s.world_relics.entries[i].id;break;}
  if(value<0)for(unsigned i=0;i<s.relic_count;i++)if(world_relic_definitions[s.relic_order[i]].rarity==1){value=s.relic_order[i];break;}
  if(value<0)return false;switch(value){case WorldRelicBurningBlood:second=WorldRelicBlackBlood;break;case WorldRelicRingOfTheSnake:second=WorldRelicRingOfTheDrake;break;case WorldRelicCrackedCore:second=WorldRelicInfusedCore;break;case WorldRelicDivineRight:second=WorldRelicDivineDestiny;break;case WorldRelicBoundPhylactery:second=WorldRelicPhylacteryUnbound;break;default:second=WorldRelicCirclet;}break;}
 case WorldRelicSeaGlass:value=0;break;
 default:break;
 }}
 auto& w=s.world_relics;if(w.prepared_count>=16){w_error(s);return false;}
 w.prepared[w.prepared_count++]={id,{int16_t(value),int16_t(second)}};return true;
}
bool world_relic_setup_offer(State& s,RelicId id,int value){return world_relic_prepare(s,id,value);}

namespace {
void w_forget_prepared(State& s,RelicId id){auto& w=s.world_relics;for(unsigned i=0;i<w.prepared_count;i++)if(w.prepared[i].id==id){for(unsigned j=i+1;j<w.prepared_count;j++)w.prepared[j-1]=w.prepared[j];w.prepared[--w.prepared_count]={};return;}}
WorldRelicCardSnapshot w_snapshot(const State& s,unsigned i){return {uint16_t(card_token(s.deck[i])),s.defect.genetic_deck[i],s.necro.scythe_deck[i],s.event_cards.deck[i]};}
int w_restore_card(State& s,const WorldRelicCardSnapshot& snap,bool upgrade=false){Card c=card_from_token(snap.token);if(upgrade&&card_upgradeable(c))c.upgraded=1;int idx=event_add_deck_card(s,c,&snap.facts);if(idx>=0){s.defect.genetic_deck[idx]=snap.genetic;s.necro.scythe_deck[idx]=snap.scythe;}return idx;}
bool w_transform(State& s,unsigned idx,reference::Random& rng,CardKind replacement=CardKinds,bool upgrade=false,bool carry=false){if(idx>=s.deck_count)return false;Card c;EventCardFacts f;
 if(!world_transform_card(s,rng,s.deck[idx],false,c,f,replacement))return false;
 if(carry){c.upgraded=s.deck[idx].upgraded;f=s.event_cards.deck[idx];}
 if(upgrade&&card_upgradeable(c))c.upgraded=1;
 return event_replace_deck_card(s,int(idx),c,&f);
}
void w_add(State& s,CardKind kind,unsigned count=1,bool upgraded=false){for(unsigned i=0;i<count;i++)event_add_deck_card(s,{kind,uint8_t(upgraded&&card_upgradeable({kind,0}))});}
void w_curses(State& s,unsigned count){WorldCardOptions o;o.pools[0]=WorldCardPool::Curse;CardKind cards[CardKinds];unsigned n=world_card_candidates(s,o,cards,CardKinds),out=0;
 for(unsigned i=0;i<n;i++)if(world_card_modifier_allowed(cards[i]))cards[out++]=cards[i];n=out;world_stable_cards(cards,n);
 for(unsigned i=0;i<count&&n;i++){unsigned at=world_rng(s,WorldRng::Niche).below(n);w_add(s,cards[at]);for(unsigned j=at+1;j<n;j++)cards[j-1]=cards[j];--n;}
}
// Choice filters are persisted so saves can reconstruct the exact allowed set.
enum WFilter {WAny,WRemove,WUpgradeable,WTransform,WNonQuest,WEnchant,WTooth,WClaws};
bool w_deck_eligible(const State& s,unsigned idx){if(idx>=s.deck_count)return false;Card c=s.deck[idx];const auto& w=s.world_relics;switch(w.choice_filter){
 case WRemove:return card_removable(c);case WUpgradeable:return card_upgradeable(c);case WTransform:case WClaws:return card_transformable(c);
 case WNonQuest:return definitions[c.kind].type!=CardType::Quest;case WEnchant:return event_card_can_enchant(s,int(idx),w.choice_enchantment);
 case WTooth:return card_removable(c)&&card_upgradeable(c);default:return true;}}
void w_choose_deck(State& s,WorldRelicInstance& r,WorldRelicChoice kind,unsigned maximum,unsigned filter,unsigned minimum=65535,EventEnchantment enchant=EventEnchantment::None,int amount=0){
 auto& w=s.world_relics;w.choice_filter=uint8_t(filter);w.choice_enchantment=enchant;w.choice_amount=int16_t(amount);
 unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(w_deck_eligible(s,i))++n;
 maximum=unsigned(w_min(int(maximum),int(n)));if(minimum==65535)minimum=maximum;minimum=unsigned(w_min(int(minimum),int(maximum)));
 if(!maximum)return;w.choice_cancelable=uint8_t(minimum==0);w.offer_count=0;w_pause(s,kind,r.serial,minimum,maximum,filter);
}
void w_random_enchant(State& s,EventEnchantment e,unsigned count,int amount){uint16_t ids[MaxDeck];unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(event_card_can_enchant(s,int(i),e))ids[n++]=uint16_t(i);
 // TakeRandom is a complete UnstableShuffle followed by Take, even when only
 // a small prefix is requested; this preserves the Niche stream consumption.
 reference::shuffle(ids,n,world_rng(s,WorldRng::Niche));count=unsigned(w_min(int(count),int(n)));
 for(unsigned i=0;i<count;i++)event_card_enchant(s,ids[i],e,amount);
}
unsigned w_new_group(State& s,unsigned owner,bool skip=true){auto& w=s.world_relics;for(unsigned i=0;i<MaxWorldRelicRewardGroups;i++)if(!w.group_active[i]){
 w.group_active[i]=1;w.group_skip[i]=uint8_t(skip);w.group_owner[i]=uint16_t(owner);if(i>=w.group_count)w.group_count=uint8_t(i+1);return i+1;
 }w_error(s);return 0;}
WorldRelicReward* w_reward_slot(State& s,unsigned group,unsigned kind){if(!group)return nullptr;auto& w=s.world_relics;for(unsigned i=0;i<MaxWorldRelicRewards;i++)if(!w.rewards[i].group){
 auto& r=w.rewards[i];r={};r.group=uint8_t(group);r.kind=uint8_t(kind);if(i>=w.reward_count)w.reward_count=uint8_t(i+1);return &r;
 }w_error(s);return nullptr;}
void w_group_free(State& s,unsigned group){auto& w=s.world_relics;if(!group||group>MaxWorldRelicRewardGroups)return;
 for(auto& r:w.rewards)if(r.group==group)r={};w.group_active[group-1]=0;w.group_skip[group-1]=0;w.group_owner[group-1]=0;
 while(w.reward_count&&!w.rewards[w.reward_count-1].group)--w.reward_count;
 while(w.group_count&&!w.group_active[w.group_count-1])--w.group_count;
}
void w_reward_relic(State& s,unsigned group,RelicId id,bool wax=false){auto* r=w_reward_slot(s,group,2);if(r){r->value=id;r->flags=uint8_t(wax?2:0);}}
void w_reward_potion(State& s,unsigned group){auto* r=w_reward_slot(s,group,3);if(r){Potion p;world_random_potions(s,world_rng(s,WorldRng::Rewards),1,&p);r->value=uint16_t(p);}}
WorldCardOptions w_options(WorldCardOdds odds=WorldCardOdds::RegularEncounter,unsigned rarity=0){WorldCardOptions o;o.odds=odds;o.flags=WorldIsCardReward|WorldNoUpgradeRoll;o.rarity_mask=rarity?world_rarity_mask(rarity):0;return o;}
void w_store_options(WorldRelicReward& r,const WorldCardOptions& o){r.rarity_mask=o.rarity_mask;r.creation_flags=o.flags;r.pool_count=o.pool_count;r.odds=uint8_t(o.odds);r.source=uint8_t(o.source);for(unsigned i=0;i<o.pool_count;i++)r.pools[i]=uint8_t(o.pools[i]);}
bool w_reward_cards(State& s,unsigned group,const WorldCardOptions& o,unsigned n=3){auto* r=w_reward_slot(s,group,1);if(!r)return false;Card cards[8];EventCardFacts facts[8];
 if(n>8||!world_create_reward(s,world_rng(s,WorldRng::Rewards),n,o,cards,facts,8,&n)){w_error(s);return false;}
 r->count=uint8_t(n);for(unsigned i=0;i<n;i++){r->tokens[i]=uint16_t(card_token(cards[i]));r->facts[i]=facts[i];}w_store_options(*r,o);
 if(w_first(s,WorldRelicDriftwood,true)>=0)r->flags|=16;return true;
}
void w_schedule_group(State& s,unsigned group){if(group)w_enqueue(s,WorldRelicOp::RewardGroup,s.world_relics.group_owner[group-1],int(group));}
void w_group_show(State& s,unsigned group){auto& w=s.world_relics;if(!group||group>w.group_count||!w.group_active[group-1])return;w.offer_count=0;
 for(unsigned i=0;i<w.reward_count;i++)if(w.rewards[i].group==group&&!(w.rewards[i].flags&1))w.offers[w.offer_count++]=uint16_t(i);
 if(!w.offer_count){w_group_free(s,group);return;}w.choice_group=uint8_t(group);w.choice_reward=0;w.choice_cancelable=w.group_skip[group-1];
 w_pause(s,WorldRelicChoice::OfferReward,w.group_owner[group-1],w.choice_cancelable?0:1,1);
}
void w_show_cards(State& s,unsigned owner,const Card* cards,const EventCardFacts* facts,unsigned count,unsigned minimum=0,unsigned maximum=1){auto& w=s.world_relics;
 if(count>MaxWorldRelicOffers){w_error(s);return;}w.offer_count=uint16_t(count);for(unsigned i=0;i<count;i++){w.offers[i]=uint16_t(card_token(cards[i]));w.offer_facts[i]=facts[i];}
 w.choice_cancelable=uint8_t(minimum==0);w_pause(s,WorldRelicChoice::OfferCard,owner,minimum,unsigned(w_min(int(maximum),int(count))));
}
void w_single_card_offer(State& s,unsigned owner,WorldCardOptions options,unsigned count){Card cards[8];EventCardFacts facts[8];
 if(count>8||!world_create_reward(s,world_rng(s,WorldRng::Rewards),count,options,cards,facts,8,&count)){w_error(s);return;}
 s.world_relics.choice_group=s.world_relics.choice_reward=0;w_show_cards(s,owner,cards,facts,count);
}
void w_basic_transform(State& s,bool all,bool remove_hp=false){uint16_t indices[MaxDeck];unsigned count=0;bool strike=false,defend=false;
 for(unsigned i=0;i<s.deck_count;i++){Card c=s.deck[i];bool a=w_basic_strike(c.kind),b=w_basic_defend(c.kind);if((a||b)&&card_removable(c)&&(all||(a&&!strike)||(b&&!defend))){indices[count++]=uint16_t(i);strike|=a;defend|=b;}}
 (void)remove_hp;
 for(unsigned i=0;i<count;i++)w_transform(s,indices[i],world_rng(s,all?WorldRng::Niche:WorldRng::Transformations));
}
void w_scroll_boxes(State& s,WorldRelicInstance& r){WorldCardOptions o=w_options(WorldCardOdds::Uniform,2);o.flags|=WorldNoRarityModification;world_relic_modify_card_creation(s,o);
 CardKind common[CardKinds],uncommon[CardKinds];unsigned nc=world_card_candidates(s,o,common,CardKinds);o.rarity_mask=world_rarity_mask(3);world_relic_modify_card_creation(s,o);unsigned nu=world_card_candidates(s,o,uncommon,CardKinds);
 Card cards[6];EventCardFacts facts[6]{};bool used[CardKinds]{};auto& rng=world_rng(s,WorldRng::Rewards);
 for(unsigned b=0;b<2;b++){
  if(s.character==Character::Defect&&rng.below(100)<1){for(unsigned i=0;i<3;i++)cards[b*3+i]={DefectClaw,0};continue;}
  for(unsigned k=0;k<3;k++){auto* pool=k<2?common:uncommon;unsigned n=k<2?nc:nu;CardKind available[CardKinds];unsigned count=0;for(unsigned i=0;i<n;i++)if(!used[pool[i]])available[count++]=pool[i];
   if(!count){w_error(s);return;}CardKind card=available[rng.below(count)];used[card]=true;cards[b*3+k]={card,0};
  }
 }
 auto& w=s.world_relics;w.choice_group=w.choice_reward=0;w.offer_count=6;for(unsigned i=0;i<6;i++){w.offers[i]=uint16_t(card_token(cards[i]));w.offer_facts[i]=facts[i];}
 w.choice_cancelable=0;w_pause(s,WorldRelicChoice::OfferBundle,r.serial,1,1);
}
void w_kaleidoscope(State& s,WorldRelicInstance& r){unsigned group=w_new_group(s,r.serial);for(unsigned b=0;b<2;b++){
 unsigned pools[4],n=0;for(unsigned c=0;c<CharacterKinds;c++)if(c!=unsigned(s.character))pools[n++]=c;
 // StableShuffle sorts CharacterCardPool models by their original model ID.
 for(unsigned i=1;i<n;i++){unsigned x=pools[i],j=i;const char* names[]={"IRONCLAD_CARD_POOL","SILENT_CARD_POOL","DEFECT_CARD_POOL","REGENT_CARD_POOL","NECROBINDER_CARD_POOL"};while(j&&w_cmp(names[pools[j-1]],names[x])>0){pools[j]=pools[j-1];--j;}pools[j]=x;}
 reference::shuffle(pools,n,world_rng(s,WorldRng::Niche));auto* reward=w_reward_slot(s,group,1);if(!reward)return;reward->count=3;
 for(unsigned i=0;i<3;i++){WorldCardOptions o=w_options();o.flags=WorldNoUpgradeRoll|WorldNoCardPoolModifications;o.pools[0]=WorldCardPool(pools[i]);Card c;EventCardFacts f;
  if(!world_create_reward(s,world_rng(s,WorldRng::Rewards),1,o,&c,&f)){w_error(s);return;}reward->tokens[i]=uint16_t(card_token(c));reward->facts[i]=f;
 }
 WorldCardOptions late=w_options(WorldCardOdds::Uniform);late.flags=WorldNoCardPoolModifications|WorldNoCardModelModifications|WorldIsCardReward;late.pool_count=0;
 Card cards[8];EventCardFacts facts[8];for(unsigned i=0;i<3;i++){cards[i]=card_from_token(reward->tokens[i]);facts[i]=reward->facts[i];}
 unsigned count=3;world_relic_modify_reward_options(s,cards,facts,count,8,late);reward->count=uint8_t(count);for(unsigned i=0;i<count;i++){reward->tokens[i]=uint16_t(card_token(cards[i]));reward->facts[i]=facts[i];}
 w_store_options(*reward,w_options());
 }w_schedule_group(s,group);
}
void w_neows_bones(State& s,WorldRelicInstance& r){
 const RelicId all[]={WorldRelicCursedPearl,WorldRelicDowsingRod,WorldRelicHeftyTablet,WorldRelicLargeCapsule,WorldRelicLeafyPoultice,
 WorldRelicNeowsSacrifice,WorldRelicPrecariousShears,WorldRelicSilkenTress,WorldRelicSilverCrucible,
 WorldRelicArcaneScroll,WorldRelicBoomingConch,WorldRelicFishingRod,WorldRelicGoldenPearl,WorldRelicKaleidoscope,
 WorldRelicLeadPaperweight,WorldRelicLostCoffer,WorldRelicNeowsTorment,WorldRelicNewLeaf,WorldRelicPhialHolster,
 WorldRelicPreciseScissors,WorldRelicScrollBoxes,WorldRelicWingedBoots,WorldRelicLavaRock,WorldRelicNeowsTalisman,
 WorldRelicNutritiousOyster,WorldRelicPomander,WorldRelicSmallCapsule,WorldRelicStoneHumidifier};
 RelicId ids[w_count(all)];unsigned n=0;for(auto id:all)if(world_relic_allowed_at_neow(s,id))ids[n++]=id;
 reference::shuffle(ids,n,world_rng(s,WorldRng::Rewards));unsigned g=w_new_group(s,r.serial,false);
 for(unsigned i=0;i<n&&i<2;i++)w_reward_relic(s,g,ids[i]);w_enqueue(s,WorldRelicOp::Special,r.serial,1);w_schedule_group(s,g);
}
void w_obtained(State& s,WorldRelicInstance& r){unsigned id=r.id,owner_serial=r.serial;auto& w=s.world_relics;
 switch(id){
 case WorldRelicStrawberry:increase_max_hp(s,7);break;case WorldRelicPear:increase_max_hp(s,10);break;case WorldRelicMango:increase_max_hp(s,14);break;
 case WorldRelicBigMushroom:increase_max_hp(s,20);break;case WorldRelicFakeMango:increase_max_hp(s,3);break;
 case WorldRelicLoomingFruit:increase_max_hp(s,31);break;case WorldRelicNutritiousOyster:increase_max_hp(s,11);break;
 case WorldRelicOldCoin:gain_gold(s,300);break;case WorldRelicGoldenPearl:gain_gold(s,150);break;case WorldRelicSignetRing:gain_gold(s,888);break;
 case WorldRelicLeesWaffle:increase_max_hp(s,7);heal(s,s.max_hp);break;case WorldRelicFakeLeesWaffle:heal(s,s.max_hp/10);break;
 case WorldRelicPotionBelt:add_potion_slots(s,2);break;
 case WorldRelicAlchemicalCoffer:{unsigned old=potion_slot_count(s);if(!add_potion_slots(s,4))break;Potion potions[4];unsigned n=world_random_potions(s,world_rng(s,WorldRng::CombatPotionGeneration),4,potions);
  for(unsigned i=0;i<n;i++)potion_set(s,old+i,potions[i]);break;}
 case WorldRelicPhialHolster:{add_potion_slots(s,1);Potion p[2];unsigned n=world_random_potions(s,world_rng(s,WorldRng::CombatPotionGeneration),2,p);for(unsigned i=0;i<n;i++)procure_potion(s,p[i]);break;}
 case WorldRelicArcaneScroll:{WorldCardOptions o=w_options(WorldCardOdds::Uniform,4);o.flags=WorldNoUpgradeRoll;Card c;EventCardFacts f;if(world_create_reward(s,world_rng(s,WorldRng::Rewards),1,o,&c,&f))event_add_deck_card(s,c,&f);else w_error(s);break;}
 case WorldRelicArchaicTooth:{if(!world_relic_prepare(s,r.id))break;int at=world_relic_prepared_value(s,r.id);if(at>=0&&at<s.deck_count)w_transform(s,unsigned(at),world_rng(s,WorldRng::Transformations),w_transcend(s.deck[at].kind),false,true);w_forget_prepared(s,r.id);break;}
 case WorldRelicAstrolabe:w_choose_deck(s,r,WorldRelicChoice::Transform,3,WTransform);break;
 case WorldRelicBeautifulBracelet:w_random_enchant(s,EventEnchantment::Swift,4,2);break;
 case WorldRelicBiiigHug:w_choose_deck(s,r,WorldRelicChoice::Remove,4,WRemove);break;
 case WorldRelicBloodSoakedRose:w_add(s,EventCardEnthralled);break;
 case WorldRelicByrdpip:{reference::Random rng;rng.named(world_run_seed(s),"BYRDPIP");r.skin=uint8_t(rng.below(4));for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind==EventCardByrdonisEgg)w_transform(s,i,world_rng(s,WorldRng::Transformations),EventCardByrdSwoop);break;}
 case WorldRelicCallingBell:{w_add(s,EventCardCurseOfTheBell);unsigned g=w_new_group(s,r.serial);for(unsigned rarity=2;rarity<=4;rarity++)w_reward_relic(s,g,world_relic_pull(s,WorldRelicPool(rarity)));w_schedule_group(s,g);break;}
 case WorldRelicCauldron:{unsigned g=w_new_group(s,r.serial);for(unsigned i=0;i<5;i++)w_reward_potion(s,g);w_schedule_group(s,g);break;}
 case WorldRelicClaws:w_choose_deck(s,r,WorldRelicChoice::Transform,6,WClaws,0);break;
 case WorldRelicCursedPearl:w_add(s,EventCardGreed);gain_gold(s,333);break;
 case WorldRelicDistinguishedCape:w_curses(s,2);w_add(s,EventCardApparition,3);break;
 case WorldRelicDollysMirror:w_choose_deck(s,r,WorldRelicChoice::Duplicate,1,WNonQuest);break;
 case WorldRelicDowsingRod:w_add(s,EventCardDowsing);break;
 case WorldRelicDustyTome:if(world_relic_prepare(s,r.id)){int card=world_relic_prepared_value(s,r.id);if(card>=0&&card<CardKinds)w_add(s,CardKind(card),1,true);w_forget_prepared(s,r.id);}break;
 case WorldRelicElectricShrymp:w_choose_deck(s,r,WorldRelicChoice::Enchant,1,WEnchant,65535,EventEnchantment::Imbued,1);break;
 case WorldRelicEmptyCage:w_choose_deck(s,r,WorldRelicChoice::Remove,2,WRemove);break;
 case WorldRelicFragrantMushroom:player_hp_loss(s,15,false,false,-1);if(s.hp>0)w_random_upgrade(s,2);break;
 case WorldRelicFurCoat:r.values[0]=int(world_act_index(s));world_relic_map_generated(s);break;
 case WorldRelicGlassEye:{unsigned g=w_new_group(s,r.serial);for(unsigned i=0;i<5;i++){auto o=w_options(WorldCardOdds::Uniform,i<2?2:i<4?3:4);o.flags=WorldNoUpgradeRoll|WorldNoRarityModification|WorldIsCardReward;w_reward_cards(s,g,o);}w_schedule_group(s,g);break;}
 case WorldRelicGnarledHammer:w_choose_deck(s,r,WorldRelicChoice::Enchant,3,WEnchant,0,EventEnchantment::Sharp,3);break;
 case WorldRelicGoldenCompass:r.values[0]=int(world_act_index(s));break;
 case WorldRelicHeftyTablet:{w_enqueue(s,WorldRelicOp::AddCard,r.serial,EventCardInjury,1);WorldCardOptions o=w_options(WorldCardOdds::Uniform,4);o.flags=WorldNoUpgradeRoll;w_single_card_offer(s,r.serial,o,3);break;}
 case WorldRelicJewelryBox:w_add(s,EventCardApotheosis);break;
 case WorldRelicKaleidoscope:w_kaleidoscope(s,r);break;
 case WorldRelicKifuda:w_choose_deck(s,r,WorldRelicChoice::Enchant,3,WEnchant,0,EventEnchantment::Adroit,3);break;
 case WorldRelicLargeCapsule:w_enqueue(s,WorldRelicOp::AddCard,r.serial,w_defend(s.character),1);w_enqueue(s,WorldRelicOp::AddCard,r.serial,w_strike(s.character),1);w_enqueue(s,WorldRelicOp::RelicReward,r.serial,-1,0,1);w_enqueue(s,WorldRelicOp::RelicReward,r.serial,-1,0,1);break;
 case WorldRelicLeadPaperweight:{WorldCardOptions o=w_options();o.flags=WorldNoUpgradeRoll;o.pools[0]=WorldCardPool::Colorless;w_single_card_offer(s,r.serial,o,2);break;}
 case WorldRelicLeafyPoultice:s.max_hp=w_max(1,s.max_hp-12);if(s.hp>s.max_hp)s.hp=s.max_hp;w_basic_transform(s,false);break;
 case WorldRelicLostCoffer:{unsigned g=w_new_group(s,r.serial);w_reward_cards(s,g,w_options());w_reward_potion(s,g);w_schedule_group(s,g);break;}
 case WorldRelicNeowsBones:w_neows_bones(s,r);break;
 case WorldRelicNeowsSacrifice:procure_potion(s,Potion::Ambergris);w_add(s,EventCardGuilty);break;
 case WorldRelicNeowsTalisman:{int strike=-1,defend=-1;for(unsigned i=0;i<s.deck_count;i++){if(w_basic_strike(s.deck[i].kind))strike=int(i);if(w_basic_defend(s.deck[i].kind))defend=int(i);}if(strike>=0&&card_upgradeable(s.deck[strike]))s.deck[strike].upgraded=1;if(defend>=0&&card_upgradeable(s.deck[defend]))s.deck[defend].upgraded=1;break;}
 case WorldRelicNeowsTorment:w_add(s,EventCardNeowsFury);break;
 case WorldRelicNewLeaf:w_choose_deck(s,r,WorldRelicChoice::Transform,1,WTransform);break;
 case WorldRelicNutritiousSoup:for(unsigned i=0;i<s.deck_count;i++)if(w_basic_strike(s.deck[i].kind)&&event_card_can_enchant(s,int(i),EventEnchantment::TezcatarasEmber))event_card_enchant(s,int(i),EventEnchantment::TezcatarasEmber,1);break;
 case WorldRelicOrrery:{unsigned g=w_new_group(s,r.serial);for(unsigned i=0;i<5;i++)w_reward_cards(s,g,w_options());w_schedule_group(s,g);break;}
 case WorldRelicPaelsClaw:for(unsigned i=0;i<s.deck_count;i++)if(event_card_can_enchant(s,int(i),EventEnchantment::Goopy))event_card_enchant(s,int(i),EventEnchantment::Goopy,1);break;
 case WorldRelicPaelsEye:r.values[1]=s.phase==Phase::Combat;break;
 case WorldRelicPaelsGrowth:w_choose_deck(s,r,WorldRelicChoice::Enchant,1,WEnchant,65535,EventEnchantment::Clone,4);break;
 case WorldRelicPaelsHorn:w_add(s,EventCardRelax,2);break;
 case WorldRelicPaelsLegion:{reference::Random rng;rng.named(world_run_seed(s),"PAELS_LEGION");r.skin=uint8_t(rng.below(4));break;}
 case WorldRelicPaelsTooth:w_choose_deck(s,r,WorldRelicChoice::Remove,5,WTooth);break;
 case WorldRelicPandorasBox:w_basic_transform(s,true);break;
 case WorldRelicPomander:w_choose_deck(s,r,WorldRelicChoice::Upgrade,1,WUpgradeable);break;
 case WorldRelicPrecariousShears:w_enqueue(s,WorldRelicOp::Damage,r.serial,16);w_choose_deck(s,r,WorldRelicChoice::Remove,2,WRemove);break;
 case WorldRelicPreciseScissors:w_choose_deck(s,r,WorldRelicChoice::Remove,1,WRemove);break;
 case WorldRelicPreservedFog:w_enqueue(s,WorldRelicOp::AddCard,r.serial,EventCardFolly,1);w_choose_deck(s,r,WorldRelicChoice::Remove,3,WRemove);break;
 case WorldRelicPumpkinCandle:r.values[0]+=5;break;
 case WorldRelicPunchDagger:w_choose_deck(s,r,WorldRelicChoice::Enchant,1,WEnchant,65535,EventEnchantment::Momentum,5);break;
 case WorldRelicRoyalStamp:{uint16_t ids[MaxDeck];unsigned n=0;for(unsigned i=0;i<s.deck_count;i++)if(event_card_can_enchant(s,int(i),EventEnchantment::RoyallyApproved))ids[n++]=uint16_t(i);reference::shuffle(ids,n,world_rng(s,WorldRng::Niche));w_choose_deck(s,r,WorldRelicChoice::Enchant,1,WEnchant,65535,EventEnchantment::RoyallyApproved,1);w.offer_count=uint16_t(n);for(unsigned i=0;i<n;i++)w.offers[i]=ids[i];break;}
 case WorldRelicSandCastle:w_random_upgrade(s,6);break;
 case WorldRelicScrollBoxes:w_scroll_boxes(s,r);break;
 case WorldRelicSeaGlass:{world_relic_prepare(s,r.id);int character=world_relic_prepared_value(s,r.id);WorldCardOptions o=w_options(WorldCardOdds::Uniform);o.pools[0]=WorldCardPool(character<0?0:character);o.flags=WorldNoRarityModification|WorldNoCardPoolModifications|WorldNoUpgradeRoll;
  Card cards[15];EventCardFacts facts[15];for(unsigned i=0;i<3;i++){o.rarity_mask=world_rarity_mask(i+2);if(!world_create_reward(s,world_rng(s,WorldRng::Rewards),5,o,cards+i*5,facts+i*5)){w_error(s);return;}}
  w.choice_group=w.choice_reward=0;w_show_cards(s,r.serial,cards,facts,15,0,15);w_forget_prepared(s,r.id);break;}
 case WorldRelicSereTalon:s.max_hp=w_max(1,s.max_hp-9);if(s.hp>s.max_hp)s.hp=s.max_hp;w_add(s,EventCardWish,3);break;
 case WorldRelicSilkenTress:s.gold=0;break;
 case WorldRelicSmallCapsule:{unsigned g=w_new_group(s,r.serial);w_reward_relic(s,g,random_world_relic(s));w_schedule_group(s,g);break;}
 case WorldRelicStorybook:w_add(s,EventCardBrightestFlame);break;
 case WorldRelicTanxsWhistle:w_add(s,EventCardWhistle);break;
 case WorldRelicTouchOfOrobas:{if(!world_relic_prepare(s,r.id))break;RelicId from=RelicId(world_relic_prepared_value(s,r.id)),to=RelicId(world_relic_prepared_value(s,r.id,1));int at=w_first(s,from);if(at>=0&&to<WorldRelicKinds){
  unsigned serial=w.entries[at].serial;lose_world_relic(s,from);w_enqueue(s,WorldRelicOp::Obtain,serial,to,at+1);}
  w_forget_prepared(s,WorldRelicTouchOfOrobas);break;}
 case WorldRelicToyBox:{unsigned g=w_new_group(s,r.serial);for(unsigned i=0;i<5;i++)w_reward_relic(s,g,random_world_relic(s),true);w_schedule_group(s,g);break;}
 case WorldRelicTriBoomerang:w_choose_deck(s,r,WorldRelicChoice::Enchant,3,WEnchant,65535,EventEnchantment::Instinct,1);break;
 case WorldRelicWarPaint:w_random_upgrade(s,2,unsigned(CardType::Skill)+1);break;case WorldRelicWhetstone:w_random_upgrade(s,2,unsigned(CardType::Attack)+1);break;
 case WorldRelicYummyCookie:w_choose_deck(s,r,WorldRelicChoice::Upgrade,4,WUpgradeable);break;
 default:break;
 }
 // These callbacks only apply when the relic was actually obtained during a
 // running combat; normal room/start callbacks are dispatched independently.
 world_relic_combat_acquired(s,owner_serial);
}
void w_step(State& s,const WorldRelicStep& step){auto& w=s.world_relics;switch(step.op){
 case WorldRelicOp::Obtain:{if(step.a<0||step.a>=WorldRelicKinds||w.count>=WorldRelicCapacity||w.last_serial==65535){w_error(s);return;}
  unsigned at=step.b>0?unsigned(step.b-1):w.count;if(at>w.count)at=w.count;for(unsigned i=w.count;i>at;i--)w.entries[i]=w.entries[i-1];++w.count;
  auto& r=w.entries[at];r={};r.id=RelicId(step.a);r.floor=uint16_t(world_total_floor(s));r.serial=++w.last_serial;r.flags=uint8_t(step.c&WorldRelicWax);w_initial_values(r);
  if(r.id<RelicKinds){RelicMask bit=RelicMask(1)<<r.id;if(!(s.relics&bit)){s.relics|=bit;s.relic_order[s.relic_count++]=uint8_t(r.id);}}
  if(!(world_relic_definitions[r.id].flags&8))w_bag_remove(s,r.id);world_relic_flash(s,r.id);unsigned serial=r.serial;w_obtained(s,r);
  // Existing card rewards subscribe to RelicObtained, including rewards in an
  // outer set while an acquired relic opens a nested set.
  int current=world_relic_instance_index(s,serial);if(current>=0)for(unsigned i=0;i<w.reward_count;i++){auto& reward=w.rewards[i];if(!reward.group||reward.kind!=1||(reward.flags&1))continue;
   Card cards[8];EventCardFacts facts[8];unsigned count=reward.count;for(unsigned j=0;j<count;j++){cards[j]=card_from_token(reward.tokens[j]);facts[j]=reward.facts[j];}
   WorldCardOptions o;o.flags=reward.creation_flags;o.rarity_mask=reward.rarity_mask;o.pool_count=reward.pool_count;o.odds=WorldCardOdds(reward.odds);o.source=WorldCardSource(reward.source);for(unsigned j=0;j<o.pool_count;j++)o.pools[j]=WorldCardPool(reward.pools[j]);
   w_apply_reward_relic(s,w.entries[current],cards,facts,count,8,o,false);reward.count=uint8_t(count);for(unsigned j=0;j<count;j++){reward.tokens[j]=uint16_t(card_token(cards[j]));reward.facts[j]=facts[j];}
  }break;}
 case WorldRelicOp::Damage:player_hp_loss(s,step.a,false,false,-1);break;
 case WorldRelicOp::Heal:heal(s,step.a<0?s.max_hp:step.a);break;
 case WorldRelicOp::MaxHp:if(step.a>=0)increase_max_hp(s,step.a);else{s.max_hp=w_max(1,s.max_hp+step.a);if(s.hp>s.max_hp)s.hp=s.max_hp;}break;
 case WorldRelicOp::Gold:gain_gold(s,step.a);break;
 case WorldRelicOp::AddCard:w_add(s,CardKind(step.a),unsigned(w_max(0,step.b)),step.c!=0);break;
 case WorldRelicOp::RandomUpgrade:w_random_upgrade(s,unsigned(w_max(0,step.a)),unsigned(w_max(0,step.b)));break;
 case WorldRelicOp::TransformBasic:w_basic_transform(s,step.a!=0);break;
 case WorldRelicOp::RelicReward:{RelicId id=step.a<0?random_world_relic(s):RelicId(step.a);
  if(step.c)w_enqueue(s,WorldRelicOp::Obtain,0,id);else{unsigned g=w_new_group(s,step.owner);w_reward_relic(s,g,id,step.b!=0);w_schedule_group(s,g);}break;}
 case WorldRelicOp::RewardGroup:w_group_show(s,unsigned(step.a));break;
 case WorldRelicOp::Special:if(step.a==1)w_curses(s,1);break;
 case WorldRelicOp::None:case WorldRelicOp::Finish:break;
 default:w_error(s);break;
}}
void w_clear_choice(State& s){auto& w=s.world_relics;w.choice=WorldRelicChoice::None;s.phase=Phase(w.return_phase);w.selected_count=0;}
void w_select_apply(State& s,const uint16_t* ids,unsigned count){auto& w=s.world_relics;auto choice=w.choice;unsigned serial=w.choice_owner,filter=w.choice_filter,group=w.choice_group,reward_index=w.choice_reward;
 int owner=world_relic_instance_index(s,serial);RelicId owner_id=owner>=0?w.entries[owner].id:NoWorldRelic;
 w_clear_choice(s);
 if(choice==WorldRelicChoice::OfferReward){
  if(!count){w_group_free(s,group);return;}
  unsigned index=w.offers[ids[0]];auto& reward=w.rewards[index];w.choice_reward=uint8_t(index+1);
  w_enqueue(s,WorldRelicOp::RewardGroup,serial,int(group));
  if(reward.kind==2){reward.flags|=1;w_enqueue(s,WorldRelicOp::Obtain,0,reward.value,0,(reward.flags&2)?WorldRelicWax:0);}
  else if(reward.kind==3){
   bool full=true;for(unsigned i=0;i<potion_slot_count(s);i++)if(potion_at(s,i)==Potion::Empty)full=false;
   if(!full||w_first(s,WorldRelicSozu,true)>=0){procure_potion(s,Potion(reward.value));reward.flags|=1;}
   else{w.offer_count=1;w.offers[0]=reward.value;w.choice_cancelable=1;w_pause(s,WorldRelicChoice::Potion,serial,0,1);}
  }else if(reward.kind==1){Card cards[8];for(unsigned i=0;i<reward.count;i++)cards[i]=card_from_token(reward.tokens[i]);w_show_cards(s,serial,cards,reward.facts,reward.count);}
  return;
 }
 if(choice==WorldRelicChoice::OfferCard){
  Card cards[MaxWorldRelicOffers];EventCardFacts facts[MaxWorldRelicOffers];for(unsigned i=0;i<count;i++){cards[i]=card_from_token(w.offers[ids[i]]);facts[i]=w.offer_facts[ids[i]];}
  if(reward_index&&reward_index<=w.reward_count&&count)w.rewards[reward_index-1].flags|=1;
  for(unsigned i=0;i<count;i++)event_add_deck_card(s,cards[i],&facts[i]);return;
 }
 if(choice==WorldRelicChoice::OfferBundle){unsigned first=ids[0]*3;Card cards[3];EventCardFacts facts[3];for(unsigned i=0;i<3;i++){cards[i]=card_from_token(w.offers[first+i]);facts[i]=w.offer_facts[first+i];}for(unsigned i=0;i<3;i++)event_add_deck_card(s,cards[i],&facts[i]);return;}
 if(choice==WorldRelicChoice::Hand){world_relic_combat_select(s,serial,ids,count);return;}
 uint16_t selected[MaxDeck];for(unsigned i=0;i<count;i++)selected[i]=ids[i];
 if(filter==WTooth){w_sort_deck(s,selected,count);w.tooth_count=uint8_t(count);for(unsigned i=0;i<count;i++)w.tooth_cards[i]=w_snapshot(s,selected[i]);}
 for(unsigned i=0;i<count;i++){
  unsigned at=selected[i];if(choice==WorldRelicChoice::Remove)for(unsigned j=0;j<i;j++)if(selected[j]<selected[i])--at;
  if(at>=s.deck_count){w_error(s);return;}
  switch(choice){
  case WorldRelicChoice::Remove:event_remove_deck_card(s,int(at));break;
  case WorldRelicChoice::Upgrade:if(card_upgradeable(s.deck[at])){s.deck[at].upgraded=1;record(s,LogKind::Upgrade,s.deck[at].kind);}break;
  case WorldRelicChoice::Duplicate:w_restore_card(s,w_snapshot(s,at));break;
  case WorldRelicChoice::Transform:w_transform(s,at,world_rng(s,filter==WClaws?WorldRng::Transformations:WorldRng::Niche),filter==WClaws?CardKind(EventCardMaul):CardKind(CardKinds),owner_id==WorldRelicAstrolabe,filter==WClaws);break;
  case WorldRelicChoice::Enchant:event_card_enchant(s,int(at),w.choice_enchantment,w.choice_amount);break;
  default:w_error(s);return;
  }
 }
}
}

unsigned world_relic_eligible(const State& s,uint16_t* out,unsigned capacity){const auto& w=s.world_relics;unsigned n=0;
 if(w.choice==WorldRelicChoice::None)return 0;
 if(w.choice==WorldRelicChoice::Potion){for(unsigned i=0;i<potion_slot_count(s);i++){if(out&&n<capacity)out[n]=uint16_t(i);++n;}return n;}
 if(w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::OfferReward||w.choice==WorldRelicChoice::OfferBundle){unsigned count=w.choice==WorldRelicChoice::OfferBundle?w.offer_count/3:w.offer_count;for(unsigned i=0;i<count;i++){if(out&&n<capacity)out[n]=uint16_t(i);++n;}return n;}
 if(w.choice==WorldRelicChoice::Hand){for(unsigned i=0;i<w.offer_count;i++){unsigned id=w.offers[i];if(id<s.card_count&&s.zones[id]==Zone::Hand){if(out&&n<capacity)out[n]=uint16_t(id);++n;}}return n;}
 if(w.offer_count&&w.choice==WorldRelicChoice::Enchant){for(unsigned i=0;i<w.offer_count;i++)if(w_deck_eligible(s,w.offers[i])){if(out&&n<capacity)out[n]=w.offers[i];++n;}return n;}
 for(unsigned i=0;i<s.deck_count;i++)if(w_deck_eligible(s,i)){if(out&&n<capacity)out[n]=uint16_t(i);++n;}return n;
}
bool world_relic_select(State& s,const uint16_t* ids,unsigned count){auto& w=s.world_relics;
 if(s.phase!=Phase::WorldRelicChoice||w.choice==WorldRelicChoice::None||count<w.choice_min||count>w.choice_max||(!ids&&count))return false;
 if(w.choice==WorldRelicChoice::Potion)return world_relic_potion(s,count?int(ids[0]):-1);
 uint16_t eligible[MaxWorldRelicOffers];unsigned n=world_relic_eligible(s,eligible,MaxWorldRelicOffers);
 for(unsigned i=0;i<count;i++){bool found=false;for(unsigned j=0;j<n&&j<MaxWorldRelicOffers;j++)if(eligible[j]==ids[i])found=true;for(unsigned j=0;j<i;j++)if(ids[j]==ids[i])return false;if(!found)return false;}
 w_select_apply(s,ids,count);w_pump(s);++s.revision;
 if(!world_relic_pending(s)&&s.phase!=Phase::Error&&s.phase!=Phase::Dead){w.offer_count=0;w.choice_group=w.choice_reward=0;original_event_resume(s);}
 if(s.phase==Phase::Combat)resolve_pending(s);return s.phase!=Phase::Error;
}
bool world_relic_potion(State& s,int slot){auto& w=s.world_relics;if(s.phase!=Phase::WorldRelicChoice||w.choice!=WorldRelicChoice::Potion||slot>=int(potion_slot_count(s)))return false;
 unsigned reward=w.choice_reward;Potion potion=Potion(w.offers[0]);w_clear_choice(s);
 if(slot>=0){if(potion_at(s,unsigned(slot))!=Potion::Empty)discard_potion(s,slot);potion_set(s,unsigned(slot),potion);if(reward&&reward<=w.reward_count)w.rewards[reward-1].flags|=1;}
 w_pump(s);++s.revision;if(!world_relic_pending(s)&&s.phase!=Phase::Error&&s.phase!=Phase::Dead)original_event_resume(s);return true;
}

void world_relic_modify_card_creation(State& s,WorldCardOptions& o){if(!s.world_relics.active||(o.flags&WorldNoCardPoolModifications)||!(o.flags&WorldIsCardReward))return;
 for(unsigned ri=0;ri<s.world_relics.count;ri++){const auto& r=s.world_relics.entries[ri];if(!w_present(r))continue;
  if(r.id==WorldRelicDingyRug){bool has=false;for(unsigned i=0;i<o.pool_count;i++)if(o.pools[i]==WorldCardPool::Colorless)has=true;if(!has&&o.pool_count<11)o.pools[o.pool_count++]=WorldCardPool::Colorless;}
  if(r.id==WorldRelicPrismaticGem){bool colored=false;for(unsigned i=0;i<o.pool_count;i++)if(o.pools[i]!=WorldCardPool::Colorless)colored=true;if(!colored)continue;
   WorldCardPool pools[11]={WorldCardPool::Ironclad,WorldCardPool::Silent,WorldCardPool::Regent,WorldCardPool::Necrobinder,WorldCardPool::Defect};unsigned n=5;
   for(unsigned i=0;i<o.pool_count;i++){auto p=o.pools[i]==WorldCardPool::Own?WorldCardPool(unsigned(s.character)):o.pools[i];bool seen=false;for(unsigned j=0;j<n;j++)if(pools[j]==p)seen=true;if(!seen&&n<11)pools[n++]=p;}
   o.pool_count=uint8_t(n);for(unsigned i=0;i<n;i++)o.pools[i]=pools[i];
  }
 }
}
namespace {
CardType w_detached_type(Card c,const EventCardFacts& f){return c.kind==EventCardMadScience?CardType(f.science_type):definitions[c.kind].type;}
struct WCandyFilter {const Card* cards;unsigned count;const WorldCardOptions* original;bool duplicates;};
bool w_candy_filter(const State& s,CardKind k,const void* pointer){const auto& c=*static_cast<const WCandyFilter*>(pointer);
 if(definitions[k].type!=CardType::Power)return false;
 if(c.original->predicate&&!c.original->predicate(s,k,c.original->predicate_data))return false;
 if(!c.duplicates)for(unsigned i=0;i<c.count;i++)if(c.cards[i].kind==k)return false;return true;
}
void w_candy(State& s,WorldRelicInstance& r,Card* cards,EventCardFacts* facts,unsigned& count,unsigned capacity,const WorldCardOptions& o){
 if(r.values[0]<=0||r.values[0]%2!=1||o.source!=WorldCardSource::Encounter||!(o.flags&WorldIsCardReward)||!(o.flags&WorldIsFromCombat))return;
 WCandyFilter f{cards,count,&o,false};WorldCardOptions extra=o;extra.source=WorldCardSource::Other;extra.flags=WorldNoModifyHooks|WorldNoCardPoolModifications;extra.type_mask=world_type_mask(CardType::Power);extra.predicate=w_candy_filter;extra.predicate_data=&f;
 CardKind candidates[CardKinds];unsigned n=world_card_candidates(s,extra,candidates,CardKinds);if(!n){f.duplicates=true;n=world_card_candidates(s,extra,candidates,CardKinds);}if(!n)return;
 if(count>=capacity){w_error(s);return;}unsigned made=1;if(!world_create_reward(s,world_rng(s,WorldRng::Rewards),1,extra,cards+count,facts+count,capacity-count,&made)){w_error(s);return;}count+=made;
}
void w_apply_reward_relic(State& s,WorldRelicInstance& r,Card* cards,EventCardFacts* facts,unsigned& count,unsigned capacity,const WorldCardOptions& o,bool callbacks){
 bool upgrade=false;int type=-1;EventEnchantment enchant=EventEnchantment::None;int amount=1;bool after=false;
 switch(r.id){
 case WorldRelicLastingCandy:w_candy(s,r,cards,facts,count,capacity,o);return;
 case WorldRelicFrozenEgg:if(o.flags&WorldNoHookUpgrades)return;upgrade=true;type=int(CardType::Power);break;
 case WorldRelicMoltenEgg:if(o.flags&WorldNoHookUpgrades)return;upgrade=true;type=int(CardType::Attack);break;
 case WorldRelicToxicEgg:if(o.flags&WorldNoHookUpgrades)return;upgrade=true;type=int(CardType::Skill);break;
 case WorldRelicFresnelLens:enchant=EventEnchantment::Nimble;amount=2;break;
 case WorldRelicGlitter:enchant=EventEnchantment::Glam;break;
 case WorldRelicLavaLamp:if(world_room_type(s)<1||world_room_type(s)>3||r.values[0])return;upgrade=true;break;
 case WorldRelicSilverCrucible:if(r.values[0]>=3||!(o.flags&WorldIsCardReward))return;upgrade=true;after=true;break;
 case WorldRelicSilkenTress:if(r.values[0]||!(o.flags&WorldIsCardReward))return;enchant=EventEnchantment::Glam;after=true;break;
 case WorldRelicWingCharm:{unsigned eligible[MaxWorldRelicOffers],n=0;for(unsigned i=0;i<count;i++)if(event_card_can_enchant(s,cards[i],facts[i],EventEnchantment::Swift))eligible[n++]=i;
  if(n){unsigned i=eligible[world_rng(s,WorldRng::Niche).below(n)];event_card_apply_enchantment(cards[i],facts[i],EventEnchantment::Swift,1);}return;}
 default:return;
 }
 for(unsigned i=0;i<count;i++){
  if(upgrade&&(type<0||int(w_detached_type(cards[i],facts[i]))==type)&&card_upgradeable(cards[i]))cards[i].upgraded=1;
  if(enchant!=EventEnchantment::None&&event_card_can_enchant(s,cards[i],facts[i],enchant))event_card_apply_enchantment(cards[i],facts[i],enchant,amount);
 }
 if(after&&callbacks){if(r.id==WorldRelicSilverCrucible)++r.values[0];else r.values[0]=1;}
}
}
void world_relic_modify_reward_options(State& s,Card* cards,EventCardFacts* facts,unsigned& count,unsigned capacity,const WorldCardOptions& o){if(!s.world_relics.active||(o.flags&WorldNoModifyHooks))return;
 // Normal hooks finish before the first Late hook, independent of ownership.
 for(unsigned i=0;i<s.world_relics.count;i++){auto& r=s.world_relics.entries[i];if(w_present(r)&&r.id==WorldRelicLastingCandy)w_apply_reward_relic(s,r,cards,facts,count,capacity,o,true);}
 for(unsigned i=0;i<s.world_relics.count;i++){auto& r=s.world_relics.entries[i];if(w_present(r)&&r.id!=WorldRelicLastingCandy)w_apply_reward_relic(s,r,cards,facts,count,capacity,o,true);}
}
bool world_relic_modify_deck_card(State& s,Card& c,EventCardFacts& f){if(c.kind>=CardKinds)return false;if(!s.world_relics.active)return true;
 for(unsigned i=0;i<s.world_relics.count;i++){const auto& r=s.world_relics.entries[i];if(!w_present(r))continue;CardType type=w_detached_type(c,f);
  if((r.id==WorldRelicFrozenEgg&&type==CardType::Power)||(r.id==WorldRelicMoltenEgg&&type==CardType::Attack)||(r.id==WorldRelicToxicEgg&&type==CardType::Skill)){if(card_upgradeable(c))c.upgraded=1;}
  if(r.id==WorldRelicFresnelLens&&event_card_can_enchant(s,c,f,EventEnchantment::Nimble))event_card_apply_enchantment(c,f,EventEnchantment::Nimble,2);
 }return true;
}
void world_relic_modify_merchant_cards(State& s,Card* cards,EventCardFacts* facts,unsigned count){for(unsigned i=0;i<count;i++)world_relic_modify_deck_card(s,cards[i],facts[i]);}
int world_relic_modify_upgrade_odds(State&,Card,int odds){return odds;}
int world_relic_modify_card_reward_count(State&,int count,const WorldCardOptions&){return count;}
void world_relic_before_reward_offered(State& s,bool cards){if(!cards)return;for(unsigned i=0;i<s.world_relics.count;i++){auto& r=s.world_relics.entries[i];if(r.id==WorldRelicLastingCandy&&w_present(r)){if(r.values[0]>0&&r.values[0]%2==1)world_relic_flash(s,r.id);++r.values[0];}}}

} // namespace spire
