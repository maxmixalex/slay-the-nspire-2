#pragma once
#include "speech_bubble_assets.hpp"
namespace spire {
constexpr unsigned HistoryBodyContentVersion=39;
enum HistoryBodyUiAsset {
 I10_HistoryRoomFirst=HistoryHeaderAuxiliaryCount,
 HistoryBodyAuxiliaryCount=I10_HistoryRoomFirst+64
};
// ImageHelper chooses the actual room, including unknown-room variants and
// authored boss/ancient portraits. Each entry is followed by its own outline.
inline constexpr const char*HistoryRoomKeys[]={
 "AEONGLASS_BOSS","ANCIENT","CEREMONIAL_BEAST_BOSS","DARV","DOORMAKER_BOSS","ELITE","EVENT",
 "KAISER_CRAB_BOSS","KNOWLEDGE_DEMON_BOSS","LAGAVULIN_MATRIARCH_BOSS","MONSTER","NEOW",
 "NONUPEIPE","OROBAS","PAEL","QUEEN_BOSS","REST_SITE","SHOP","SOUL_FYSH_BOSS","TANX",
 "TEST_SUBJECT_BOSS","TEZCATARA","THE_INSATIABLE_BOSS","THE_KIN_BOSS","TREASURE",
 "UNKNOWN_ELITE","UNKNOWN_MONSTER","UNKNOWN_SHOP","UNKNOWN_TREASURE","VAKUU","VANTOM_BOSS","WATERFALL_GIANT_BOSS"
};
static_assert(I10_HistoryRoomFirst==2068&&HistoryBodyAuxiliaryCount==2132,
 "History body appends64 original images after the v38 reminder lock");
static_assert(sizeof(HistoryRoomKeys)/sizeof(*HistoryRoomKeys)==32,"History image pairs and keys agree");
}
