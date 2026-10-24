// All world choices use the same persisted indexes as the core. Paging and
// inspection only change App state, so they cannot reroll an offer.
static bool world_selected(const App&a,unsigned id){for(unsigned i=0;i<a.choice_selected_count;i++)if(a.choice_selected[i]==id)return true;return false;}
static const char* world_choice_heading(const State&s){
 if(s.phase==Phase::WorldRelicChoice){switch(s.world_relics.choice){
  case WorldRelicChoice::Remove:return "选择移除的牌";case WorldRelicChoice::Upgrade:return "选择升级的牌";
  case WorldRelicChoice::Transform:return "选择变化的牌";case WorldRelicChoice::Duplicate:return "选择复制的牌";
  case WorldRelicChoice::Enchant:return "选择附魔的牌";case WorldRelicChoice::OfferBundle:return "选择一组卡牌";
  case WorldRelicChoice::OfferReward:return "选择领取的奖励";case WorldRelicChoice::Potion:return "战利品";
  default:return "选择卡牌";
 }}
 switch(s.original_events.selection){
  case OriginalEventSelection::Remove:return "选择移除的牌";case OriginalEventSelection::Upgrade:return "选择升级的牌";
  case OriginalEventSelection::Transform:return "选择变化的牌";case OriginalEventSelection::Duplicate:return "选择复制的牌";
  case OriginalEventSelection::Enchant:case OriginalEventSelection::TransferEnchantment:return "选择附魔的牌";
  case OriginalEventSelection::Rewards:return "选择领取的奖励";case OriginalEventSelection::Potion:return "战利品";
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
 if(!*text){TextBuffer b{text,sizeof text,0,&c.a.content};b.text("剩余占卜：");b.number(e.crystal_charges);}c.paragraph(text,192,39,118,3,0,10,Gold);
 for(int tool=1;tool<=2;tool++){int yy=tool==1?84:139;
  c.picture(a.content.world_image("crystal",tool==1?"SMALL_DIVINATION_ICON":"BIG_DIVINATION_ICON"),194,yy,31,31);
  a.localized_text("events",tool==1?"CRYSTAL_SPHERE.button.DIVINATION_LABEL_SMALL":"CRYSTAL_SPHERE.button.DIVINATION_LABEL_BIG",text,sizeof text);
  c.paragraph(text,229,yy+2,79,3,0,10,e.crystal_tool==tool?Gold:White);
  if(e.crystal_tool==tool)c.box(190,yy-4,122,45,Gold);
  c.action(189,yy-5,125,48,tool==1?UiAction::CrystalSmall:UiAction::CrystalLarge,e.crystal_charges);
 }
 c.dim(0,214,320,26,185);c.centered("方向键定位 · Enter 占卜 · 空格切换工具",0,219,320,10,White);
}
#include "world_reward_choice_ui.inc"
// Standalone PotionReward uses the same original row as a reward set. The
// button attempts first-empty procurement; the belt remains interactive so
// the player can discard a bottle explicitly and retry a TooFull reward.
static void native_potion_reward_screen(Canvas&c){
 auto&a=c.a;const Potion bottle=a.world_choice_potion(0);const auto window=a.reward_presentation.window(a.map_clock_ms);
 Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*window.alpha));
 native_reward_backdrop(layer);native_reward_window(layer,window.y);
 const int y=79+window.y;const bool selected=a.pointer.visible?a.pointer.x>=97&&a.pointer.x<223&&a.pointer.y>=y-1&&a.pointer.y<y+27:true;
 native_reward_row(layer,y,selected);layer.picture(a.content.potion_image(bottle),106,y+4,18,18);
 layer.paragraph_fitted(potion_name(int(bottle),&a.content),128,y+2,88,22,10,selected?Gold:rgb(255,246,226),8);
 c.hit(97,y-1,126,28,HitKind::WorldChoice,0,potion_definition(bottle));
 const bool skip=a.world_choice_min()==0;
 native_proceed(c,209,211,103,27,native_reward_text(a,"gameplay_ui","CHOOSE_CARD_SKIP_BUTTON","跳过"),skip);
 c.action(205,208,111,32,UiAction::ConfirmWorldChoice,skip);
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
 a.cursor=mn(mx(0,a.cursor),mx(0,int(count)-1));
 if(rewards){native_reward_set_screen(c,ids,count);return;}
 if(potion){native_potion_reward_screen(c);return;}
 if(a.world_choice_is_card_reward()){native_card_reward_screen(c,ids,count);return;}
 const bool scrolling_cards=!potion&&!relics&&!bundle&&!(offer&&count<=3);
 auto&grid=a.configure_card_grid(CardGridSurface::WorldChoice,int(count),s.revision);CardGridPainter painter{c,grid};const int first=scrolling_cards?grid.first():a.cursor/10*10;
 c.dim(0,0,320,240,224);c.centered(world_choice_heading(s),0,6,320,16,Gold);
 char label[128]{};TextBuffer title{label,sizeof label,0,&c.a.content};
 {title.text("已选择 ");title.number(a.choice_selected_count);title.text(" / ");title.number(a.world_choice_max());}
 c.centered(label,0,29,320,10,White);
 if(bundle){
  const int first_bundle=a.cursor/2*2;
  for(int i=first_bundle;i<mn(int(count),first_bundle+2);i++){int yy=47+(i-first_bundle)*73;bool selected=world_selected(a,ids[i]);
   for(int j=0;j<3;j++){unsigned at=ids[i]*3+j;if(at<w.offer_count)small_card(c,card_from_token(w.offers[at]),56+j*79,yy,selected||a.cursor==i,true,0,-2,-2,-1,-1,&w.offer_facts[at]);}
   c.hit(44,yy-2,237,70,HitKind::WorldChoice,ids[i]);
  }
 }else if(relics){
  for(int i=first;i<mn(int(count),first+10);i++){unsigned at=ids[i];int x=12+(i-first)%5*61,y=48+(i-first)/5*67;
   const unsigned model=e.offers[at];bool selected=world_selected(a,at);
   if(selected||i==a.cursor)c.box(x-2,y-2,58,65,selected?Green:Gold);
   c.picture(a.relic_face_image(model),x+13,y+1,30,34);
   const char*name=a.relic_title(RelicId(model));
   c.paragraph(name,x,y+37,54,2,0,10,White);c.hit(x-2,y-2,58,65,HitKind::WorldChoice,at);
  }
 }else{
  const bool large=offer&&count<=3;
  int portraits[16];unsigned portrait_count=0;
  for(int i=first;i<(large?mn(int(count),first+3):grid.last());++i){unsigned at=ids[i];Card card{};const EventCardFacts*facts=nullptr;int instance=-1,deck=-1;
   if(offer){card=card_from_token(relic?w.offers[at]:e.offers[at]);facts=relic?&w.offer_facts[at]:&e.offer_facts[at];}
   else if(hand){instance=at;card=s.cards[at];}else{deck=at;card=s.deck[at];}
   portraits[portrait_count++]=a.card_face_image(card,instance,deck,facts);
  }
  a.content.prepare_images(portraits,portrait_count);
  for(int i=first;i<(large?mn(int(count),first+3):grid.last());i++){unsigned at=ids[i];Card card{};const EventCardFacts*facts=nullptr;int instance=-1,deck=-1;
   if(offer){card=card_from_token(relic?w.offers[at]:e.offers[at]);facts=relic?&w.offer_facts[at]:&e.offer_facts[at];}
   else if(hand){instance=at;card=s.cards[at];}else{deck=at;card=s.deck[at];}
   bool selected=world_selected(a,at);int x=large?(320-int(count)*94)/2+7+i*94:grid.card_x(i),y=large?65:grid.card_y(i);
   if(large)large_card(c,card,x,y,selected||a.cursor==i,-2,-2,instance,deck,facts);
   else{
    const bool hover=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+55&&a.pointer.y>=y-2&&a.pointer.y<y+77&&a.pointer.y>=grid.top&&a.pointer.y<grid.bottom;
    painter.card(i,card,hover||(!a.pointer.visible&&a.cursor==i),true,instance>=0?cost(s,instance):-2,-2,instance,deck,facts,selected);
   }
   if(large){if(selected)c.box(x-3,y-3,86,118,Green);c.hit(x-3,y-3,86,118,HitKind::WorldChoice,at);}
   else painter.hit(i,HitKind::WorldChoice,at);
  }
 }
 painter.finish();
 if(scrolling_cards)card_grid_scrollbar(c,grid,CardGridSurface::WorldChoice);
 else if(count>(bundle?2:10)){const int page_first=bundle?a.cursor/2*2:first,per=bundle?2:10;c.text("‹",12,191,16,Gold);c.action(0,187,42,21,UiAction::PreviousWorldPage,page_first>0);c.text("›",295,191,16,Gold);c.action(278,187,42,21,UiAction::NextWorldPage,page_first+per<int(count));}
 else if(offer&&world_card_reward_alternatives(s)){
  unsigned alternatives=world_card_reward_alternatives(s);if(alternatives&1){c.button("献祭牌奖励",18,189,133,false,20);c.action(16,187,137,23,UiAction::SacrificeReward);}
  if(alternatives&2){c.button("重掷",169,189,133,false,20);c.action(167,187,137,23,UiAction::RerollReward);}
 }
 c.button("清除",4,213,77,false,24);c.action(1,209,83,31,UiAction::ClearWorldChoice,a.choice_selected_count);
 c.button("详情",85,213,77,false,24);c.hit(82,209,83,31,HitKind::Command,(int)Key::Detail,count>0);
 const bool ready=a.choice_selected_count>=a.world_choice_min()&&a.choice_selected_count<=a.world_choice_max()&&world_rest_selection_allowed(s,a.choice_selected_count);
 c.button(rewards?count?"跳过剩余":"继续":!a.choice_selected_count&&a.world_choice_min()==0?"跳过":"确认",185,210,129,ready,28);
 if(!ready)c.dim(185,210,129,28,95);c.action(181,208,138,32,UiAction::ConfirmWorldChoice,ready);
}
