static void native_reward_header(Canvas&c,const char*label,int y){
 c.ui(U_RewardBanner,98,y,124,31);Canvas title=c;title.font_face=5;
 title.fitted(label,125,y-2,70,27,8,6,rgb(255,246,226),true,true,true,false);
}
static void native_reward_window(Canvas&c,int dy){
 c.ui(U_RewardPanel,110,62+dy,100,122);
 const char*title=c.a.content.world_string("gameplay_ui","COMBAT_REWARD_HEADER_LOOT");
 native_reward_header(c,*title?title:"战利品",57+dy);
}
static int native_reward_label_size(const App&a){
 // RewardButton's source label is 24px, projected to 5px. Historical packs
 // without the new native glyph size retain a real available font face.
 return a.content.glyph_metrics('?',5).w?5:a.content.glyph_metrics('?',6).w?6:8;
}
static void native_reward_row(Canvas&c,int y,bool selected){
 Canvas button=c;button.brightness=uint8_t(div255(button.brightness*(selected?255:204)));
 button.ui(U_RewardButton,122,y,77,16);
 if(selected&&!c.a.pointer.visible){const auto texture=c.a.content.image(c.a.content.world_image("map","SELECTION_RETICLE"));
  if(texture.pixels&&texture.format<4)for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom=corner&1;const int x=right?195:120,top=bottom?y+13:y-2;
   map_reticle_tile(c,texture,x+1,top+1,x+5,top+5,right,bottom,255,true);
   map_reticle_tile(c,texture,x,top,x+4,top+4,right,bottom,255,false);}
 }
}
static Canvas native_reward_list_layer(Canvas&panel,int dy){
 Canvas layer=panel;const int top=82+dy;layer.pixels+=top*layer.pitch;if(layer.coverage)layer.coverage+=top*layer.pitch;
 layer.clip_h=92;layer.bold_face=5;return layer;
}
static bool native_reward_row_visible(int y,int dy){return y+17>82+dy&&y-1<174+dy;}
static void native_reward_row_hit(Canvas&c,int y,int dy,HitKind kind,int value,bool enabled=true){
 const int top=mx(y-1,82+dy),bottom=mn(y+17,174+dy);
 if(bottom>top)c.hit(120,top,81,bottom-top,kind,value,enabled);
}
static void native_reward_scrollbar(Canvas&c,int dy){
 auto&scroll=c.a.reward_presentation.list_scroll;if(!scroll.maximum)return;
 const int top=89+dy,bottom=174+dy,center=scroll.thumb_y(top,bottom-top);
 c.ui(I6_ScrollbarTrack,208,top,9,bottom-top);c.ui(I6_ScrollbarEdge,208,top-9,9,9);
 ui_picture_transform(c,c.a.content.ui_image(I6_ScrollbarEdge),212,bottom+4,9,9,0,-1024);
 c.ui(I6_ScrollbarGrabber,208,center-4,9,9);c.hit(205,top-5,15,bottom-top+10,HitKind::RewardScroll);
}
#include "reward_cards_ui.inc"
static void native_reward_content(Canvas&c,bool include_proceed=true,int only=-1){
 auto&a=c.a;auto&s=a.game;const auto window=a.reward_presentation.window(a.map_clock_ms);
 UiAction actions[6];int count=ui_reward_actions(s,actions);a.cursor=mn(mx(0,a.cursor),count-1);
 Canvas panel=c;panel.opacity=uint8_t(div255(panel.opacity*window.alpha));const int dy=window.y;
 if(only<0)native_reward_window(panel,dy);
 Canvas rows=native_reward_list_layer(panel,dy);const int label_size=native_reward_label_size(a);
 for(int i=0;i<count-1;i++){if(only>=0&&i!=only)continue;const int y=88+RewardPresentation::row_offset(unsigned(i))-a.reward_presentation.list_scroll.offset()+dy;
  if(!native_reward_row_visible(y,dy))continue;
  UiAction action=actions[i];int image=-1;char text[256]{};TextBuffer t{text,sizeof text,0,&c.a.content};
  switch(action){
   case UiAction::RewardGold:image=a.content.ui_image(U_RewardGold);t.number(s.last_gold);t.text(" 金币");break;
   case UiAction::RewardCards:image=a.content.ui_image(U_RewardCard);{const char*label=a.content.world_string("gameplay_ui","COMBAT_REWARD_ADD_CARD");t.text(*label?label:"选择一张牌");}break;
   case UiAction::RewardPotion:image=a.content.potion_image(s.reward_potion);if(const auto*d=potion_definition(s.reward_potion))t.text(d->name);break;
   case UiAction::RewardRelic:image=a.relic_face_image(s.world.reward_relic);t.text(a.relic_title(s.world.reward_relic));break;
   case UiAction::RewardGrimoire:image=a.content.ui_image(U_NecroPowerStart+(int)NPower::ForbiddenGrimoire);t.text("魔典移牌 × ");t.number(s.necro.grimoire_pending);break;
   default:break;
  }
  const bool selected=a.pointer.visible?a.pointer.x>=120&&a.pointer.x<201&&a.pointer.y>=y-1&&a.pointer.y<y+17:a.cursor==i;
  const int local=y-82-dy;native_reward_row(rows,local,selected);rows.picture(image,125,local+3,11,11);
  rows.fitted(text,138,local+1,60,15,label_size,label_size,selected?Gold:rgb(255,246,226),true,false,true,false);
  native_reward_row_hit(c,y,dy,HitKind::UiAction,int(action));
 }
 if(only<0)native_reward_scrollbar(panel,dy);
 if(!include_proceed)return;
 const bool remaining=count>1;const char*proceed=a.content.world_string("gameplay_ui",remaining?"CHOOSE_CARD_SKIP_BUTTON":"PROCEED_BUTTON");native_proceed(c,*proceed?proceed:remaining?"跳过":"前进",true,a.cursor==count-1);native_proceed_action(c,UiAction::LeaveRewards);
}
static void native_reward_screen(Canvas&c){
 auto&a=c.a;auto&s=a.game;const auto window=a.reward_presentation.window(a.map_clock_ms);
 combat_reward_room(c);c.dim(0,22,320,218,a.reward_cards||a.grimoire_select?155:155*window.alpha/255);
 if(a.grimoire_select&&s.necro.grimoire_pending){deck_select(c);return;}
 if(a.reward_cards){
  native_card_reward_body(c,nullptr,s.world.reward_count,true);return;
 }
 native_reward_content(c);
}
