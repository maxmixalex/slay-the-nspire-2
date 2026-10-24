#pragma once
#include "core.hpp"
#include "ui_assets.hpp"
namespace spire {
// Names follow the pinned power localization. Descriptions use {n} for the
// current amount instead of the original localization's illustrative numbers.
struct StatusDefinition { const char* name; const char* description; int image; };
inline constexpr StatusDefinition status_definitions[]={
 {"力量","攻击伤害修正 [gold]{n}[/gold]。",U_Strength},
 {"敏捷","从卡牌获得的格挡增加 [blue]{n}[/blue]。",U_Dexterity},
 {"活力","下一张攻击的每次命中额外造成 [gold]{n}[/gold] 点伤害。",U_Vigor},
 {"荆棘","受到攻击时反击 [gold]{n}[/gold] 点伤害。",U_Thorns},
 {"虚弱","攻击伤害降低 [blue]25%[/blue]，剩余 [gold]{n}[/gold] 回合。",U_Weak},
 {"易伤","受到的攻击伤害增加 [blue]50%[/blue]，剩余 [gold]{n}[/gold] 回合。",U_Vulnerable},
 {"中毒","自身回合开始时失去 [green]{n}[/green] 点生命，无视格挡；随后层数减一。",U_Poison},
 {"无惧疼痛","每消耗一张牌，获得 [blue]{n}[/blue] 点格挡。",U_FeelNoPain},
 {"黑暗之拥","每消耗一张牌，抽 [blue]{n}[/blue] 张牌。",U_DarkEmbrace},
 {"不可抽牌","本回合无法再抽牌。生成卡牌不属于抽牌。",U_NoDraw},
 {"不可格挡","无法从卡牌获得格挡，剩余 [gold]{n}[/gold] 个敌方回合。遗物、药水及能力触发仍可获得格挡。",U_NoBlock},
 {"毒雾","回合开始时，给予所有敌人 [green]{n}[/green] 层中毒。",U_NoxiousFumes},
 {"精准","小刀的攻击伤害修正 [gold]{n}[/gold]。",U_Accuracy},
 {"恶魔形态","回合开始时，获得 [gold]{n}[/gold] 点力量。",U_DemonForm},
 {"无尽刀刃","常规抽牌前，在手牌中生成 [gold]{n}[/gold] 张小刀。",U_InfiniteBlades},
 {"火焰屏障","本回合每次受到攻击后，反击 [gold]{n}[/gold] 点伤害；敌方回合结束时移除。",U_FlameBarrier},
 {"下回合格挡","下回合清除格挡后，获得 [blue]{n}[/blue] 点格挡。",U_BlockNextTurn},
 {"下回合抽牌","下回合额外抽 [blue]{n}[/blue] 张牌。",U_DrawNextTurn},
#include "extended_status_definitions.inc"
#include "character_status_definitions.inc"
#include "five_character_status_definitions.inc"
};
constexpr int LegacyStatusKinds=sizeof(status_definitions)/sizeof(status_definitions[0]);
constexpr int EnemyStatusStart=LegacyStatusKinds,EventCardStatusStart=EnemyStatusStart+EnemyPowerCount;
constexpr int StatusKinds=EventCardStatusStart+EventCardPowerKinds;
constexpr int IroncladStatusStart=47,SilentStatusStart=IroncladStatusStart+IPowerCount;
constexpr int CharacterExtraStatusStart=SilentStatusStart+SPowerCount-2;
constexpr int DefectStatusStart=CharacterExtraStatusStart+6,RegentStatusStart=DefectStatusStart+DPowerCount;
constexpr int NecroStatusStart=RegentStatusStart+RPowerCount,FiveCharacterExtraStatusStart=NecroStatusStart+NPowerCount;
static_assert(IPowerCount==18&&SPowerCount==22&&DPowerCount==24&&RPowerCount==22&&NPowerCount==22&&LegacyStatusKinds==FiveCharacterExtraStatusStart+5,"Keep power and status directories aligned");
inline int status_amount(const State&s,int actor,int id){
    if(id<0||id>=StatusKinds||actor<0||actor>s.enemy_count)return 0;
    if(id>=EventCardStatusStart)return actor==0?s.event_cards.powers[id-EventCardStatusStart]:0;
    if(id>=EnemyStatusStart){
        if(!s.enemy_world.active||actor&&s.enemies[actor-1].hp<=0)return 0;EnemyPower power=EnemyPower(id-EnemyStatusStart);
        if(actor){if(power==EnemyPower::Strength||power==EnemyPower::Weak||power==EnemyPower::Vulnerable||power==EnemyPower::Artifact||power==EnemyPower::Debilitate)return 0;}
        else if(power==EnemyPower::Strength||power==EnemyPower::Dexterity||power==EnemyPower::Thorns||power==EnemyPower::Vigor||power==EnemyPower::Weak||power==EnemyPower::Vulnerable||power==EnemyPower::Plating||power==EnemyPower::Ritual||power==EnemyPower::Intangible)return 0;
        return enemy_power_value(s,actor-1,power);
    }
    if(actor){const auto&e=s.enemies[actor-1];if(!e.hp)return 0;
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
inline int collect_statuses(const State&s,int actor,int* out){
    int count=0;for(int i=0;i<StatusKinds;i++){
        // NStarCounter is a combat resource control, never a Power icon.
        // Keep status_amount(44) for deferred resource-value snapshots.
        if(i==44)continue;
        const bool resource=actor==0&&((i==45&&(s.character==Character::Necrobinder||s.expansion.osty_max_hp>0))||(i==46&&s.expansion.orb_slots>0));
        if(resource||status_amount(s,actor,i)!=0){if(out)out[count]=i;++count;}
    }return count;
}
}
