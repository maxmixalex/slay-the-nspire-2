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
static void native_reward_set_screen(Canvas&c,const uint16_t*ids,unsigned count,bool backdrop=true,int only=-1,bool proceed=true){
 auto&a=c.a;const auto&s=a.game;const bool relic=s.phase==Phase::WorldRelicChoice;
 const unsigned first=unsigned(a.reward_presentation.list_first());
 const auto window=a.reward_presentation.window(a.map_clock_ms);const int dy=window.y;
 Canvas panel=c;panel.opacity=uint8_t(div255(panel.opacity*window.alpha));
 if(backdrop)native_reward_backdrop(panel);if(only<0)native_reward_window(panel,dy);
 Canvas rows=native_reward_list_layer(panel,dy);
 for(unsigned i=first;i<count&&i<first+7;i++){
  if(only>=0&&int(i)!=only)continue;
  const unsigned at=ids[i];unsigned kind,model,amount;WorldCardOptions options;CardKind first_card=CardKinds;
  if(relic){const auto&r=s.world_relics.rewards[s.world_relics.offers[at]];
   kind=r.kind==1?4:r.kind==2?3:r.kind==3?2:r.kind==4?1:r.kind;model=r.value;amount=r.value;
   if(kind==4){options.pool_count=r.pool_count;for(unsigned j=0;j<r.pool_count&&j<11;++j)options.pools[j]=WorldCardPool(r.pools[j]);
    options.rarity_mask=r.rarity_mask;options.odds=WorldCardOdds(r.odds);options.source=WorldCardSource(r.source);
    if(r.count)first_card=card_from_token(r.tokens[0]).kind;}
  }else{const auto&r=s.original_events.rewards[at];kind=r.kind;model=r.model;amount=r.amount;
   if(kind==4){options.pool_count=r.pool_count;for(unsigned j=0;j<r.pool_count&&j<11;++j)options.pools[j]=WorldCardPool(r.pools[j]);
    options.rarity_mask=r.rarity_mask;options.type_mask=r.type_mask;options.zero_energy=r.zero_energy;options.odds=WorldCardOdds(r.odds);options.source=WorldCardSource(r.source);
    if(r.count&&r.first<MaxOriginalEventRewardCards)first_card=card_from_token(s.original_events.reward_cards[r.first]).kind;}
  }
  const int y=75+RewardPresentation::row_offset(i)-a.reward_presentation.list_scroll.offset()+dy;
  if(!native_reward_row_visible(y,dy))continue;
  const int local=y-70-dy;
  const bool selected=a.pointer.visible?a.pointer.x>=87&&a.pointer.x<233&&a.pointer.y>=y-1&&a.pointer.y<y+25:a.cursor==int(i);
  int image=kind==1?a.content.ui_image(U_RewardGold):kind==2?a.content.potion_image(Potion(model)):kind==3?a.relic_face_image(model):
   kind==5?a.content.ui_image(I20_RewardSpecial):kind==6?a.content.world_image("rewards","CARD_REMOVAL"):native_card_reward_image(a,options,first_card);
  if(kind==5&&image<0)image=a.content.ui_image(U_RewardCard);
  if(kind==6&&image<0)image=a.content.ui_image(U_ShopRemove);
  char name[256]{};TextBuffer b{name,sizeof name,0,&c.a.content};
  if(kind==1){b.number(amount);b.text(" 金币");}
  else if(kind==2){const auto*d=potion_definition(Potion(model));if(d)b.text(d->name);}
  else if(kind==3)b.text(a.relic_title(RelicId(model)));
  else if(kind==5)b.text(a.content.card_string(card_from_token(model).kind));
  else if(kind==6)b.text(native_reward_text(a,"gameplay_ui","COMBAT_REWARD_CARD_REMOVAL","移除一张牌"));
  else b.text(native_reward_text(a,"gameplay_ui","COMBAT_REWARD_ADD_CARD","选择一张牌"));
  native_reward_row_content(rows,name,local,image,selected);
  native_reward_row_hit(c,y,dy,HitKind::WorldChoice,int(at));
 }
 if(only<0)native_reward_scrollbar(panel,dy);if(!proceed)return;
 const bool can_leave=a.world_choice_min()==0;
 native_proceed(c,native_reward_text(a,"gameplay_ui",count?"CHOOSE_CARD_SKIP_BUTTON":"PROCEED_BUTTON",count?"跳过剩余":"前进"),can_leave);
 native_proceed_action(c,UiAction::LeaveWorldRewards,can_leave);
}
static void native_card_reward_screen(Canvas&c,const uint16_t*ids,unsigned count){
 native_reward_backdrop(c);native_card_reward_body(c,ids,count,false);
}
