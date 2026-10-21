#pragma once
#include "app.hpp"
#include "runtime_performance.hpp"
namespace spire {
// Match App::render's opaque-screen and modal precedence on every host.
inline RuntimeScene runtime_scene(const App&app){
    if(app.map_preview.open)return RuntimeScene::Map;
    if(app.library)return app.detail_card>=0||app.detail_item>=0?RuntimeScene::Overlay:RuntimeScene::Collection;
    if(app.character_select)return RuntimeScene::Character;
    if(app.paused||app.help||app.settings_open||app.frontend_confirm||app.detail_card>=0||app.detail_item>=0||app.status_actor>=0||app.event_reader||app.deck_candidate>=0||(app.panel&&!(app.panel==3&&app.map_window.visible)))return RuntimeScene::Overlay;
    if(app.title)return RuntimeScene::Title;
    if(app.map_window.visible||app.game.phase==Phase::Map)return RuntimeScene::Map;
    if(combat_phase(app.game.phase))return RuntimeScene::Combat;
    return RuntimeScene::Room;
}
}
