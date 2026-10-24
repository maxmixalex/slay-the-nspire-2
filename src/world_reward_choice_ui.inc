// Original RewardsScreen's centered panel and single-column list. The saved
// reward indexes remain owned by the core; page/hover/detail never populate it.
static const char* native_reward_text(const App&a,const char*table,const char*key,const char*fallback){
 const char*text=a.content.world_string(table,key);return *text?text:fallback;
}
static void native_reward_backdrop(Canvas&c){
 const auto&s=c.a.game;
 if(s.phase==Phase::WorldRelicChoice&&s.world_relics.return_phase==uint8_t(Phase::Reward))c.picture(c.a.combat_background(),0,22,320,218);
 c.dim(0,22,320,218,185);
}
static void native_reward_pages(Canvas&c,unsigned count,unsigned per,int y,bool cards=false){
 auto&a=c.a;const unsigned pages=(count+per-1)/per,page=unsigned(a.cursor)/per;
 if(pages<=1)return;
 char label[40]{};TextBuffer b{label,sizeof label,0,&c.a.content};b.number(page+1);b.text(" / ");b.number(pages);
 c.centered(label,99,y+1,122,cards?8:10,Muted);
 if(cards){
  c.centered("‹",0,119,22,16,page?Gold:Muted);c.action(0,116,22,26,UiAction::PreviousWorldPage,page>0);
  c.centered("›",298,119,22,16,page+1<pages?Gold:Muted);c.action(298,116,22,26,UiAction::NextWorldPage,page+1<pages);
 }else{
  c.centered("‹",55,y-2,35,16,page?Gold:Muted);c.action(52,y-2,41,21,UiAction::PreviousWorldPage,page>0);
  c.centered("›",230,y-2,35,16,page+1<pages?Gold:Muted);c.action(227,y-2,41,21,UiAction::NextWorldPage,page+1<pages);
 }
}
static void native_reward_set_screen(Canvas&c,const uint16_t*ids,unsigned count){
 auto&a=c.a;const auto&s=a.game;const bool relic=s.phase==Phase::WorldRelicChoice;
 const unsigned per=a.world_choice_page_size(),first=unsigned(a.cursor)/per*per;
 const auto window=a.reward_presentation.window(a.map_clock_ms);const int dy=window.y;
 Canvas panel=c;panel.opacity=uint8_t(div255(panel.opacity*window.alpha));
 native_reward_backdrop(panel);panel.ui(U_RewardPanel,39,46+dy,242,151);panel.ui(U_RewardBanner,28,44+dy,264,40);
 panel.centered(native_reward_text(a,"gameplay_ui","COMBAT_REWARD_HEADER_LOOT","战利品"),53,55+dy,214,16,Gold);
 for(unsigned i=first;i<count&&i<first+per;i++){
  const unsigned at=ids[i];unsigned kind,model,amount;const EventCardFacts*facts=nullptr;
  if(relic){const auto&r=s.world_relics.rewards[s.world_relics.offers[at]];
   kind=r.kind==1?4:r.kind==2?3:r.kind==3?2:r.kind==4?1:r.kind;model=r.value;amount=r.value;if(r.kind==5)facts=&r.facts[0];
  }else{const auto&r=s.original_events.rewards[at];kind=r.kind;model=r.model;amount=r.amount;}
  const int x=59,y=87+int(i-first)*26+dy;
  Canvas button=panel;button.brightness=uint8_t(div255(button.brightness*(a.cursor==int(i)?255:229)));
  button.ui(U_RewardButton,x,y,202,24);if(a.cursor==int(i))panel.box(x-2,y-1,206,26,Gold);
  int image=kind==1?a.content.ui_image(U_RewardGold):kind==2?a.content.potion_image(Potion(model)):kind==3?a.relic_face_image(model):
   kind==5?a.card_face_image(card_from_token(model),-1,-1,facts):kind==6?a.content.world_image("rewards","CARD_REMOVAL"):a.content.ui_image(U_RewardCard);
  if(kind==6&&image<0)image=a.content.ui_image(U_ShopRemove);
  panel.picture(image,x+6,y+2,20,20);
  char name[256]{};TextBuffer b{name,sizeof name,0,&c.a.content};
  if(kind==1){b.number(amount);b.text(" 金币");}
  else if(kind==2){const auto*d=potion_definition(Potion(model));if(d)b.text(d->name);}
  else if(kind==3)b.text(a.relic_title(RelicId(model)));
  else if(kind==5)b.text(a.content.card_string(card_from_token(model).kind));
  else if(kind==6)b.text(native_reward_text(a,"gameplay_ui","COMBAT_REWARD_CARD_REMOVAL","移除一张牌"));
  else b.text(native_reward_text(a,"gameplay_ui","COMBAT_REWARD_ADD_CARD","选择一张牌"));
  panel.text(name,x+34,y+5,10,a.cursor==int(i)?Gold:White,161);c.hit(x-2,y-1,206,26,HitKind::WorldChoice,at);
 }
 native_reward_pages(c,count,per,199);
 c.button("详情 I",8,213,91,false,24);c.hit(5,210,97,29,HitKind::Command,int(Key::Detail),count>0);
 const bool can_leave=a.world_choice_min()==0;
 native_proceed(c,209,211,103,27,native_reward_text(a,"gameplay_ui",count?"CHOOSE_CARD_SKIP_BUTTON":"PROCEED_BUTTON",count?"跳过剩余":"前进"),can_leave);
 c.action(205,208,111,32,UiAction::LeaveWorldRewards,can_leave);
}
static void native_card_reward_screen(Canvas&c,const uint16_t*ids,unsigned count){
 auto&a=c.a;const auto&s=a.game;const bool relic=s.phase==Phase::WorldRelicChoice;
 const unsigned per=a.world_choice_page_size(),first=unsigned(a.cursor)/per*per;
 const unsigned shown=mn(int(per),int(count-first));
 native_reward_backdrop(c);c.ui(U_RewardBanner,28,44,264,30);
 c.centered(native_reward_text(a,"gameplay_ui","CHOOSE_CARD_HEADER","选择一张牌"),52,48,216,16,Gold);
 native_reward_pages(c,count,per,64,true);
 for(unsigned i=0;i<shown;i++){
  const unsigned at=ids[first+i];Card card=card_from_token(relic?s.world_relics.offers[at]:s.original_events.offers[at]);
  const auto&facts=relic?s.world_relics.offer_facts[at]:s.original_events.offer_facts[at];
  const int x=(320-int(shown)*94)/2+7+int(i)*94,y=73;
  large_card(c,card,x,y,a.cursor==int(first+i),-2,-2,-1,-1,&facts);
  c.hit(x-3,y-3,86,118,HitKind::WorldChoice,at);
 }
 const unsigned alternatives=world_card_reward_alternatives(s);
 for(unsigned bit=1;bit<=2;bit<<=1)if(alternatives&bit){
  const int x=alternatives==3?(bit==1?17:169):93,w=134;
  c.ui(U_RewardSkip,x,190,w,21);c.centered(native_reward_text(a,"card_reward_ui",bit==1?"OPTION_SACRIFICE.name":"OPTION_REROLL.name",bit==1?"献祭牌奖励":"重掷"),x+3,193,w-6,10,Gold);
  c.action(x-2,188,w+4,25,bit==1?UiAction::SacrificeReward:UiAction::RerollReward);
 }
 if(!alternatives)c.centered("Enter 领取 · I 详情",20,193,280,10,Muted);
 c.button("详情 I",8,215,91,false,23);c.hit(5,212,97,28,HitKind::Command,int(Key::Detail),count>0);
 const bool can_back=a.world_choice_min()==0;
 c.ui(U_RewardSkip,209,213,103,25,can_back?255:100);c.centered("返回奖励",210,216,101,12,can_back?Gold:Muted);
 c.action(205,211,111,29,UiAction::Back,can_back);
}
