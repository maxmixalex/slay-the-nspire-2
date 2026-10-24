// Original RewardsScreen's centered panel and single-column list. The saved
// reward indexes remain owned by the core; page/hover/detail never populate it.
static const char* native_reward_text(const App&a,const char*table,const char*key,const char*fallback){
 const char*text=a.content.world_string(table,key);return *text?text:fallback;
}
static void native_reward_backdrop(Canvas&c){
 const auto&s=c.a.game;
 if(s.original_events.id==OriginalEventId::CrystalSphere&&s.original_events.crystal_item_count&&(s.phase==Phase::OriginalEventChoice||s.phase==Phase::WorldRelicChoice)){c.a.sync_crystal_presentation();Canvas background=c;background.opacity=255;crystal_backdrop(background);crystal_dialogue(background);}
 c.dim(0,22,320,218,185);
}
static void native_reward_set_screen(Canvas&c,const uint16_t*ids,unsigned count){
 auto&a=c.a;const auto&s=a.game;const bool relic=s.phase==Phase::WorldRelicChoice;
 const unsigned first=unsigned(a.reward_presentation.list_first());
 const auto window=a.reward_presentation.window(a.map_clock_ms);const int dy=window.y;
 Canvas panel=c;panel.opacity=uint8_t(div255(panel.opacity*window.alpha));
 native_reward_backdrop(panel);native_reward_window(panel,dy);
 Canvas rows=native_reward_list_layer(panel,dy);
 for(unsigned i=first;i<count&&i<first+7;i++){
  const unsigned at=ids[i];unsigned kind,model,amount;const EventCardFacts*facts=nullptr;
  if(relic){const auto&r=s.world_relics.rewards[s.world_relics.offers[at]];
   kind=r.kind==1?4:r.kind==2?3:r.kind==3?2:r.kind==4?1:r.kind;model=r.value;amount=r.value;if(r.kind==5)facts=&r.facts[0];
  }else{const auto&r=s.original_events.rewards[at];kind=r.kind;model=r.model;amount=r.amount;}
  const int y=88+RewardPresentation::row_offset(i)-a.reward_presentation.list_scroll.offset()+dy;
  if(!native_reward_row_visible(y,dy))continue;
  const int local=y-82-dy;
  const bool selected=a.pointer.visible?a.pointer.x>=120&&a.pointer.x<201&&a.pointer.y>=y-1&&a.pointer.y<y+17:a.cursor==int(i);
  native_reward_row(rows,local,selected);
  int image=kind==1?a.content.ui_image(U_RewardGold):kind==2?a.content.potion_image(Potion(model)):kind==3?a.relic_face_image(model):
   kind==5?a.card_face_image(card_from_token(model),-1,-1,facts):kind==6?a.content.world_image("rewards","CARD_REMOVAL"):a.content.ui_image(U_RewardCard);
  if(kind==6&&image<0)image=a.content.ui_image(U_ShopRemove);
  rows.picture(image,125,local+3,11,11);
  char name[256]{};TextBuffer b{name,sizeof name,0,&c.a.content};
  if(kind==1){b.number(amount);b.text(" 金币");}
  else if(kind==2){const auto*d=potion_definition(Potion(model));if(d)b.text(d->name);}
  else if(kind==3)b.text(a.relic_title(RelicId(model)));
  else if(kind==5)b.text(a.content.card_string(card_from_token(model).kind));
  else if(kind==6)b.text(native_reward_text(a,"gameplay_ui","COMBAT_REWARD_CARD_REMOVAL","移除一张牌"));
  else b.text(native_reward_text(a,"gameplay_ui","COMBAT_REWARD_ADD_CARD","选择一张牌"));
  rows.fitted(name,138,local,59,16,6,6,selected?Gold:rgb(255,246,226),true,false,true,false);
  native_reward_row_hit(c,y,dy,HitKind::WorldChoice,int(at));
 }
 native_reward_scrollbar(panel,dy);
 const bool can_leave=a.world_choice_min()==0;
 native_proceed(c,209,211,103,27,native_reward_text(a,"gameplay_ui",count?"CHOOSE_CARD_SKIP_BUTTON":"PROCEED_BUTTON",count?"跳过剩余":"前进"),can_leave);
 c.action(205,208,111,32,UiAction::LeaveWorldRewards,can_leave);
}
static void native_card_reward_screen(Canvas&c,const uint16_t*ids,unsigned count){
 native_reward_backdrop(c);native_card_reward_body(c,ids,count,false);
}
