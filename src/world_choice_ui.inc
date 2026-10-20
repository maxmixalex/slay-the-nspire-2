// All world choices use the same persisted indexes as the core. Paging and
// inspection only change App state, so they cannot reroll an offer.
static bool world_selected(const App&a,unsigned id){for(unsigned i=0;i<a.choice_selected_count;i++)if(a.choice_selected[i]==id)return true;return false;}
static const char* world_choice_heading(const State&s){
 if(s.phase==Phase::WorldRelicChoice){switch(s.world_relics.choice){
  case WorldRelicChoice::Remove:return "选择移除的牌";case WorldRelicChoice::Upgrade:return "选择升级的牌";
  case WorldRelicChoice::Transform:return "选择变化的牌";case WorldRelicChoice::Duplicate:return "选择复制的牌";
  case WorldRelicChoice::Enchant:return "选择附魔的牌";case WorldRelicChoice::OfferBundle:return "选择一组卡牌";
  case WorldRelicChoice::OfferReward:return "选择领取的奖励";case WorldRelicChoice::Potion:return "选择药水槽";
  default:return "选择卡牌";
 }}
 switch(s.original_events.selection){
  case OriginalEventSelection::Remove:return "选择移除的牌";case OriginalEventSelection::Upgrade:return "选择升级的牌";
  case OriginalEventSelection::Transform:return "选择变化的牌";case OriginalEventSelection::Duplicate:return "选择复制的牌";
  case OriginalEventSelection::Enchant:case OriginalEventSelection::TransferEnchantment:return "选择附魔的牌";
  case OriginalEventSelection::Rewards:return "选择领取的奖励";case OriginalEventSelection::Potion:return "选择药水槽";
  case OriginalEventSelection::Relic:return "选择遗物";default:return "选择卡牌";
 }
}
static const char* crystal_item_image(const CrystalItemState&item){
 if(item.kind==0)return item.model==2?"CRYSTAL_SPHERE_COMMON_CARD_REWARD":item.model==3?"CRYSTAL_SPHERE_UNCOMMON_CARD_REWARD":"CRYSTAL_SPHERE_RARE_CARD_REWARD";
 if(item.kind==1)return "CRYSTAL_SPHERE_CURSE";
 if(item.kind==2)return item.amount>10?"CRYSTAL_SPHERE_BIG_GOLD":"CRYSTAL_SPHERE_GOLD";
 if(item.kind==3)return item.model==1?"CRYSTAL_SPHERE_COMMON_POTION":item.model==2?"CRYSTAL_SPHERE_UNCOMMON_POTION":"CRYSTAL_SPHERE_RARE_POTION";
 return "CRYSTAL_SPHERE_RELIC";
}
static void crystal_screen(Canvas&c){
 auto&a=c.a;const auto&e=a.game.original_events;a.hit_count=0;
 c.picture(a.content.world_image("crystal","CRYSTAL_SPHERE_MINIGAME_BG"),0,22,320,218);
 c.dim(0,0,320,25,190);c.centered(a.content.world_title("events","CRYSTAL_SPHERE"),0,4,320,16,Gold);
 const int left=14,top=39,cell=15;
 // Render discovered pieces through revealed cells only. Uncovering a square
 // must never disclose the position or kind of an adjacent hidden object.
 for(unsigned i=0;i<e.crystal_item_count;i++){const auto&item=e.crystal_items[i];if(!item.w||!item.h)continue;
  bool visible=false;for(unsigned y=0;y<item.h&&item.y+y<11;y++)for(unsigned x=0;x<item.w&&item.x+x<11;x++){unsigned at=(item.y+y)*11+item.x+x;if(e.crystal_owner[at]==i+1&&e.crystal_clear[at])visible=true;}
  if(visible)c.picture(a.content.world_image("crystal",crystal_item_image(item)),left+item.x*cell,top+item.y*cell,item.w*cell,item.h*cell);
 }
 for(int y=0;y<11;y++)for(int x=0;x<11;x++){int at=y*11+x,xx=left+x*cell,yy=top+y*cell;
  if(!e.crystal_clear[at]){c.rect(xx,yy,cell,cell,rgb(64,46,94));c.box(xx,yy,cell,cell,rgb(110,83,146));}
  if(at==a.cursor&&!e.crystal_clear[at])c.box(xx,yy,cell,cell,Gold);
  c.hit(xx,yy,cell,cell,HitKind::CrystalCell,at,!e.crystal_clear[at]&&e.crystal_charges);
 }
 char text[512]{};a.localized_text("events","CRYSTAL_SPHERE.minigame.divinationsRemain",text,sizeof text);
 if(!*text){TextBuffer b{text,sizeof text,0};b.text("剩余占卜：");b.number(e.crystal_charges);}c.paragraph(text,192,39,118,3,0,10,Gold);
 for(int tool=1;tool<=2;tool++){int yy=tool==1?84:139;
  c.picture(a.content.world_image("crystal",tool==1?"SMALL_DIVINATION_ICON":"BIG_DIVINATION_ICON"),194,yy,31,31);
  a.localized_text("events",tool==1?"CRYSTAL_SPHERE.button.DIVINATION_LABEL_SMALL":"CRYSTAL_SPHERE.button.DIVINATION_LABEL_BIG",text,sizeof text);
  c.paragraph(text,229,yy+2,79,3,0,10,e.crystal_tool==tool?Gold:White);
  if(e.crystal_tool==tool)c.box(190,yy-4,122,45,Gold);
  c.action(189,yy-5,125,48,tool==1?UiAction::CrystalSmall:UiAction::CrystalLarge,e.crystal_charges);
 }
 c.dim(0,214,320,26,185);c.centered("方向键定位 · Enter 占卜 · 空格切换工具",0,219,320,10,White);
}
static void world_choice_screen(Canvas&c){
 auto&a=c.a;const auto&s=a.game;a.sync_selection();a.hit_count=0;
 if(s.phase==Phase::OriginalEventChoice&&s.original_events.selection==OriginalEventSelection::Crystal){crystal_screen(c);return;}
 const bool relic=s.phase==Phase::WorldRelicChoice;
 const auto&w=s.world_relics;const auto&e=s.original_events;
 const bool potion=relic?w.choice==WorldRelicChoice::Potion:e.selection==OriginalEventSelection::Potion;
 const bool rewards=relic?w.choice==WorldRelicChoice::OfferReward:e.selection==OriginalEventSelection::Rewards;
 const bool relics=!relic&&e.selection==OriginalEventSelection::Relic;
 const bool offer=relic?(w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::CombatOffer):e.selection==OriginalEventSelection::OfferCard;
 const bool bundle=relic&&w.choice==WorldRelicChoice::OfferBundle;
 const bool hand=relic&&w.choice==WorldRelicChoice::Hand;
 uint16_t ids[MaxWorldUiChoices];unsigned count=a.world_choice_options(ids,MaxWorldUiChoices);count=mn(int(count),MaxWorldUiChoices);
 a.cursor=mn(mx(0,a.cursor),mx(0,int(count)-1));const int first=a.cursor/10*10;
 c.dim(0,0,320,240,224);c.centered(world_choice_heading(s),0,6,320,16,Gold);
 char label[128]{};TextBuffer title{label,sizeof label,0};
 if(potion){unsigned id=relic?w.offers[0]:e.offers[0];const auto*d=potion_definition(Potion(id));title.text(d?d->name:"");title.text(" · 选择替换位置");}
 else{title.text("已选择 ");title.number(a.choice_selected_count);title.text(" / ");title.number(a.world_choice_max());if(a.world_choice_min()==0)title.text(" · 可跳过");}
 c.centered(label,0,29,320,10,White);
 if(bundle){
  const int first_bundle=a.cursor/2*2;
  for(int i=first_bundle;i<mn(int(count),first_bundle+2);i++){int yy=47+(i-first_bundle)*73;bool selected=world_selected(a,ids[i]);
   for(int j=0;j<3;j++){unsigned at=ids[i]*3+j;if(at<w.offer_count)small_card(c,card_from_token(w.offers[at]),56+j*79,yy,selected||a.cursor==i,true,0,-2,-2,-1,-1,&w.offer_facts[at]);}
   c.hit(44,yy-2,237,70,HitKind::WorldChoice,ids[i]);
  }
 }else if(rewards){
  for(int i=first;i<mn(int(count),first+10);i++){unsigned at=ids[i],kind=0,model=0,amount=0;
   if(relic){const auto&r=w.rewards[w.offers[at]];kind=r.kind==1?4:r.kind==4?1:r.kind;model=r.value;amount=r.value;}
   else{const auto&r=e.rewards[at];kind=r.kind;model=r.model;amount=r.amount;}
   int x=8+(i-first)%2*156,y=48+(i-first)/2*28;bool selected=world_selected(a,at);
   c.ui(U_RewardButton,x,y,148,26);if(selected||a.cursor==i)c.box(x-1,y-1,150,28,selected?Green:Gold);
   int image=kind==1?A_Coin:kind==2?a.content.potion_image(Potion(model)):kind==3?a.content.relic_image(model):a.content.ui_image(U_RewardCard);
   c.picture(image,x+4,y+2,22,22);char name[128]{};TextBuffer b{name,sizeof name,0};
   if(kind==1){b.number(amount);b.text(" 金币");}else if(kind==2){const auto*d=potion_definition(Potion(model));if(d)b.text(d->name);}
   else if(kind==3)b.text(a.relic_title(RelicId(model)));else b.text("选择一张牌");
   c.text(name,x+30,y+5,10,White,112);c.hit(x,y,148,26,HitKind::WorldChoice,at);
  }
 }else if(potion||relics){
  for(int i=first;i<mn(int(count),first+10);i++){unsigned at=ids[i];int x=12+(i-first)%5*61,y=48+(i-first)/5*67;
   const unsigned model=potion?unsigned(potion_at(s,at)):e.offers[at];bool selected=world_selected(a,at);
   if(selected||i==a.cursor)c.box(x-2,y-2,58,65,selected?Green:Gold);
   c.picture(potion?a.content.potion_image(Potion(model)):a.content.relic_image(model),x+13,y+1,30,34);
   const auto*pd=potion?potion_definition(Potion(model)):nullptr;
   const char*name=potion?(pd?pd->name:"空药水槽"):a.relic_title(RelicId(model));
   c.paragraph(name,x,y+37,54,2,0,10,White);c.hit(x-2,y-2,58,65,HitKind::WorldChoice,at);
  }
 }else{
  const bool large=offer&&count<=3;
  for(int i=first;i<mn(int(count),first+(large?3:10));i++){unsigned at=ids[i];Card card{};const EventCardFacts*facts=nullptr;int instance=-1,deck=-1;
   if(offer){card=card_from_token(relic?w.offers[at]:e.offers[at]);facts=relic?&w.offer_facts[at]:&e.offer_facts[at];}
   else if(hand){instance=at;card=s.cards[at];}else{deck=at;card=s.deck[at];}
   bool selected=world_selected(a,at);int x=large?(320-int(count)*94)/2+7+i*94:26+(i-first)%5*56,y=large?65:48+(i-first)/5*73;
   if(large)large_card(c,card,x,y,selected||a.cursor==i,-2,-2,instance,deck,facts);
   else small_card(c,card,x,y,a.cursor==i,true,0,instance>=0?cost(s,instance):-2,-2,instance,deck,facts);
   if(selected)c.box(x-3,y-3,large?86:53,large?118:72,Green);
   c.hit(x-3,y-3,large?86:53,large?118:72,HitKind::WorldChoice,at);
  }
 }
 if(count>10&&!bundle){c.text("‹",12,191,16,Gold);c.action(0,187,42,21,UiAction::PreviousWorldPage,first>0);c.text("›",295,191,16,Gold);c.action(278,187,42,21,UiAction::NextWorldPage,first+10<int(count));}
 else if(offer&&(world_card_reward_alternatives(s)&1)){c.button("献祭牌奖励",96,189,128,false,20);c.action(94,187,132,23,UiAction::SacrificeReward);}
 else c.centered(potion?"选择已有药水会替换它 · 不选可跳过":"Enter 选择 · 空格确认 · I 详情",0,192,320,10,Muted);
 c.button("清除",4,213,77,false,24);c.action(1,209,83,31,UiAction::ClearWorldChoice,a.choice_selected_count);
 c.button("详情",85,213,77,false,24);c.hit(82,209,83,31,HitKind::Command,(int)Key::Detail,count>0);
 const bool ready=a.choice_selected_count>=a.world_choice_min()&&a.choice_selected_count<=a.world_choice_max()&&world_rest_selection_allowed(s,a.choice_selected_count);
 c.button(!a.choice_selected_count&&a.world_choice_min()==0?"跳过":"确认",185,210,129,ready,28);
 if(!ready)c.dim(185,210,129,28,95);c.action(181,208,138,32,UiAction::ConfirmWorldChoice,ready);
}
