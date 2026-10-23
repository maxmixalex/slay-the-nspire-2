#pragma once

// Original main-menu logo pose, cropped from the 1920x1080 scene to 320x240.
namespace spire::title_scene {
constexpr int AtlasWidth=164,AtlasHeight=106;
constexpr int TextX=40,TextY=48,TextWidth=164,TextHeight=84;
constexpr int FireX=106,FireY=66,FireWidth=16,FireHeight=22;
constexpr int FireAtlasY=84,FireFrames=10,FireFrameMs=100;
constexpr int MenuCenter=119,MenuLabelWidth=164,MenuHitWidth=174;
constexpr int menu_top(int rows){return rows==5?137:147;}
constexpr int MenuRowHeight=20;
}
