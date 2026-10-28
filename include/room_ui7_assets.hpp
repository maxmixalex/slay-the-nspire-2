#pragma once
#include "ui6_assets.hpp"
namespace spire {
constexpr unsigned RoomUi7ContentVersion=24;
enum RoomUi7Asset {
 I7_MerchantHandStart=Ui6AuxiliaryCount,
 I7_MerchantHandLast=I7_MerchantHandStart+46,
 I7_FakeMerchantHandStart,
 I7_FakeMerchantHandLast=I7_FakeMerchantHandStart+46,
 I7_MerchantDialogue,
 I7_MerchantRemoval01,
 I7_MerchantRemoval02,
 I7_MerchantRemoval04,
 I7_AncientDialogue,
 I7_AncientDialogueTail,
 I7_AncientSpeakerStart,
 I7_AncientSpeakerLast=I7_AncientSpeakerStart+7,
 RoomUi7AuxiliaryCount
};
constexpr unsigned MerchantHandSamples=47;
constexpr int MerchantHandPivotX=160,MerchantHandPivotY=230;
static_assert(I7_MerchantHandStart==1320&&RoomUi7AuxiliaryCount==1428,"UI7 room asset order");
}
