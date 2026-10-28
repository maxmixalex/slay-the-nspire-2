#pragma once
#include "core.hpp"
#include "power_values.hpp"
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
static_assert(sizeof(status_definitions)/sizeof(status_definitions[0])==LegacyStatusKinds,"Keep the power model and visual directories aligned");
inline int status_amount(const State&s,int actor,int id){return power_amount(s,actor,id);}
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
