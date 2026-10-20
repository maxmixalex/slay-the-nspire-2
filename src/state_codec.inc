// Versioned little-endian field encoding, not a raw C++ object image.
struct Codec{
    uint8_t* out; const uint8_t* in; size_t at,limit; bool good;
    uint32_t field(uint32_t v){if(at+4>limit){good=false;return 0;}uint32_t r=0;for(int i=0;i<4;i++){if(out)out[at+i]=(uint8_t)(v>>(8*i));else r|=(uint32_t)in[at+i]<<(8*i);}at+=4;return out?v:r;}
    uint32_t compact(uint32_t v,unsigned count){if(at>limit||count>limit-at){good=false;return 0;}uint32_t r=0;for(unsigned i=0;i<count;++i){if(out)out[at+i]=(uint8_t)(v>>(8*i));else r|=(uint32_t)in[at+i]<<(8*i);}at+=count;return out?v:r;}
};
static uint32_t checksum(const uint8_t*p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;i++){h^=p[i];h*=16777619u;}return h;}
template<class T> static void field(T& v,Codec&c){uint32_t raw=c.field((uint32_t)v);v=static_cast<T>(raw);if(c.in&&(uint32_t)v!=raw)c.good=false;}
// All historical fields occupy four bytes, even after a C++ field widens.
// Relic masks keep their low word at the old offset and append their high word
// in v14. Encoding the low word must never truncate the in-memory copy.
static void field(RelicMask&v,Codec&c){uint32_t raw=c.field((uint32_t)v);if(c.in)v=(v&0xffffffff00000000ull)|raw;}
static void field_high(RelicMask&v,Codec&c){uint32_t raw=c.field((uint32_t)(v>>32));if(c.in)v=(v&0xffffffffull)|(RelicMask(raw)<<32);}
static void field64(uint64_t&v,Codec&c){field(v,c);field_high(v,c);}
template<unsigned Bytes,class T> static void compact_field(T&v,Codec&c){
    static_assert(sizeof(T)<=Bytes,"Compact fields use their declared integer width");
    constexpr uint32_t mask=Bytes==1?255u:65535u;
    uint32_t raw=c.compact((uint32_t)v,Bytes);if(c.in){v=static_cast<T>(raw);if(((uint32_t)v&mask)!=raw)c.good=false;}
}
static void code_state(State&s,Codec&c,unsigned version){
#define F(x) field(x,c)
 F(s.phase);F(s.practice);F(s.floor);F(s.enemy_count);F(s.hp);F(s.max_hp);F(s.energy);F(s.block);F(s.strength);F(s.pain);F(s.embrace);F(s.turn);F(s.power_count);
 if(s.enemy_count>(version>=15?MaxEnemies:LegacyEnemies)){c.good=false;return;}
 for(int i=0;i<2;i++)F(s.power_order[i]);F(s.rng_cards);F(s.rng_world);F(s.seed);F(s.revision);F(s.deck_count);
 if(s.deck_count>MaxDeck){c.good=false;return;}for(int i=0;i<s.deck_count;i++){F(s.deck[i].kind);F(s.deck[i].upgraded);}
 F(s.card_count);if(s.card_count>MaxCards){c.good=false;return;}for(int i=0;i<s.card_count;i++){F(s.cards[i].kind);F(s.cards[i].upgraded);F(s.zones[i]);}
 Pile*ps[]={&s.draw,&s.hand,&s.discard,&s.exhaust,&s.powers};for(int z=0;z<5;z++){auto&p=*ps[z];F(p.count);if(p.count>MaxCards){c.good=false;return;}for(int i=0;i<p.count;i++)F(p.ids[i]);}
 for(int t=0;t<LegacyEnemies;t++){auto&e=s.enemies[t];F(e.hp);F(e.max_hp);F(e.block);F(e.strength);F(e.vulnerable);F(e.move);}
 F(s.effect_count);if(s.effect_count>MaxEffects){c.good=false;return;}for(int i=0;i<s.effect_count;i++){auto&e=s.effects[i];F(e.kind);F(e.value);F(e.target);}
 F(s.playing);
 F(s.relic_count);for(int i=0;i<10;i++)F(s.relic_order[i]);for(int i=0;i<10;i++)F(s.relic_counters[i]);
 F(s.dexterity);F(s.double_attack);F(s.relic_flash);F(s.relic_revision);
 F(s.current_node);F(s.next_row);F(s.encounter);F(s.relics);F(s.removals);F(s.gold);F(s.last_gold);
 for(auto&n:s.map){F(n.room);F(n.links);F(n.visited);F(n.event);}
 for(unsigned p=0;p<LegacyPotionSlots;++p)F(s.potions[p]);for(int i=0;i<4;i++)F(s.shop_cards[i]);F(s.shop_relics[0]);F(s.shop_potions[0]);F(s.shop_sold);
 const int legacy_prices[]={0,1,2,3,7,10};for(int i:legacy_prices)F(s.shop_prices[i]);
 F(s.event_id);F(s.event_page);F(s.event_count);F(s.event_remaining);F(s.event_value);F(s.event_first);
 for(int i=0;i<3;i++)F(s.reward[i]);F(s.log_count);if(s.log_count>24){c.good=false;return;}for(int i=0;i<s.log_count;i++){auto&l=s.logs[i];F(l.kind);F(l.a);F(l.b);}
if(version>=4){F(s.no_draw);for(int t=0;t<LegacyEnemies;t++)F(s.enemies[t].weak);}
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
  for(int t=0;t<LegacyEnemies;t++)F(s.enemies[t].poison);
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
 if(version>=10){F(s.event_deck_action);}
 else if(c.in){
  if((unsigned)s.event_id>=3)c.good=false;
  for(const auto&node:s.map)if(node.event>=3)c.good=false;
 }
#include "expansion_codec.inc"
 if(version>=13){
#include "ironclad_codec.inc"
#include "silent_codec.inc"
  static_assert(MaxPowerInstances+6==22,"v13 post-play listener width is frozen");
  for(int n=0;n<22;n++)F(s.expansion.after_play_order[n]);
 }else if(c.in){
  // Old formats cannot carry the newly appended models, module operations or
  // selections. Keep their original directory/capacity limits explicit.
  if(s.deck_count>128)c.good=false;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=114)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=114)c.good=false;
  for(auto k:s.shop_cards)if(k>=114)c.good=false;
  for(auto k:s.reward)if(k>=114)c.good=false;
  for(auto card:s.expansion.offers)if(card.kind>=114)c.good=false;
  if((unsigned)s.expansion.choice>(unsigned)XChoice::Transform)c.good=false;
  for(unsigned i=0;i<s.effect_count;i++){
   const auto&e=s.effects[i];
   if((e.kind==EffectKind::Extended&&e.target>(int)XOp::TurnRelics)||
      (e.kind!=EffectKind::Extended&&e.aux))c.good=false;
  }
  // The historical writers had no character history. Each suspended Attack
  // is nevertheless visibly in progress and must be counted as started
  // before its continuation records a finished play in the new module.
  auto pending_attack=[&](int id){
   if(id>=0&&id<s.card_count&&s.cards[id].kind<CardKinds&&card_type(s,s.cards[id],id)==CardType::Attack)
    ++s.ironclad.attacks_started;
  };
  pending_attack(s.playing);for(unsigned i=0;i<s.expansion.depth;i++)pending_attack(s.expansion.frames[i].playing);
  // Old engines dispatched Panache before Calamity. Preserve that explicit
  // migration policy; the original acquisition history cannot be recovered.
  for(unsigned i=0;i<MaxPowerInstances;i++)if(s.expansion.panache[i])after_play_register(s,(unsigned)PostPlayActor::PanacheStart+i);
  if(power_value(s,XPower::Calamity))after_play_register(s,(unsigned)PostPlayActor::Calamity);
 }
 if(version>=14){
#include "defect_codec.inc"
#include "regent_codec.inc"
#include "necro_codec.inc"
  static_assert(MaxPostPlayPowers>=52&&RelicKinds==33,"v14 shared dimensions are frozen");
  for(int n=22;n<52;n++)F(s.expansion.after_play_order[n]);
  field_high(s.relics,c);field_high(s.relic_flash,c);
  for(auto&mask:s.shop_relics)field_high(mask,c);field_high(s.treasure_relic,c);
  for(int n=30;n<33;n++){F(s.relic_order[n]);F(s.relic_counters[n]);}
  for(int slot=0;slot<16;slot++){
   auto&n=s.silent.nightmares[slot];
   F(n.defect_value);F(n.defect_cost_delta);F(n.defect_turn_origin);F(n.defect_combat_origin);
   F(n.regent_kick_discount);F(n.regent_kick_turn_origin);F(n.regent_kick_combat_origin);F(n.regent_punch_damage);
   F(n.necro_ethereal);F(n.necro_flatten_zero);F(n.necro_cost_delta);F(n.necro_turn_origin);F(n.necro_combat_origin);F(n.necro_flatten_origin);F(n.necro_scythe_value);
   F(n.star_turn_free);F(n.star_combat_free);
  }
  for(int id=0;id<512;id++){F(s.expansion.star_turn_free[id]);F(s.expansion.star_combat_free[id]);}
 }else if(c.in){
  // The v13 writer could express neither new models nor new characters,
  // operations, relics or listener actors. A widened C++ enum must not make
  // a forged historical payload acceptable under a newer directory.
  constexpr unsigned old_models=228,old_relics=30;
  constexpr RelicMask old_mask=(RelicMask(1)<<old_relics)-1;
  if((unsigned)s.character>(unsigned)Character::Silent||s.relic_count>old_relics||
     (s.relics&~old_mask)||(s.relic_flash&~old_mask)||(s.treasure_relic&~old_mask)||
     s.strength<0||s.temporary_strength>s.strength||s.expansion.powers[(unsigned)XPower::Focus]<0||s.expansion.x>999)c.good=false;
  for(auto mask:s.shop_relics)if(mask&~old_mask)c.good=false;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=old_models)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=old_models)c.good=false;
  for(auto k:s.shop_cards)if(k>=old_models)c.good=false;
  for(auto k:s.reward)if(k>=old_models)c.good=false;
  for(auto card:s.expansion.offers)if(card.kind>=old_models)c.good=false;
  for(const auto&n:s.silent.nightmares)if(n.card.kind>=old_models)c.good=false;
  if((unsigned)s.expansion.choice>(unsigned)XChoice::NightmareCard)c.good=false;
  for(unsigned actor:s.expansion.after_play_order)if(actor>=(unsigned)PostPlayActor::Relics)c.good=false;
  for(unsigned i=0;i<s.effect_count;i++){
   const auto&e=s.effects[i];
   if(e.kind==EffectKind::Extended){
    if(e.target>(int)XOp::PostPlayListener)c.good=false;
    if(e.target==(int)XOp::PostPlayListener&&e.value>=(int)PostPlayActor::DefectStorm)c.good=false;
    if(e.target==(int)XOp::Create&&(e.aux<0||e.aux>=512||card_from_token(e.aux).kind>=old_models))c.good=false;
   }else if(e.kind==EffectKind::Damage&&e.aux)c.good=false;
  }
  for(unsigned i=0;i<s.enemy_count;i++)if(s.enemies[i].max_hp<42||s.enemies[i].max_hp>46)c.good=false;
  for(unsigned i=0;i<s.log_count;i++)if((unsigned)s.logs[i].kind>(unsigned)LogKind::PoisonDamage)c.good=false;
  if(s.playing>=0&&(s.playing>=s.card_count||s.zones[s.playing]!=Zone::Limbo))c.good=false;
  for(unsigned i=0;i<s.expansion.depth;i++){const auto&f=s.expansion.frames[i];int id=f.playing;if(f.x>999||(id>=0&&(id>=s.card_count||s.zones[id]!=Zone::Limbo)))c.good=false;}
  // The old dark-orb representation is already DOrb::Dark=0. Existing dead
  // creatures and pet presence are observable; they are the migration
  // baseline, so a newly generated Melancholy cannot count an old death again.
  s.necro.osty_was_alive=(uint8_t)(s.expansion.osty_hp>0);
  for(unsigned t=0;t<s.enemy_count;t++)if(!s.enemies[t].hp)s.necro.enemy_dead_mask|=(uint8_t)(1u<<t);
  // Envenom is the only historical player damage listener in this new shared
  // directory. Its old cross-hook position was never stored; append it after
  // the preserved active prefix, before powers acquired after migration.
  // Register only after rejecting impossible actors from the old payload.
  if(spower(s,SPower::Envenom))after_play_register(s,(unsigned)PostPlayActor::SilentEnvenom);
  // The historical BulletTime representation incorrectly lasted until side
  // end. Its original command uses SetThisTurnOrUntilPlayed. Check the old
  // fields before normalizing them, so malformed flags/origins cannot hide.
  if(!valid_silent_state(s))c.good=false;
  if(c.good){
   for(unsigned id=0;id<s.card_count;id++)if(s.silent.bullet_free[id]){
    s.expansion.star_turn_free[id]=1;
    if(!s.expansion.turn_cost[id]){
     int origin=s.silent.bullet_origin[id];s.expansion.turn_cost[id]=1;temporary_cost_set(s,id);
     // Silent's captured boundary is known. Stomp's cross-module boundary
     // was absent, so its visible current discount is the migration baseline.
     s.silent.turn_origin[id]=(uint16_t)origin;
    }
    s.silent.bullet_free[id]=s.silent.bullet_origin[id]=0;
   }
   for(auto&n:s.silent.nightmares)if(n.bullet_free){
    n.star_turn_free=1;
    if(!n.turn_cost){
     n.turn_cost=1;n.turn_origin=n.bullet_origin;n.stomp_turn_origin=n.stomp_discount;
     n.defect_turn_origin=n.defect_cost_delta;n.regent_kick_turn_origin=n.regent_kick_discount;n.necro_turn_origin=n.necro_cost_delta;
    }
    n.bullet_free=n.bullet_origin=0;
   }
  }
 }
 if(version>=15){
#define B(x) compact_field<1>(x,c)
#define H(x) compact_field<2>(x,c)
#include "world_codec.inc"
#include "world_run_codec.inc"
#include "event_card_codec.inc"
#include "original_event_codec.inc"
#include "enemy_codec.inc"
#include "world_relic_codec.inc"
  F(s.world_factory.rarity_offset_bits);B(s.world_factory.initialized);
  // New compact suffixes end on a canonical four-byte boundary, preserving
  // the historical checksum envelope without altering any old field offset.
  while(c.at%4&&c.good)if(c.compact(0,1)!=0&&c.in)c.good=false;
#undef H
#undef B
 }else if(c.in){
  // Operand groups in genuine old saves used a two-enemy stride. Decode their
  // original range before translating the queued continuation to eight slots.
  if((unsigned)s.phase>(unsigned)Phase::ExtendedChoice)c.good=false;
  for(unsigned i=0;i<s.deck_count;i++)if(s.deck[i].kind>=497)c.good=false;
  for(unsigned i=0;i<s.card_count;i++)if(s.cards[i].kind>=497)c.good=false;
  for(unsigned i=0;i<s.effect_count;i++)if(s.effects[i].kind==EffectKind::Extended){
   auto&e=s.effects[i];auto op=(XOp)e.target;
   if(op==XOp::NTempStrength||op==XOp::NEnemyPower||op==XOp::NApplyCopiedDebuff){
    int groups=op==XOp::NTempStrength?NTempStrengthCount:op==XOp::NEnemyPower?5:NDebuffCount;
    if(e.value<0||e.value>=groups*LegacyEnemies)c.good=false;
    else e.value=(int16_t)((e.value/LegacyEnemies)*MaxEnemies+e.value%LegacyEnemies);
   }else if(op==XOp::SKnifeShiv){
    if(e.aux<0||e.aux>=2*LegacyEnemies)c.good=false;
    else e.aux=(int16_t)((e.aux/LegacyEnemies)*MaxEnemies+e.aux%LegacyEnemies);
   }
   if(e.target>(int)XOp::AllCharactersBeetle)c.good=false;
  }
  card_result_migrate_order(s);
 }
#undef F
}
bool save(const State&s,SaveData&out){
    if(!validate(s)||s.phase==Phase::Error)return false;
    State copy=s;Codec c{out.bytes,nullptr,0,sizeof(out.bytes),true};c.field(0x3156534E);c.field(SaveVersion);c.field(0x41CEF1EA);code_state(copy,c,SaveVersion);
    uint32_t h=checksum(out.bytes,c.at);c.field(h);out.size=c.at;return c.good;
}
bool load(State&s,const uint8_t*data,size_t len){
    if(!data||len<16||len>sizeof(SaveData::bytes)||len%4)return false;
    Codec c{nullptr,data,0,len-4,true};if(c.field(0)!=0x3156534E)return false;
    const unsigned version=c.field(0);
    if((version<3||version>SaveVersion)||c.field(0)!=0x41CEF1EA)return false;
    State tmp{};code_state(tmp,c,version);if(!c.good||c.at!=len-4||!validate(tmp)||tmp.phase==Phase::Error)return false;
    uint32_t h=checksum(data,len-4),stored=0;for(int i=0;i<4;i++)stored|=(uint32_t)data[len-4+i]<<(8*i);if(h!=stored)return false;
    s=tmp;return true;
}
