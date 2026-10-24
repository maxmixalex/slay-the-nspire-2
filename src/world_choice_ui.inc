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
#include "crystal_ui.inc"
#include "world_reward_choice_ui.inc"
#include "world_selection_ui.inc"
// Standalone PotionReward uses the same original row as a reward set. The
// button attempts first-empty procurement; the belt remains interactive so
// the player can discard a bottle explicitly and retry a TooFull reward.
static void native_potion_reward_screen(Canvas&c){
 auto&a=c.a;const Potion bottle=a.world_choice_potion(0);const auto window=a.reward_presentation.window(a.map_clock_ms);
 Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*window.alpha));
 native_reward_backdrop(layer);native_reward_window(layer,window.y);
 const int y=88+window.y;const bool selected=a.pointer.visible?a.pointer.x>=120&&a.pointer.x<201&&a.pointer.y>=y-1&&a.pointer.y<y+17:true;
 native_reward_row(layer,y,selected);layer.picture(a.content.potion_image(bottle),125,y+3,11,11);
 const int label_size=native_reward_label_size(a);layer.bold_face=5;layer.fitted(potion_name(int(bottle),&a.content),138,y+1,60,15,label_size,label_size,selected?Gold:rgb(255,246,226),true,false,true,false);
 native_reward_row_hit(c,y,window.y,HitKind::WorldChoice,0,potion_definition(bottle));
 const bool skip=a.world_choice_min()==0;
 native_proceed(c,native_reward_text(a,"gameplay_ui","CHOOSE_CARD_SKIP_BUTTON","跳过"),skip);
 native_proceed_action(c,UiAction::ConfirmWorldChoice,skip);
}
static void world_choice_screen(Canvas&c){
 if(c.a.combat_selection_kind()!=CombatSelectionKind::None){card_choice_screen(c);return;}
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
 if(a.world_selection_kind()!=WorldSelectionKind::None){native_world_selection_screen(c,ids,count);return;}
 const bool scrolling_cards=!potion&&!relics&&!bundle&&!(offer&&count<=3);
 auto&grid=a.configure_card_grid(CardGridSurface::WorldChoice,int(count),s.revision);CardGridPainter painter{c,grid};const int first=scrolling_cards?grid.first():a.cursor/10*10;
 c.dim(0,0,320,240,224);
 if(neow_modifier_selection(s,RunModifier::SealedDeck))c.text_fitted(a.content.world_string("modifiers","SEALED_DECK.selectionPrompt"),8,6,304,14,Gold,8,true);
 else c.centered(world_choice_heading(s),0,6,320,16,Gold);
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
  int portraits[24];unsigned portrait_count=0;
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
    const bool hover=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+CardGridScroll::CardW+2&&a.pointer.y>=y-2&&a.pointer.y<y+CardGridScroll::CardH+2&&a.pointer.y>=grid.top&&a.pointer.y<grid.bottom;
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
