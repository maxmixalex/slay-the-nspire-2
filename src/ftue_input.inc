bool App::open_ftue(FtueId id,int target){
 if(ftue.active()||!content.has_ftue_assets()||unsigned(id)>=FtueCount||
    (id==FtueId::AscensionSingleplayer?seen_popup(id):seen_ftue(id)))return false;
 ftue.backstop_from=ftue.closing&&map_clock_ms-ftue.closing_at<300?uint16_t(ftue.closing_alpha*(300-(map_clock_ms-ftue.closing_at))/300):0;ftue.closing=false;
 ftue.id=id;ftue.target=int16_t(target);ftue.page=0;ftue.focus=1;ftue.page_direction=0;
 ftue.text_scroll=ftue.text_maximum=ftue.text_rows=0;
 ftue.opened=ftue.page_started=map_clock_ms;++ftue.serial;ftue.pending&=uint16_t(~ftue_bit(id));
 pointer_cancel();keyword_hover={};hit_count=0;
 // The three-page lesson and the two account popups mark on completion.
 // Every contextual NFtue in the pinned source marks when it is shown.
 if(id!=FtueId::AcceptTutorials&&id!=FtueId::CombatRules&&id!=FtueId::AscensionSingleplayer)mark_ftue(id);
 return true;
}
void App::ftue_control(int control){
 if(!ftue.active())return;
 const FtueId id=ftue.id;
 if(id==FtueId::AscensionSingleplayer&&ftue.age(map_clock_ms)<1000)return;
 if(control==5||control==6){
  const int step=mx(1,int(ftue.text_rows)-1),next=int(ftue.text_scroll)+(control==5?-step:step);
  ftue.text_scroll=uint16_t(mn(int(ftue.text_maximum),mx(0,next)));pointer_cancel();return;
 }
 if(id==FtueId::CombatRules){
  if(control==2&&ftue.page){--ftue.page;ftue.page_direction=-1;ftue.page_started=map_clock_ms;ftue.text_scroll=ftue.text_maximum=0;++ftue.serial;pointer_cancel();return;}
  if(control!=3)return;
  if(ftue.page<2){++ftue.page;ftue.page_direction=1;ftue.page_started=map_clock_ms;ftue.text_scroll=ftue.text_maximum=0;++ftue.serial;pointer_cancel();return;}
  mark_ftue(id);
  ftue.banner=true;ftue.banner_player=false;ftue.banner_at=map_clock_ms;
  // The lesson owns the start gate, not a second banner renderer. A
  // restored first fight also starts its one sequence after completion.
  turn_banner.reset();turn_banner_number=0;turn_banner_suppress_intro=false;
 }else if(control!=1&&control!=0&&control!=4)return;
 const RunOptions embark=ftue.embark;
 if(id==FtueId::AcceptTutorials){mark_ftue(id);if(control==0)set_ftues_enabled(false);}
 if(id==FtueId::AscensionSingleplayer)mark_ftue(id);
 ftue.closing_alpha=uint16_t(ftue.backstop_from+(217-ftue.backstop_from)*mn(300u,ftue.age(map_clock_ms))/300);
 ftue.closing_at=map_clock_ms;ftue.closing=true;
 ftue.id=FtueId::Count;ftue.target=-1;ftue.shuffle_waiting=false;++ftue.serial;pointer_cancel();hit_count=0;
 if(id==FtueId::AcceptTutorials){const bool animated=character_departure.animated_ftue;character_departure.animated_ftue=false;start_selected_run(embark,animated);return;}
 // These are the only source tutorials with a live "sneaky" control.
 // A normal confirmation merely closes the explanation.
 if(control==4&&id==FtueId::CannotPlayCard){ftue.skip_end_turn_check=true;input(Key::EndTurn);ftue.skip_end_turn_check=false;}
 if(control==4&&id==FtueId::Merchant)ui_action(UiAction::OpenShop);
}
bool App::ftue_input(Key key){
 if(!ftue.active())return false;
 if(key==Key::Up||key==Key::Down){ftue_control(key==Key::Up?5:6);return true;}
 if(ftue.id==FtueId::CombatRules){if(key==Key::Left)ftue_control(2);if(key==Key::Right)ftue_control(3);return true;}
 if(ftue.id==FtueId::AcceptTutorials){
  if(key==Key::Left||key==Key::Right)ftue.focus=uint8_t(key==Key::Right);
  if(key==Key::Confirm)ftue_control(ftue.focus);return true;
 }
 if(key==Key::Confirm)ftue_control(1);return true;
}
bool App::ftue_before_end_turn(){
 if(ftue.skip_end_turn_check||seen_ftue(FtueId::CanPlayCards)||!content.has_ftue_assets())return false;
 for(unsigned i=0;i<game.hand.count;++i)if(can_play_card(game,game.hand.ids[i]))return open_ftue(FtueId::CanPlayCards);
 if(++ftue.no_play_turns>=3)mark_ftue(FtueId::CanPlayCards);return false;
}
bool App::ftue_before_play(int index){
 if(seen_ftue(FtueId::CannotPlayCard)||index<0||index>=game.hand.count)return false;
 const int id=game.hand.ids[index];
 // Source UnplayableReason is a flags mask and the tutorial requires exact
 // equality to EnergyCostTooHigh, not merely that the energy bit is present.
 if(!card_has_nonresource_play_block(game,id)&&cost(game,id)>game.energy&&card_star_cost(game,game.cards[id],id)<=game.expansion.stars)
  return open_ftue(FtueId::CannotPlayCard);
 return false;
}
void App::ftue_manual_played(){
 if(ftue.manual_cards<8&&++ftue.manual_cards==8&&!seen_ftue(FtueId::CannotPlayCard))mark_ftue(FtueId::CannotPlayCard);
}
bool App::ftue_before_action(UiAction action){
 if(ftue.active())return true;
 if(action==UiAction::OpenShop&&(game.phase==Phase::Shop&&!title&&!paused&&!help&&!panel&&!library&&!character_select&&!map_preview.open&&!event_reader&&!frontend_confirm&&!settings_open&&!map_window.is_open&&detail_card<0&&detail_item<0&&status_actor<0&&deck_candidate<0)){if(!seen_ftue(FtueId::Merchant))mark_ftue(FtueId::Merchant);return false;}
 if(action==UiAction::LeaveShop&&!shop_open&&game.phase==Phase::Shop&&(game.phase==Phase::Shop&&!title&&!paused&&!help&&!panel&&!library&&!character_select&&!map_preview.open&&!event_reader&&!frontend_confirm&&!settings_open&&!map_window.is_open&&detail_card<0&&detail_item<0&&status_actor<0&&deck_candidate<0))return open_ftue(FtueId::Merchant);
 if(action==UiAction::LeaveRewards&&game.phase==Phase::Reward){
  // Boss rewards and victory use their own terminal transitions.
  const bool ordinary=game.world.room_type!=unsigned(WorldRoom::Boss);
  if(ordinary&&ui_reward_actions(game,nullptr)>1)return open_ftue(FtueId::CombatReward);
  if(ordinary&&game.world.map_row+1>4&&!seen_ftue(FtueId::CombatReward))mark_ftue(FtueId::CombatReward);
 }
 if(action==UiAction::LeaveWorldRewards&&game.phase==Phase::WorldRelicChoice&&
    game.world_relics.return_phase==uint8_t(Phase::Reward)&&game.world_relics.choice==WorldRelicChoice::OfferReward&&
    !world_choice_min()&&game.world_relics.choice_group&&game.world_relics.choice_group<=game.world_relics.group_count&&
    !game.world_relics.group_owner[game.world_relics.choice_group-1]&&
    game.world.room_type!=unsigned(WorldRoom::Boss)&&world_choice_options(nullptr,0))return open_ftue(FtueId::CombatReward);
 return false;
}
void App::observe_ftue_log(Log log){
 if(log.kind==LogKind::Turn&&game.turn==1&&!seen_ftue(FtueId::CombatRules))ftue.pending|=ftue_bit(FtueId::CombatRules);
 if(log.kind==LogKind::Shuffle&&game.turn>0&&!seen_ftue(FtueId::Shuffle)&&combat_feedback.active&&!ftue.shuffle_waiting){
  // The core has resolved the command synchronously. Its visual queue stays
  // before the actual shuffle cue until the source modal is acknowledged.
  ftue.shuffle_at=ui_feedback_later(animation+1,combat_feedback.effect_at);
  const auto piles=combat_pile_counts();ftue.shuffle_draw=piles.draw;ftue.shuffle_discard=piles.discard;ftue.shuffle_exhaust=piles.exhaust;
  ftue.shuffle_waiting=true;
 }
}
void App::observe_ftue_history(const RunHistoryNotice&notice){
 if(notice.kind==RunHistoryEvent::PotionObtained&&!seen_ftue(FtueId::ObtainPotion))ftue.pending|=ftue_bit(FtueId::ObtainPotion);
}
void App::sync_ftues(){
 if(character_departure.active())return;
 if(!content.has_ftue_assets())return;
 if(ftue.banner&&!ftue.banner_player&&map_clock_ms-ftue.banner_at>=2600)ftue.banner_player=true;
 if(ftue.banner&&map_clock_ms-ftue.banner_at>=5100)ftue.banner=false;
 const auto&w=game.world;
 if(ftue.encounter!=w.combat_count){ftue.encounter=w.combat_count;ftue.no_play_turns=0;}
 if(ftue.room_seed!=w.run_seed||ftue.room_act!=w.act_index||ftue.room_point!=w.current_point||ftue.room_type!=w.room_type){
  ftue.room_seed=w.run_seed;ftue.room_act=w.act_index;ftue.room_point=w.current_point;ftue.room_type=w.room_type;
  ftue.room_since=map_clock_ms;ftue.no_play_turns=0;
 }
 if(ftue.active()||paused||help||settings_open||frontend_confirm||library||bestiary.open||profiles_open||
    statistics_open||history_open||compendium_open||detail_card>=0||detail_item>=0||status_actor>=0||
    auxiliary.screen!=AuxiliaryScreen::None||map_preview.open||combat_preparation.active)return;
 if(character_select&&run_mode.screen==RunMenu::Closed&&!practice_entry&&!seen_popup(FtueId::AscensionSingleplayer)){
  for(const auto&character:progress.characters)if(character.highest_ascension){open_ftue(FtueId::AscensionSingleplayer);return;}
 }
 if(title||character_select||run_mode.screen!=RunMenu::Closed||!w.active)return;
 if(map_window.is_open&&map_intro_at_entry()&&!seen_ftue(FtueId::MapSelect)){
  // InitMapPrompt follows the full camera sweep (or its 200ms cancel
  // guard), then waits another100ms. A new input cannot select the root.
  if(map_act_intro.camera_running||map_act_intro.input_guard){ftue.page_started=map_clock_ms;return;}
  if(map_clock_ms-ftue.page_started>=100){const auto*m=world_current_map(game);open_ftue(FtueId::MapSelect,m?m->root:-1);}return;
 }
 if(panel||map_window.visible)return;
 // Continuing an opted-in first fight reconstructs its modal after the
 // required room poses; it cannot silently retire an uncompleted lesson.
 if(game.phase==Phase::Combat&&game.turn==1&&!seen_ftue(FtueId::CombatRules))ftue.pending|=ftue_bit(FtueId::CombatRules);
 if((ftue.pending&ftue_bit(FtueId::CombatRules))&&game.phase==Phase::Combat&&combat_hand().count&&!combat_actions_locked()){open_ftue(FtueId::CombatRules);return;}
 if(ftue.pending&ftue_bit(FtueId::ObtainPotion)){open_ftue(FtueId::ObtainPotion);return;}
 if(game.phase==Phase::Camp&&!seen_ftue(FtueId::RestSite)&&map_clock_ms-ftue.room_since>=500&&camp_options(nullptr,0)){
  room_presentation.rest.show(room_presentation.rest.mask,map_clock_ms);
  open_ftue(FtueId::RestSite);return;
 }
 if(!seen_ftue(FtueId::RelicReward)){
  if(game.phase==Phase::Treasure&&game.treasure_state==1&&w.treasure_relic!=NoWorldRelic&&
     uint32_t(map_clock_ms-(treasure_collection_started?treasure_collection_start_ms:ftue.room_since))>=1000){open_ftue(FtueId::RelicReward,-2);return;}
  if(game.phase==Phase::Reward&&!reward_cards&&reward_list_interactive()&&map_clock_ms-ftue.room_since>=66){
   UiAction actions[6];const int n=ui_reward_actions(game,actions);
   for(int i=0;i<n;++i)if(actions[i]==UiAction::RewardRelic){open_ftue(FtueId::RelicReward,i);return;}
  }
  if((reward_list_mode(*this)==2||reward_list_mode(*this)==3)&&reward_list_interactive()&&map_clock_ms-ftue.room_since>=66){
   uint16_t ids[MaxWorldUiChoices];const unsigned count=world_choice_options(ids,MaxWorldUiChoices);const bool relic=game.phase==Phase::WorldRelicChoice;
   for(unsigned i=0;i<count;++i){const unsigned at=ids[i];const bool is_relic=relic?game.world_relics.rewards[game.world_relics.offers[at]].kind==2:game.original_events.rewards[at].kind==3;
    if(is_relic){open_ftue(FtueId::RelicReward,int(i));return;}}
  }
 }
 if(card_reward_active()&&card_reward_input_ready()&&!seen_ftue(FtueId::PowerCard)){
  if(game.phase==Phase::Reward){for(unsigned i=0;i<w.reward_count;++i)if(definitions[w.reward_cards[i].kind].type==CardType::Power){open_ftue(FtueId::PowerCard,int(i));return;}}
  else{uint16_t ids[MaxWorldUiChoices];const unsigned count=world_choice_options(ids,MaxWorldUiChoices);
   for(unsigned i=0;i<count;++i){const unsigned token=game.phase==Phase::WorldRelicChoice?game.world_relics.offers[ids[i]]:game.original_events.offers[ids[i]];
    if(definitions[card_from_token(token).kind].type==CardType::Power){open_ftue(FtueId::PowerCard,int(i));return;}}
  }
 }
}
