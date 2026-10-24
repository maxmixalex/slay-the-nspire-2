static void native_reward_header(Canvas&c,const char*label,int y,bool common=false){
 const int image=common&&c.a.content.version()>=Ui6ContentVersion?I6_CommonBanner:U_RewardBanner;
 c.ui(image,61,y,198,49);
 Canvas outline=c;outline.opacity=uint8_t(div255(outline.opacity*192));
 for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy)if(dx||dy)outline.centered(label,94+dx,y+11+dy,132,12,rgb(74,60,42));
 c.centered(label,94,y+11,132,12,rgb(255,246,226));
}
static void native_reward_window(Canvas&c,int dy){
 c.ui(U_RewardPanel,80,38+dy,160,194);
 const char*title=c.a.content.world_string("gameplay_ui","COMBAT_REWARD_HEADER_LOOT");
 native_reward_header(c,*title?title:"战利品",30+dy);
}
static void native_reward_row(Canvas&c,int y,bool selected){
 Canvas button=c;button.brightness=uint8_t(div255(button.brightness*(selected?255:204)));
 button.ui(U_RewardButton,99,y,122,26);
 if(selected&&!c.a.pointer.visible){const auto texture=c.a.content.image(c.a.content.world_image("map","SELECTION_RETICLE"));
  if(texture.pixels&&texture.format<4)for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom=corner&1;const int x=right?217:97,top=bottom?y+22:y-2;
   map_reticle_tile(c,texture,x+1,top+1,x+7,top+7,right,bottom,255,true);
   map_reticle_tile(c,texture,x,top,x+6,top+6,right,bottom,255,false);}
 }
}
static void native_reward_screen(Canvas&c){
 auto&a=c.a;auto&s=a.game;const auto window=a.reward_presentation.window(a.map_clock_ms);
 c.picture(a.combat_background(),0,22,320,218);c.actor(0,54,130);c.dim(0,22,320,218,a.reward_cards||a.grimoire_select?155:155*window.alpha/255);
 if(a.grimoire_select&&s.necro.grimoire_pending){deck_select(c);return;}
 if(a.reward_cards){
  const int count=s.world.reward_count;const bool compact=count>3;
  a.cursor=mn(mx(0,a.cursor),count);c.centered("选择一张牌",0,44,320,compact?12:16,Gold);
  for(int i=0;i<count;i++){const auto&card=s.world.reward_cards[i];const auto&facts=s.world.reward_facts[i];
   int x=count<=3?(320-count*94)/2+7+i*94:35+i%4*67,y=count<=3?69:64+i/4*72;
   if(count<=3)large_card(c,card,x,y,a.cursor==i,-2,-2,-1,-1,&facts);else small_card(c,card,x,y,a.cursor==i,true,0,-2,-2,-1,-1,&facts);
   c.hit(x-3,y-3,count<=3?86:53,count<=3?118:71,HitKind::Choice,i);
  }
  unsigned alternatives=world_card_reward_alternatives(s);
  const int option_y=compact?204:189,option_h=compact?15:20,footer_y=compact?221:211,footer_h=compact?19:25;
  if(alternatives&1){c.button("献祭牌奖励",18,option_y,133,false,option_h);c.action(16,option_y-(compact?0:1),137,option_h+(compact?1:2),UiAction::SacrificeReward);}
  if(alternatives&2){c.button("重掷",169,option_y,133,false,option_h);c.action(167,option_y-(compact?0:1),137,option_h+(compact?1:2),UiAction::RerollReward);}
  const int footer_hit=footer_y-(compact?1:2);
  c.ui(U_RewardSkip,211,footer_y,92,footer_h);c.centered("跳过",211,footer_y+2,92,12,a.cursor==count?Gold:White);c.hit(207,footer_hit,104,240-footer_hit,HitKind::Choice,count);return;
 }
 UiAction actions[6];int count=ui_reward_actions(s,actions);a.cursor=mn(mx(0,a.cursor),count-1);
 Canvas panel=c;panel.opacity=uint8_t(div255(panel.opacity*window.alpha));const int dy=window.y;
 native_reward_window(panel,dy);
 for(int i=0;i<count-1;i++){int y=79+i*29+dy;UiAction action=actions[i];int image=-1;char text[256]{};TextBuffer t{text,sizeof text,0,&c.a.content};
  switch(action){
   case UiAction::RewardGold:image=a.content.ui_image(U_RewardGold);t.number(s.last_gold);t.text(" 金币");break;
   case UiAction::RewardCards:image=a.content.ui_image(U_RewardCard);{const char*label=a.content.world_string("gameplay_ui","COMBAT_REWARD_ADD_CARD");t.text(*label?label:"选择一张牌");}break;
   case UiAction::RewardPotion:image=a.content.potion_image(s.reward_potion);if(const auto*d=potion_definition(s.reward_potion))t.text(d->name);break;
   case UiAction::RewardRelic:image=a.relic_face_image(s.world.reward_relic);t.text(a.relic_title(s.world.reward_relic));break;
   case UiAction::RewardGrimoire:image=a.content.ui_image(U_NecroPowerStart+(int)NPower::ForbiddenGrimoire);t.text("魔典移牌 × ");t.number(s.necro.grimoire_pending);break;
   default:break;
  }
  const bool selected=a.pointer.visible?a.pointer.x>=97&&a.pointer.x<223&&a.pointer.y>=y-1&&a.pointer.y<y+27:a.cursor==i;
  native_reward_row(panel,y,selected);panel.picture(image,106,y+4,18,18);
  panel.paragraph_fitted(text,128,y+2,88,22,10,selected?Gold:rgb(255,246,226),8);c.action(97,y-1,126,28,action);
 }
 const char*proceed=a.content.world_string("gameplay_ui","PROCEED_BUTTON");native_proceed(c,213,211,99,25,*proceed?proceed:"前进",true,a.cursor==count-1);c.action(209,208,107,30,UiAction::LeaveRewards);
}
