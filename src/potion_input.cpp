#include "app.hpp"
namespace spire {
void App::close_potion_popup(){potion_popup=-1;potion_targeting=false;popup_bottle=Potion::Empty;pointer_cancel();}
void App::open_potion_popup(int slot){
 if(title||paused||help||panel||library||character_select||map_preview.open||event_reader||status_actor>=0||detail_card>=0||detail_item>=0||deck_candidate>=0||choice_phase(game.phase)||slot<0||slot>=game.potion_slots)return;
 if(game.phase==Phase::Dead||game.phase==Phase::Complete||game.phase==Phase::Error||game.potions[slot]==Potion::Empty)return;
 pointer_cancel();potion_popup=slot;popup_bottle=game.potions[slot];potion_revision=game.revision;potion_menu=0;potion_targeting=false;hand_focus=false;
}
void App::choose_potion_target(int actor){
 if(potion_popup<0||potion_popup>=game.potion_slots||game.revision!=potion_revision||game.potions[potion_popup]!=popup_bottle){close_potion_popup();return;}
 if(!can_use_potion(game,potion_popup,actor))return;
 int slot=potion_popup;Phase previous=game.phase;close_potion_popup();
 if(use_potion(game,slot,actor)){
  cursor=game.hand.count?(cursor<game.hand.count?cursor:game.hand.count-1):0;
  if(game.phase!=previous){cursor=0;reward_cards=false;}
  if(choice_phase(game.phase)){choice_selected_count=0;choice_revision=game.revision;busy=0;}
  else busy=8;
 }
}
void App::potion_action(UiAction action){
 if(potion_popup<0)return;
 if(game.revision!=potion_revision||game.potions[potion_popup]!=popup_bottle){close_potion_popup();return;}
 const auto*d=potion_definition(popup_bottle);if(!d){close_potion_popup();return;}
 if(action==UiAction::ClosePopupPotion){close_potion_popup();return;}
 if(action==UiAction::DetailPopupPotion){int model=(int)popup_bottle-1;close_potion_popup();detail_item=RelicKinds+model;detail_owned=-1;detail_scroll=0;return;}
 if(action==UiAction::DiscardPopupPotion){int slot=potion_popup;close_potion_popup();discard_potion(game,slot);return;}
 if(action!=UiAction::UsePopupPotion||d->automatic)return;
 if(d->targeted){
  int actor=-1;for(int i=0;i<game.enemy_count;i++)if(can_use_potion(game,potion_popup,i)){actor=i;break;}
  if(actor<0)return;potion_targeting=true;target=actor;pointer_cancel();
 }else choose_potion_target(0);
}
bool App::potion_input(Key key){
 if(potion_popup<0)return false;
 if(game.revision!=potion_revision||game.potions[potion_popup]!=popup_bottle){close_potion_popup();return true;}
 if(key==Key::Back||key==Key::Potions){close_potion_popup();return true;}
 if(key==Key::Quit){quit=true;return true;}
 if(key==Key::Detail){potion_action(UiAction::DetailPopupPotion);return true;}
 if(potion_targeting){
  if(key==Key::Left||key==Key::Right||key==Key::Up||key==Key::Down){for(int n=1;n<=game.enemy_count;n++){int i=(target+n)%game.enemy_count;if(game.enemies[i].hp){target=i;break;}}}
  if(key==Key::Confirm)choose_potion_target(target);
 }else{
  if(key==Key::Left||key==Key::Right){int step=key==Key::Left?-1:1,start=potion_popup;for(int j=1;j<=game.potion_slots;j++){int slot=(start+step*j+game.potion_slots*2)%game.potion_slots;if(game.potions[slot]!=Potion::Empty){close_potion_popup();open_potion_popup(slot);break;}}}
  if(key==Key::Up||key==Key::Down)potion_menu=1-potion_menu;
  if(key==Key::Confirm)potion_action(potion_menu?UiAction::DiscardPopupPotion:UiAction::UsePopupPotion);
 }
 return true;
}
}
