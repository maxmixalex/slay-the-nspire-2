#pragma once
#include "core.hpp"
namespace spire {
inline int power_amount(const State&s,int actor,int id,bool include_dead=false){
    if(id<0||id>=StatusKinds||actor<0||actor>s.enemy_count)return 0;
    if(id>=EventCardStatusStart)return actor==0?s.event_cards.powers[id-EventCardStatusStart]:0;
    if(id>=EnemyStatusStart){
        if(!s.enemy_world.active||actor&&s.enemies[actor-1].hp<=0&&!include_dead)return 0;EnemyPower power=EnemyPower(id-EnemyStatusStart);
        if(actor){if(power==EnemyPower::Strength||power==EnemyPower::Weak||power==EnemyPower::Vulnerable||power==EnemyPower::Artifact||power==EnemyPower::Debilitate)return 0;}
        else if(power==EnemyPower::Strength||power==EnemyPower::Dexterity||power==EnemyPower::Thorns||power==EnemyPower::Vigor||power==EnemyPower::Weak||power==EnemyPower::Vulnerable||power==EnemyPower::Plating||power==EnemyPower::Ritual||power==EnemyPower::Intangible)return 0;
        return enemy_power_value(s,actor-1,power);
    }
    if(actor){const auto&e=s.enemies[actor-1];if(!e.hp&&!include_dead)return 0;
        if(id==CharacterExtraStatusStart+1)return s.silent.strangle[actor-1];
        if(id==CharacterExtraStatusStart+2)return s.silent.enemy_artifact[actor-1];
        if(id==CharacterExtraStatusStart+3)return s.expansion.restore_strength[actor-1];
        if(id==40)return s.expansion.demise[actor-1];
        if(id==41)return s.expansion.beetle[actor-1];
        if(id==42)return s.expansion.doom[actor-1];
        if(id==FiveCharacterExtraStatusStart)return s.regent.conqueror[actor-1];
        if(id==FiveCharacterExtraStatusStart+1)return s.necro.debilitate[actor-1];
        if(id==FiveCharacterExtraStatusStart+2)return s.necro.hang[actor-1];
        if(id==FiveCharacterExtraStatusStart+3)return s.necro.oblivion[actor-1];
        if(id==FiveCharacterExtraStatusStart+4)return s.necro.sic_em[actor-1];
        switch(id){case 0:return e.strength;case 4:return e.weak;case 5:return e.vulnerable;case 6:return e.poison;default:return 0;}}
    const int values[]={s.strength,s.dexterity,s.vigor,s.thorns,s.enemy_world.active?enemy_power_value(s,-1,EnemyPower::Weak):0,s.enemy_world.active?enemy_power_value(s,-1,EnemyPower::Vulnerable):0,0,s.pain,s.embrace,
        s.no_draw,s.no_block,s.noxious_fumes,s.accuracy,s.demon_form,s.infinite_blades,
        s.flame_barrier,s.block_next_turn,s.draw_next_turn};
    if(id<18)return values[id];
    if(id<40)return s.expansion.powers[id-18];
    if(id==42)return s.necro.player_doom;
    if(id==43){int n=0;for(auto turns:s.expansion.bomb_turns)n+=turns!=0;return n;}
    if(id==44)return s.expansion.stars;
    if(id==45)return s.expansion.osty_hp;
    if(id==46)return s.expansion.orb_count;
    if(id>=IroncladStatusStart&&id<SilentStatusStart)return s.ironclad.powers[id-IroncladStatusStart];
    if(id>=SilentStatusStart&&id<CharacterExtraStatusStart)return s.silent.powers[id-SilentStatusStart];
    if(id==CharacterExtraStatusStart){int n=0;for(auto active:s.silent.nightmare_active)n+=active!=0;return n;}
    if(id==CharacterExtraStatusStart+4)return s.ironclad.setup_strength;
    if(id==CharacterExtraStatusStart+5)return s.silent.extra_rewards;
    if(id>=DefectStatusStart&&id<RegentStatusStart)return s.defect.powers[id-DefectStatusStart];
    if(id>=RegentStatusStart&&id<NecroStatusStart)return s.regent.powers[id-RegentStatusStart];
    if(id>=NecroStatusStart&&id<FiveCharacterExtraStatusStart)return s.necro.powers[id-NecroStatusStart];
    return 0;
}
constexpr bool power_instanced(unsigned id)noexcept{
 return id==18+unsigned(XPower::Automation)||id==18+unsigned(XPower::Panache)||
  id==18+unsigned(XPower::RollingBoulder)||id==43||id==CharacterExtraStatusStart||
  id==RegentStatusStart+unsigned(RPower::Monologue)||id==RegentStatusStart+unsigned(RPower::Orbit);
}
constexpr unsigned power_instance_family(unsigned id)noexcept{
 return id==18+unsigned(XPower::Automation)?0:id==18+unsigned(XPower::Panache)?1:
  id==18+unsigned(XPower::RollingBoulder)?2:id==43?3:id==CharacterExtraStatusStart?4:
  id==RegentStatusStart+unsigned(RPower::Monologue)?5:6;
}
constexpr uint16_t power_token(unsigned actor,unsigned id,unsigned instance=0)noexcept{return uint16_t((actor<<12)|(instance<<8)|id);}
constexpr unsigned power_actor(uint16_t token)noexcept{return token>>12;}
constexpr unsigned power_id(uint16_t token)noexcept{return token&255u;}
constexpr unsigned power_instance(uint16_t token)noexcept{return (token>>8)&15u;}
inline int power_instance_amount(const State&s,unsigned id,unsigned instance){
 if(instance>=MaxPowerInstances)return 0;
 if(id==18+unsigned(XPower::Automation))return s.expansion.automation[instance];
 if(id==18+unsigned(XPower::Panache))return s.expansion.panache[instance];
 if(id==18+unsigned(XPower::RollingBoulder))return s.expansion.boulder[instance];
 if(id==43)return s.expansion.bomb_turns[instance];
 if(id==CharacterExtraStatusStart)return s.silent.nightmare_active[instance]?3:0;
 if(id==RegentStatusStart+unsigned(RPower::Monologue))return s.regent.monologue[instance];
 if(id==RegentStatusStart+unsigned(RPower::Orbit))return s.regent.orbit[instance];
 return 0;
}
inline bool power_token_present(const State&s,uint16_t token){
 const unsigned actor=power_actor(token),id=power_id(token),instance=power_instance(token);
 if(actor>unsigned(s.enemy_count)||id>=StatusKinds||id==44||id==45||id==46)return false;
 if(power_instanced(id))return actor==0&&power_instance_amount(s,id,instance)!=0;
 return instance==0&&power_amount(s,int(actor),int(id),true)!=0;
}
constexpr unsigned silent_power_id(unsigned power)noexcept{
 return power==unsigned(SPower::InfiniteBlades)?14:power==unsigned(SPower::NoxiousFumes)?11:
  power<SPowerCount?SilentStatusStart+power:CharacterExtraStatusStart;
}
constexpr unsigned enemy_power_id(unsigned actor,EnemyPower power)noexcept{
 if(power==EnemyPower::Strength)return 0;if(power==EnemyPower::Weak)return 4;if(power==EnemyPower::Vulnerable)return 5;
 if(actor){if(power==EnemyPower::Artifact)return CharacterExtraStatusStart+2;if(power==EnemyPower::Debilitate)return FiveCharacterExtraStatusStart+1;}
 else{switch(power){case EnemyPower::Dexterity:return 1;case EnemyPower::Thorns:return 3;case EnemyPower::Vigor:return 2;
  case EnemyPower::Plating:return 18+unsigned(XPower::Plating);case EnemyPower::Ritual:return 18+unsigned(XPower::Ritual);
  case EnemyPower::Intangible:return 18+unsigned(XPower::Intangible);default:break;}}
 return EnemyStatusStart+unsigned(power);
}
constexpr unsigned debuff_power_id(NDebuff power)noexcept{
 switch(power){case NDebuff::Weak:return 4;case NDebuff::Vulnerable:return 5;case NDebuff::Poison:return 6;
  case NDebuff::Strength:return 0;case NDebuff::Beetle:return 41;case NDebuff::Demise:return 40;case NDebuff::Doom:return 42;
  case NDebuff::Strangle:return CharacterExtraStatusStart+1;case NDebuff::Debilitate:return FiveCharacterExtraStatusStart+1;
  case NDebuff::Hang:return FiveCharacterExtraStatusStart+2;case NDebuff::Oblivion:return FiveCharacterExtraStatusStart+3;
  case NDebuff::SicEm:return FiveCharacterExtraStatusStart+4;case NDebuff::Conqueror:return FiveCharacterExtraStatusStart;
  default:return CharacterExtraStatusStart+3;
 }
}
} // namespace spire
