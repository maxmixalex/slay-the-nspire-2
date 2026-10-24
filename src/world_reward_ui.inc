static void native_reward_screen(Canvas&c){
 auto&a=c.a;auto&s=a.game;const auto window=a.reward_presentation.window(a.map_clock_ms);
 c.picture(a.combat_background(),0,22,320,218);c.actor(0,54,130);c.dim(0,22,320,218,a.reward_cards||a.grimoire_select?155:155*window.alpha/255);
 if(a.grimoire_select&&s.necro.grimoire_pending){deck_select(c);return;}
 if(a.reward_cards){
  const int count=s.world.reward_count;a.cursor=mn(mx(0,a.cursor),count);c.centered("选择一张牌",0,28,320,20,Gold);
  for(int i=0;i<count;i++){const auto&card=s.world.reward_cards[i];const auto&facts=s.world.reward_facts[i];
   int x=count<=3?(320-count*94)/2+7+i*94:35+i%4*67,y=count<=3?69:52+i/4*72;
   if(count<=3)large_card(c,card,x,y,a.cursor==i,-2,-2,-1,-1,&facts);else small_card(c,card,x,y,a.cursor==i,true,0,-2,-2,-1,-1,&facts);
   c.hit(x-3,y-3,count<=3?86:53,count<=3?118:71,HitKind::Choice,i);
  }
  unsigned alternatives=world_card_reward_alternatives(s);
  if(alternatives&1){c.button("献祭牌奖励",18,189,133,false,20);c.action(16,187,137,23,UiAction::SacrificeReward);}
  if(alternatives&2){c.button("重掷",169,189,133,false,20);c.action(167,187,137,23,UiAction::RerollReward);}
  c.button("详情 I",14,211,91,false,25);c.hit(10,207,99,31,HitKind::Command,(int)Key::Detail,a.cursor<count);
  c.ui(U_RewardSkip,211,211,92,25);c.centered("跳过",211,213,92,12,a.cursor==count?Gold:White);c.hit(207,207,104,31,HitKind::Choice,count);return;
 }
 UiAction actions[6];int count=ui_reward_actions(s,actions);a.cursor=mn(mx(0,a.cursor),count-1);
 Canvas panel=c;panel.opacity=uint8_t(div255(panel.opacity*window.alpha));const int dy=window.y;
 panel.ui(U_RewardPanel,50,46+dy,220,164);panel.ui(U_RewardBanner,38,44+dy,244,40);panel.centered("战利品",60,55+dy,200,16,Gold);
 for(int i=0;i<count-1;i++){int y=86+i*24+dy;UiAction action=actions[i];int image=-1;char text[256]{};TextBuffer t{text,sizeof text,0,&c.a.content};
  switch(action){
   case UiAction::RewardGold:image=a.content.ui_image(U_RewardGold);t.number(s.last_gold);t.text(" 金币");break;
   case UiAction::RewardCards:image=a.content.ui_image(U_RewardCard);t.text("选择一张牌");break;
   case UiAction::RewardPotion:image=a.content.potion_image(s.reward_potion);if(const auto*d=potion_definition(s.reward_potion))t.text(d->name);break;
   case UiAction::RewardRelic:image=a.relic_face_image(s.world.reward_relic);t.text(a.relic_title(s.world.reward_relic));break;
   case UiAction::RewardGrimoire:image=a.content.ui_image(U_NecroPowerStart+(int)NPower::ForbiddenGrimoire);t.text("魔典移牌 × ");t.number(s.necro.grimoire_pending);break;
   default:break;
  }
  Canvas button=panel;button.brightness=uint8_t(div255(button.brightness*(a.cursor==i?255:229)));
  button.ui(U_RewardButton,69,y,182,22);if(a.cursor==i)panel.box(68,y-1,184,24,Gold);
  panel.picture(image,76,y+1,20,20);panel.text(text,103,y+3,10,a.cursor==i?Gold:White,143);c.action(68,y-1,184,24,action);
 }
 c.ui(U_RewardSkip,213,211,99,25);c.centered("前进 ›",213,214,99,12,a.cursor==count-1?Gold:White);c.action(209,208,107,30,UiAction::LeaveRewards);
}
