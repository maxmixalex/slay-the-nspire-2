static const char* card_reward_label(const App&a,const char*table,const char*key,const char*fallback){
 const char*value=a.content.world_string(table,key);return *value?value:fallback;
}
static void card_reward_keycap(Canvas&c,InputCommand command,int x,int y,int size=9){
 char label[20]{};input_key_label(c.a.input_bindings.keys[c.a.keyboard_only?1:0][unsigned(command)],label,sizeof label);
 const int width=mx(size,mn(32,c.width(label,6)+4));
 c.ui(I8_KeyboardKeycap,x,y,width,size);c.fitted(label,x+1,y,width-2,size,6,6,White,true,true,true,false);
}
static void card_reward_alt_picture(Canvas&c,int x,int y,int w,int h,unsigned value){
 const auto p=c.a.content.image(c.a.content.ui_image(U_RewardSkip));if(!p.pixels||p.format>3||w<=0||h<=0)return;
 // The source HSV V goes up to 1.1 on focus; the generic dimming canvas is
 // intentionally capped at 1.0. Keep this small button's overbright separate.
 uint16_t columns[64];if(w>64)return;
 for(int xx=0;xx<w;++xx)columns[xx]=uint16_t(xx*p.w/w);
 for(int yy=mx(0,-y);yy<h&&y+yy<c.clip_h;++yy){const unsigned row=unsigned(yy*p.h/h)*p.w;
  for(int xx=mx(0,-x);xx<w&&x+xx<c.clip_w;++xx){const unsigned at=row+columns[xx],alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;
   const uint8_t*bytes=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;const unsigned color=bytes[0]|unsigned(bytes[1])<<8;
   const uint16_t shaded=uint16_t((mn(31,int(div255((color>>11)*value)))<<11)|(mn(63,int(div255(((color>>5)&63)*value)))<<5)|mn(31,int(div255((color&31)*value))));
   c.paint((y+yy)*c.pitch+x+xx,shaded,alpha);
  }
 }
}
// Shared by the canonical world reward, nested event reward and legacy
// practice path. Each uses the source row and original alternative buttons.
static void native_card_reward_body(Canvas&c,const uint16_t*ids,unsigned count,bool legacy){
 auto&a=c.a;const auto&s=a.game;auto&motion=a.reward_presentation.cards;
 const uint32_t now=a.map_clock_ms;const bool clickable=motion.clickable(now);
 Canvas banner=c;banner.opacity=uint8_t(div255(banner.opacity*motion.banner_alpha(now)));banner.font_face=source_label_face(c.a.content.english(),5);
 const int by=motion.banner_y(now);
 banner.ui(a.content.version()>=Ui6ContentVersion?I6_CommonBanner:U_RewardBanner,98,by,125,31);
 banner.fitted(card_reward_label(a,"gameplay_ui","CHOOSE_CARD_HEADER","选择一张牌"),117,by+4,86,16,6,6,rgb(255,246,226),true,true,true,false);
 int focused=a.pointer.visible?-1:(a.cursor>=0&&unsigned(a.cursor)<count?a.cursor:-1);
 const auto contains=[&](unsigned index){const int scale=motion.scale(index,now),w=CardGridPainter::face_width(scale),h=CardGridPainter::face_height(scale),x=motion.x(index,now);
  return a.pointer.x>=x-w/2-2&&a.pointer.x<x+(w+1)/2+2&&a.pointer.y>=134-h/2-2&&a.pointer.y<134+(h+1)/2+2;};
 if(a.pointer.visible){
  if(motion.focus>=0&&unsigned(motion.focus)<count&&contains(unsigned(motion.focus)))focused=motion.focus;
  else for(int i=int(count)-1;i>=0;--i)if(contains(unsigned(i))){focused=i;break;}
 }
 motion.set_focus(focused,now);
 for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<count;++i){
  if((int(i)==focused)!=(pass==1))continue;
  const unsigned at=legacy?i:ids[i];const bool relic=s.phase==Phase::WorldRelicChoice;
  const Card card=legacy?(s.world.active?s.world.reward_cards[i]:Card{s.reward[i],0}):card_from_token(relic?s.world_relics.offers[at]:s.original_events.offers[at]);
  const EventCardFacts*facts=legacy?(s.world.active?&s.world.reward_facts[i]:nullptr):relic?&s.world_relics.offer_facts[at]:&s.original_events.offer_facts[at];
  const int scale=motion.scale(i,now),w=CardGridPainter::face_width(scale),h=CardGridPainter::face_height(scale),x=motion.x(i,now)-w/2,y=134-h/2;
  Canvas face=c;face.brightness=uint8_t(div255(face.brightness*motion.shade(now)));
  draw_card_face(face,card,x,y,w,h,-2,-2,-1,-1,facts);
  c.hit(mx(0,x-2),y-2,mn(320,x+w+2)-mx(0,x-2),h+4,legacy?HitKind::Choice:HitKind::WorldChoice,int(at),clickable);
 }
 const unsigned alternatives=world_card_reward_alternatives(s);
 const bool can_skip=legacy||a.world_choice_min()==0;
 const unsigned options=unsigned(can_skip)+unsigned((alternatives&2)!=0)+unsigned((alternatives&1)!=0);
 unsigned slot=0;const int ay=motion.alternative_y(now),left=(320-(int(options)*53+mx(0,int(options)-1)*14))/2;
 for(unsigned option=0;option<3;++option){
  if((option==0&&!can_skip)||(option==1&&!(alternatives&2))||(option==2&&!(alternatives&1)))continue;
  const int x=left+int(slot++)*67;
  const char*label=card_reward_label(a,"card_reward_ui",option==0?"OPTION_SKIP.name":option==1?"OPTION_REROLL.name":"OPTION_SACRIFICE.name",option==0?"跳过":option==1?"重掷":"献祭");
  const auto visual=c.control_visual(label,x,ay,53,14,ButtonFamily::RewardAlternative,false,true);
  const int width=53*visual.scale/4096,height=14*visual.scale/4096;
  card_reward_alt_picture(c,x+(53-width)/2,ay+(14-height)/2,width,height,unsigned(visual.shade));
  Canvas text=c;text.font_face=source_label_face(c.a.content.english(),5);text.fitted(label,x+2,ay+1,49,12,6,6,rgb(253,244,227),true,true,true,false);
  c.action(x,ay,53,14,option==0?UiAction::Back:option==1?UiAction::RerollReward:UiAction::SacrificeReward);
  if(!a.pointer.visible)card_reward_keycap(c,option==0?InputCommand::Cancel:InputCommand::Exhaust,x+48,ay+8);
 }
 if(!a.pointer.visible){card_reward_keycap(c,InputCommand::Confirm,0,227,10);Canvas prompt=c;prompt.font_face=source_label_face(c.a.content.english(),5);
  prompt.text(card_reward_label(a,"gameplay_ui","TO_INSPECT_PROMPT","查看详情"),15,227,6,Gold,84);}
}
