static void native_reward_header(Canvas&c,const char*label,int y){
 c.ui(U_RewardBanner,70,y,180,45);Canvas title=c;title.font_face=source_label_face(c.a.content.original_label_fonts(),5);
 // rewards_screen.tscn centers HeaderLabel in y=-9..130 of the162px
 // Banner control. Its line is above the center of the ribbon's long tails.
 // Keep the authored baseline after the original font's LCD substitution;
 // the previous+17 origin placed the letters down on the lower fold.
 const int minimum=c.a.content.font_size(10,title.font_face),preferred=c.a.content.font_size(12,title.font_face);
 int size=preferred;while(size>minimum&&title.width(label,size)>122){const int next=c.a.content.font_size(size-1,title.font_face,true);if(next>=size)break;size=mx(minimum,next);}
 const int baseline=c.a.content.original_label_fonts()?21:22;
 title.fitted(label,99,y+baseline-size-1,122,size+5,size,size,rgb(255,246,226),true,true,true,false);
}
static void native_reward_window(Canvas&c,int dy){
 c.ui(U_RewardPanel,73,40+dy,174,174);
 // RewardContainerMask is the same original panel flipped both ways and
 // self-modulated to .310617. It is a visible inset, not just a clip region.
 Canvas inset=c;inset.brightness=uint8_t(div255(unsigned(inset.brightness)*79));
 ui_picture_transform(inset,c.a.content.ui_image(U_RewardPanel),160,136+dy,158,138,0,-1024);
 const char*title=c.a.content.world_string("gameplay_ui","COMBAT_REWARD_HEADER_LOOT");
 native_reward_header(c,*title?title:"战利品",35+dy);
}
static int native_reward_label_size(const App&a){
 return a.content.font_size(10);
}
static void native_reward_row(Canvas&c,int y,bool selected){
 Canvas button=c;button.brightness=uint8_t(div255(button.brightness*(selected?255:204)));
 button.ui(U_RewardButton,89,y,142,24);
 if(selected&&!c.a.pointer.visible){const auto texture=c.a.content.image(c.a.content.world_image("map","SELECTION_RETICLE"));
  if(texture.pixels&&texture.format<4)for(unsigned corner=0;corner<4;++corner){const bool right=corner&2,bottom=corner&1;const int x=right?228:87,top=bottom?y+21:y-2;
   map_reticle_tile(c,texture,x+1,top+1,x+5,top+5,right,bottom,255,true);
   map_reticle_tile(c,texture,x,top,x+4,top+4,right,bottom,255,false);}
 }
}
static void native_reward_row_content(Canvas&rows,const char*text,int y,int image,bool selected){
 native_reward_row(rows,y,selected);rows.picture(image,93,y+4,16,16);
 const int size=native_reward_label_size(rows.a);
 const int text_y=y+(24-size-2)/2;
 // The shared fitted label owns its full text and pixel-scroll clock. A
 // second focused-row painter used to restart at an arbitrary global phase
 // and draw the same row/portrait twice.
 rows.fitted(text,113,text_y,113,size+3,size,size,selected?Gold:rgb(255,246,226),false,true,true,false);
}
static Canvas native_reward_list_layer(Canvas&panel,int dy){
 Canvas layer=panel;const int top=70+dy;layer.pixels+=top*layer.pitch;if(layer.coverage)layer.coverage+=top*layer.pitch;
 layer.clip_h=132;layer.bold_face=5;return layer;
}
static bool native_reward_row_visible(int y,int dy){return y+25>70+dy&&y-1<202+dy;}
static void native_reward_row_hit(Canvas&c,int y,int dy,HitKind kind,int value,bool enabled=true){
 const int top=mx(y-1,70+dy),bottom=mn(y+25,202+dy);
 if(bottom>top)c.hit(87,top,146,bottom-top,kind,value,enabled);
}
static void native_reward_scrollbar(Canvas&c,int dy){
 auto&scroll=c.a.reward_presentation.list_scroll;if(!scroll.maximum)return;
 const int top=75+dy,bottom=202+dy,center=scroll.thumb_y(top,bottom-top);
 c.ui(I6_ScrollbarTrack,244,top,9,bottom-top);c.ui(I6_ScrollbarEdge,244,top-9,9,9);
 ui_picture_transform(c,c.a.content.ui_image(I6_ScrollbarEdge),248,bottom+4,9,9,0,-1024);
 native_scrollbar_grabber(c,244,center-4,9,9);c.hit(241,top-5,15,bottom-top+10,HitKind::RewardScroll);
}
#include "reward_cards_ui.inc"
static int native_card_reward_image(const App&a,const WorldCardOptions&options,CardKind first){
 const unsigned rarity=world_card_reward_icon_rarity(a.game,options,first);
 const int special=rarity==4?a.content.ui_image(I20_RewardRare):rarity==3?a.content.ui_image(I20_RewardUncommon):-1;
 return special>=0?special:a.content.ui_image(U_RewardCard);
}
static void native_reward_content(Canvas&c,bool include_proceed=true,int only=-1){
 auto&a=c.a;auto&s=a.game;const auto window=a.reward_presentation.window(a.map_clock_ms);
 UiAction actions[6];int count=ui_reward_actions(s,actions);a.cursor=mn(mx(0,a.cursor),count-1);
 Canvas panel=c;panel.opacity=uint8_t(div255(panel.opacity*window.alpha));const int dy=window.y;
 if(only<0)native_reward_window(panel,dy);
 Canvas rows=native_reward_list_layer(panel,dy);
 for(int i=0;i<count-1;i++){if(only>=0&&i!=only)continue;const int y=75+RewardPresentation::row_offset(unsigned(i))-a.reward_presentation.list_scroll.offset()+dy;
  if(!native_reward_row_visible(y,dy))continue;
  UiAction action=actions[i];int image=-1;char text[256]{};TextBuffer t{text,sizeof text,0,&c.a.content};
  switch(action){
   case UiAction::RewardGold:image=a.content.ui_image(U_RewardGold);t.number(s.last_gold);t.text(" 金币");break;
   case UiAction::RewardCards:{WorldCardOptions options;options.source=WorldCardSource::Encounter;
    options.odds=s.world.room_type==unsigned(WorldRoom::Boss)?WorldCardOdds::BossEncounter:s.world.room_type==unsigned(WorldRoom::Elite)?WorldCardOdds::EliteEncounter:WorldCardOdds::RegularEncounter;
    image=native_card_reward_image(a,options,s.world.reward_count?s.world.reward_cards[0].kind:CardKinds);
    const char*label=a.content.world_string("gameplay_ui","COMBAT_REWARD_ADD_CARD");t.text(*label?label:"选择一张牌");}break;
   case UiAction::RewardPotion:image=a.content.potion_image(s.reward_potion);if(const auto*d=potion_definition(s.reward_potion))t.text(d->name);break;
   case UiAction::RewardRelic:image=a.relic_face_image(s.world.reward_relic);t.text(a.relic_title(s.world.reward_relic));break;
   case UiAction::RewardGrimoire:image=a.content.ui_image(U_NecroPowerStart+(int)NPower::ForbiddenGrimoire);t.text("魔典移牌 × ");t.number(s.necro.grimoire_pending);break;
   default:break;
  }
  const bool selected=a.pointer.visible?a.pointer.x>=87&&a.pointer.x<233&&a.pointer.y>=y-1&&a.pointer.y<y+25:a.cursor==i;
  native_reward_row_content(rows,text,y-70-dy,image,selected);
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
