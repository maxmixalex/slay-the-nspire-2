#pragma once
#include <stdint.h>
namespace spire {
// Observations of committed commands. These never enter State, command logs,
// random streams or save compatibility; consumers may omit this callback.
enum class RunHistoryEvent:uint8_t {
 None,CardObtained,CardRemoved,CardUpgraded,CardDowngraded,CardTransformed,CardEnchanted,
 CardPicked,CardSkipped,CardBought,RelicObtained,RelicRemoved,RelicSkipped,RelicBought,
 PotionObtained,PotionUsed,PotionDiscarded,PotionSkipped,PotionBought,
 EventChoice,AncientPicked,AncientSkipped,ModifierChoice,RestChoice,
 GoldSpent,GoldLost,GoldStolen,DamageTaken,Healed,MaxHpGained,MaxHpLost,QuestCompleted,FurCoat,CombatStarted,Count
};
enum class HistoryChoiceVariable:uint8_t {None,Gold,Relic,Potion,PotionRarity};
struct RunHistoryNotice {
 RunHistoryEvent kind=RunHistoryEvent::None;
 uint16_t model=0,detail=0;
 uint8_t flags=0;
 const char* key=nullptr; // A transient stable localization key, never stored as a pointer.
};
}
