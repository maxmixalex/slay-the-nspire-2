#pragma once
#include <stdint.h>
namespace spire::game_over_ui {
// Pinned game_over_screen, score_line, badge and game_over_button scenes,
// uniformly projected from the 4:3 1680x1260 viewport to 320x240.
constexpr int BannerX=98,BannerWidth=125,BannerHeight=31;
constexpr int BannerInitialY=56,BannerSummaryY=50;
constexpr int ButtonY=207,ButtonWidth=50,ButtonHeight=12;
// NReturnToMainMenuButton._Ready moves the scene's button -140 source
// pixels and records that position again. It is centered while View Run is hidden.
constexpr int ContinueX=135,ViewRunX=111,MainMenuX=133;
constexpr bool ViewRunEnabled=false; // Pinned _Ready disables it; no Enable call exists.
constexpr int ScoreX=110,ScoreWidth=100,ScoreHeight=9,ScoreStep=7;
constexpr int BadgeSize=13;
constexpr int ViewRunText=8687,PartialHistoryText=8688;
struct BadgeText {uint16_t first;uint8_t tiers;};
constexpr BadgeText BadgeTexts[]={
 {8654,1},{8655,1},{8656,1},{8657,3},{8660,1},{8661,3},
 {8664,1},{8665,1},{8666,3},{8669,1},{8670,1},{8671,3},
 {8674,1},{8675,3},{8678,1},{8679,1},{8680,3},{8683,1},{8684,3}
};
}
