#include "ftue_input.inc"
static const char*ftue_text(const App&a,const char*key){return a.content.world_string("ftues",key);}
static void ftue_material(Canvas&c,int asset,int x,int y,int w,int h,bool vertical=false){
 const auto p=c.a.content.image(c.a.content.ui_image(asset));if(!p.pixels||p.format>3||w<=0||h<=0)return;
 for(int yy=mx(0,-y);yy<h&&y+yy<c.clip_h;++yy)for(int xx=mx(0,-x);xx<w&&x+xx<c.clip_w;++xx){
  const unsigned at=unsigned(yy*p.h/h)*p.w+unsigned(xx*p.w/w),alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;
  const auto*bytes=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;uint16_t color=uint16_t(bytes[0]|unsigned(bytes[1])<<8);
  if(!vertical)color=presentation::yiq_material(color,500,1200);
  else{
   // vertical_popup.tscn h=.505,s=1,v=.75. The source row-vector hue
   // rotation and inverse of its actual YIQ matrix use fixed Q16 constants.
   const int r=(color>>11)*4096/31,g=((color>>5)&63)*4096/63,b=(color&31)*4096/31;
   const int l=(2989*r+5870*g+1140*b)/10000,i=(5959*r-2774*g-3216*b)/10000,q=(2115*r-5229*g+3114*b)/10000;
   const int ii=(-65504*i-2059*q)/65536,qq=(2059*i-65504*q)/65536;
   const auto channel=[&](int il,int ic,int qc,unsigned maximum){const int v=int((int64_t(il)*l+int64_t(ic)*ii+int64_t(qc)*qq)*3/(65536*4));return unsigned(mn(4096,mx(0,v)))*maximum/4096;};
   color=uint16_t(channel(65737,62600,40585,31)<<11|channel(65487,-17811,-42368,63)<<5|channel(65318,-72425,111747,31));
  }
  c.paint((y+yy)*c.pitch+x+xx,color,alpha);
 }
}
static fitted_text::Plan ftue_paragraph(Canvas c,const char*text,int x,int y,int w,int h,int size,bool center=false,int step=0,bool middle=false){
 // The contextual 22px source labels project to four native pixels and
 // have -2px line separation. No rescaling of an 8px bitmap is involved.
 if(!c.a.content.glyph_metrics('?',size,c.font_face).advance)size=c.a.content.font_size(size,c.font_face);
 if(!step)step=size+1;
 auto metrics=[&](const RichChar&ch,int px){const auto g=c.a.content.glyph_metrics(ch.codepoint,px,c.font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.w),int(g.advance))};};
 fitted_text::Plan plan;plan.size=size;plan.step=step;plan.total=fitted_text::line_count(text,w,size,metrics);plan.rows=mn(plan.total,mx(0,(h-size)/step+1));plan.truncated=plan.rows<plan.total;
 int widths[32]{};if(plan.rows>32)plan.rows=32;
 if(center)fitted_text::visit(text,w,plan,metrics,[&](const RichChar&ch,int at,int row,int px){widths[row]=at+metrics(ch,px).width;});
 const int top=y+((center||middle)?mx(0,(h-(plan.rows-1)*step-size)/2):0);
 const uint16_t colors[]={rgb(255,246,226),Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
 c.clip_w=mn(c.clip_w,x+w);c.clip_h=mn(c.clip_h,y+h);
 fitted_text::visit(text,w,plan,metrics,[&](const RichChar&ch,int at,int row,int px){
  if(ch.index>=c.text_visible_characters)return;
  const auto g=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));const int left=x+at+(center?mx(0,(w-widths[row])/2):0);
  Canvas shadow=c;shadow.opacity=uint8_t(div255(shadow.opacity*128));shadow.character(ch.codepoint,g,left+1,top+row*step+1,0,true);
  c.character(ch.codepoint,g,left,top+row*step,colors[ch.style.color]);
 });return plan;
}
static void ftue_arrow(Canvas&c,int cx,int cy,int sn=0,int cs=1024,bool flip_h=false,bool flip_v=false,int w=47,int h=39){
 const auto p=c.a.content.image(c.a.content.ui_image(I9_FtueArrow));if(!p.pixels||p.format>3)return;
 const int hw=(CombatCardMotion::magnitude(cs)*w+CombatCardMotion::magnitude(sn)*h)/2048+2;
 const int hh=(CombatCardMotion::magnitude(sn)*w+CombatCardMotion::magnitude(cs)*h)/2048+2;
 for(int y=mx(0,cy-hh);y<mn(c.clip_h,cy+hh);++y)for(int x=mx(0,cx-hw);x<mn(c.clip_w,cx+hw);++x){
  int u=int(p.w)*512+((x-cx)*cs+(y-cy)*sn)*int(p.w)/w;
  int v=int(p.h)*512+(-(x-cx)*sn+(y-cy)*cs)*int(p.h)/h;
  if(u<0||v<0||u>=int(p.w)*1024||v>=int(p.h)*1024)continue;
  if(flip_h)u=int(p.w)*1024-1-u;if(flip_v)v=int(p.h)*1024-1-v;
  const unsigned at=unsigned(v/1024)*p.w+unsigned(u/1024),alpha=p.alpha?p.alpha[at]:255;if(!alpha)continue;
  const auto*b=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;c.paint(y*c.pitch+x,uint16_t(b[0]|unsigned(b[1])<<8),alpha);
 }
}
static void ftue_button(Canvas&c,int x,int y,int w,int h,int action,const char*label,bool ordinary=true){
 auto&a=c.a;const bool focused=a.pointer.visible?a.pointer.x>=x&&a.pointer.x<x+w&&a.pointer.y>=y&&a.pointer.y<y+h:a.ftue.focus==action;
 const int bw=mn(w,h*195/63),bh=bw*63/195,scale=focused?4301:4096;
 const int sw=bw*scale/4096,sh=bh*scale/4096,left=x+(w-sw)/2,top=y+(h-sh)/2;
 if(ordinary){
  const unsigned t=a.ftue.age(a.map_clock_ms)%1200;
  const unsigned alpha=focused?255:t<600?t*255/600:255-(t-600)*191/600;
  Canvas outline=c;outline.tint_red=255;outline.tint_green=213;outline.tint_blue=128;outline.ui(I9_FtueConfirmOutline,left,top,sw,sh,alpha);
  small_material_icon(c,a.content.ui_image(I9_FtueConfirm),left,top,sw,sh,1000,focused?1400:1000);
 }else{
  if(focused)c.ui(action?I6_PopupConfirmOutline:I6_PopupCancelOutline,x,y,w,h,160);
  c.ui(action?I6_PopupConfirm:I6_PopupCancel,x,y,w,h);
 }
 Canvas text=c;text.font_face=source_label_face(c.a.content.english(),5);text.fitted(label,x+5,y+2,w-10,h-3,5,5,ordinary?rgb(239,200,81):White,true,true,true,false,rgb(61,25,23));
 c.hit(x,y,w,h,HitKind::FtueControl,action);
 if(ordinary&&!a.pointer.visible)card_reward_keycap(c,InputCommand::Select,x+w-11,y+3,9);
}
static void ftue_lift(Canvas&c){
 auto&a=c.a;const auto&s=a.game;
 switch(a.ftue.id){
 case FtueId::CanPlayCards:combat_energy(c,combat_stars_visible(a),false);break;
 case FtueId::CannotPlayCard:native_end_turn(c,true);c.hit(267,188,42,17,HitKind::FtueControl,4);break;
 case FtueId::Shuffle:{const auto n=a.combat_pile_counts();combat_pile(c,A_DrawPile,n.draw,3,222,1);combat_pile(c,A_DiscardPile,n.discard,302,222,2);break;}
 case FtueId::ObtainPotion:hud_potion_container(c);break;
 case FtueId::Merchant:{const bool focus=a.pointer.visible&&a.pointer.x>=MerchantLayout::BodyX&&a.pointer.x<MerchantLayout::BodyX+MerchantLayout::BodyW&&a.pointer.y>=MerchantLayout::BodyY&&a.pointer.y<MerchantLayout::BodyY+MerchantLayout::BodyH;
  merchant_idle(c,focus?AnimMerchantRoomFocused:AnimMerchantRoomTop);c.hit(MerchantLayout::BodyX,MerchantLayout::BodyY,MerchantLayout::BodyW,MerchantLayout::BodyH,HitKind::FtueControl,4);break;}
 case FtueId::CombatReward:if(s.phase==Phase::Reward)native_reward_content(c,false);else{uint16_t ids[MaxWorldUiChoices];const unsigned n=a.world_choice_options(ids,MaxWorldUiChoices);native_reward_set_screen(c,ids,n,false,-1,false);}break;
 case FtueId::RelicReward:
  if(a.ftue.target>=0&&s.phase==Phase::Reward)native_reward_content(c,false,a.ftue.target);
  else if(a.ftue.target>=0){uint16_t ids[MaxWorldUiChoices];const unsigned n=a.world_choice_options(ids,MaxWorldUiChoices);native_reward_set_screen(c,ids,n,false,a.ftue.target,false);}
  else if(a.ftue.target==-2){const int y=93+a.treasure_holder_offset();c.picture(a.relic_face_image(s.world.treasure_relic),149,y,23,23);}
  break;
 case FtueId::MapSelect:{WorldMapLayout layout;const auto*m=world_current_map(s);
  if(m&&world_map_layout(s,layout)&&unsigned(a.ftue.target)<m->count){const auto p=layout.points[a.ftue.target];const auto view=a.map_view(layout);const auto colors=map_colors(s.world.rooms[s.world.act_index].region);
   if(m->points[a.ftue.target].type==reference::PointType::Start)map_special_node(c,*m,unsigned(a.ftue.target),p.x,p.y+view.origin,true,false,colors);
   else map_normal_node(c,*m,unsigned(a.ftue.target),p.x,p.y+view.origin,p.angle,true,false,colors);
  }break;}
 case FtueId::PowerCard:{const int i=a.ftue.target;const auto&motion=a.reward_presentation.cards;
  uint16_t ids[MaxWorldUiChoices];const unsigned count=s.phase==Phase::Reward?s.world.reward_count:a.world_choice_options(ids,MaxWorldUiChoices);
  if(i>=0&&unsigned(i)<count){const bool legacy=s.phase==Phase::Reward,relic=s.phase==Phase::WorldRelicChoice;const unsigned at=legacy?unsigned(i):ids[i];
   const Card card=legacy?s.world.reward_cards[at]:card_from_token(relic?s.world_relics.offers[at]:s.original_events.offers[at]);
   const EventCardFacts*facts=legacy?&s.world.reward_facts[at]:relic?&s.world_relics.offer_facts[at]:&s.original_events.offer_facts[at];
   const int scale=motion.scale(unsigned(i),a.map_clock_ms),w=CardGridPainter::face_width(scale),h=CardGridPainter::face_height(scale);
   draw_card_face(c,card,motion.x(unsigned(i),a.map_clock_ms)-w/2,134-h/2,w,h,-2,-2,-1,-1,facts);
  }break;}
 case FtueId::RestSite:{uint8_t ids[MaxWorldRestOptions];const unsigned n=a.camp_options(ids,MaxWorldRestOptions),shown=mn(4,int(n));const int start=(320-int(shown)*47-int(shown-1)*19)/2;
  Canvas choices=c;choices.opacity=uint8_t(div255(choices.opacity*a.room_presentation.rest.alpha(a.map_clock_ms)));
  choices.brightness=uint8_t(div255(choices.brightness*a.room_presentation.rest.alpha(a.map_clock_ms)));
  for(unsigned i=0;i<shown;++i){const auto sample=a.room_presentation.rest.buttons[ids[i]].sample(a.map_clock_ms);
   native_camp_option_picture(choices,ids[i],start+int(i)*66,world_rest_option_enabled(s,ids[i]),sample);
  }break;}
 default:break;
 }
 // Only the source sneaky controls remain live above the modal backstop.
 unsigned write=0;for(unsigned i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::FtueControl)a.hits[write++]=a.hits[i];a.hit_count=write;
}
static void ftue_combat_pages(Canvas&c){
 auto&a=c.a;const auto&f=a.ftue;const uint32_t age=a.map_clock_ms-f.page_started;
 const unsigned slide=room_curve::expo_out(age,500);const int dx=f.page_direction*38*int(4096-slide)/4096;
 Canvas image=c;const unsigned ease=room_curve::cubic_out(age,500);
 image.opacity=uint8_t(div255(image.opacity*(f.page_direction?128+127*ease/4096:255*ease/4096)));
 image.ui(I9_FtueCombat0+f.page,33+dx,65,128,98);
 Canvas text=c;text.bold_face=5;text.opacity=uint8_t(div255(text.opacity*255*room_curve::linear(age,f.page_direction?600:1000)/4096));
 char key[32]{};TextBuffer k{key,sizeof key,0};k.text("TUTORIAL_FTUE_BODY_");k.number(f.page+1);const char*body=ftue_text(a,key);
 unsigned characters=0;RichReader reader(body);RichChar ch;while(reader.next(ch))++characters;
 text.text_visible_characters=characters*room_curve::sine_out(age,600)/4096;
 ftue_paragraph(text,body,169+dx,69,119,92,5,false,0,true);
 c.fitted(ftue_text(a,"COMBAT_BASICS_FTUE_HEADER"),46,174,229,11,5,5,Gold,true,true);
 char page[64]{};const int current=f.page+1;
 const auto resolve=[](const void*p,const char*key)->LocalizedValue{return wt_equal(key,"currentPage")?LocalizedValue{true,*static_cast<const int*>(p),1,nullptr}:wt_equal(key,"totalPages")?LocalizedValue{true,3,1,nullptr}:LocalizedValue{};};
 LocalizedTemplate(page,sizeof page,resolve,&current).format(ftue_text(a,"COMBAT_BASICS_FTUE_PAGE_COUNT"));c.fitted(page,46,180,229,11,5,5,rgb(135,206,235),true,true);
 if(f.page){c.ui(I7_SettingsArrowLeft,8,108,24,24);c.hit(8,108,24,24,HitKind::FtueControl,2);}
 c.ui(I7_SettingsArrowRight,288,108,24,24);c.hit(288,108,24,24,HitKind::FtueControl,3);
}
static void ftue_start_label(Canvas c,const char*text,unsigned scale){
 c.font_face=source_label_face(c.a.content.english(),4);const int size=c.a.content.font_size(18,c.font_face);
 int x=160-int(unsigned(c.width(text,size))*scale/4096)/2;
 while(*text){const uint32_t cp=utf8(text);const auto g=c.a.content.glyph(cp,size,c.font_face);
  if(!g.pixels||g.w>48||g.h>48){x+=int(g.advance*scale/4096);continue;}
  uint8_t alpha[48*48]{};unsigned at=0;
  if(g.encoded_size){for(unsigned i=0;i+1<g.encoded_size&&at<unsigned(g.w)*g.h;i+=2)for(unsigned j=0;j<g.pixels[i]&&at<unsigned(g.w)*g.h;++j)alpha[at++]=g.pixels[i+1];}
  else for(unsigned i=0;i<unsigned(g.w)*g.h;++i)alpha[i]=g.pixels[i];
  const int w=mx(1,int(g.w*scale/4096)),h=mx(1,int(g.h*scale/4096)),r=mx(1,int(5*scale/4096)),top=109-h/2;
  // Original Label outline24px at the same node scale. A bounded coverage
  // plane prevents overlapping outline samples from darkening twice.
  uint8_t outline[96*96]{};const int ow=w+2*r,oh=h+2*r;
  if(ow<=96&&oh<=96){
   for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){const unsigned a=alpha[unsigned(yy*g.h/h)*g.w+unsigned(xx*g.w/w)];if(!a)continue;
    for(int dy=-r;dy<=r;++dy)for(int dx=-r;dx<=r;++dx)if(dx*dx+dy*dy<=r*r){auto&v=outline[(yy+dy+r)*96+xx+dx+r];if(a>v)v=uint8_t(a);}}
   for(int yy=0;yy<oh;++yy)for(int xx=0;xx<ow;++xx)if(outline[yy*96+xx]&&x+xx-r>=0&&x+xx-r<c.clip_w&&top+yy-r>=0&&top+yy-r<c.clip_h)
    c.paint((top+yy-r)*c.pitch+x+xx-r,0,outline[yy*96+xx]/2);
  }
  for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)if(x+xx>=0&&x+xx<c.clip_w&&top+yy>=0&&top+yy<c.clip_h)
   c.paint((top+yy)*c.pitch+x+xx,rgb(239,200,81),alpha[unsigned(yy*g.h/h)*g.w+unsigned(xx*g.w/w)]);
  x+=int(g.advance*scale/4096);
 }
}
static void ftue_combat_start(Canvas&c){
 const auto&f=c.a.ftue;if(!f.banner)return;const uint32_t age=c.a.map_clock_ms-f.banner_at;if(age<300||age>=5100)return;
 const unsigned q=room_curve::expo_out(age-300,750),scale=4096+(4096-q)/5;
 unsigned alpha=64*q/4096;if(age>4100)alpha=64*(4096-room_curve::cubic_out(age-4100,1000))/4096;
 Canvas band=c;band.opacity=uint8_t(div255(band.opacity*alpha));const int dx=int((scale-4096)*183/4096),dy=int((scale-4096)*17/4096);
 band.rect(-dx,88-dy,int(320*scale/4096),int(40*scale/4096),rgb(16,0,0));
 if(age>=2600)return;Canvas label=c;
 const unsigned opacity=age<1600?255*room_curve::expo_out(age-300,1300)/4096:255*(4096-room_curve::cubic_out(age-1600,1000))/4096;
 label.opacity=uint8_t(div255(label.opacity*opacity));ftue_start_label(label,c.a.content.world_string("gameplay_ui","BATTLE_START"),8192-q);
}
static void ftue_screen(Canvas&c){
 auto&a=c.a;const auto id=a.ftue.id;
 if(!a.ftue.active()){
  if(a.ftue.closing){const unsigned age=a.map_clock_ms-a.ftue.closing_at;if(age>=300)a.ftue.closing=false;else{Canvas back=c;back.opacity=uint8_t(div255(back.opacity*a.ftue.closing_alpha*(300-age)/300));back.rect(0,0,320,240,0);}}
  ftue_combat_start(c);return;
 }
 a.hit_count=0;Canvas backstop=c;const unsigned alpha=a.ftue.backstop_from+(217-a.ftue.backstop_from)*room_curve::linear(a.ftue.age(a.map_clock_ms),300)/4096;
 backstop.opacity=uint8_t(div255(backstop.opacity*alpha));backstop.rect(0,0,320,240,0);
 ftue_lift(c);
 if(id==FtueId::CombatRules){ftue_combat_pages(c);return;}
 if(id==FtueId::AcceptTutorials){
  ftue_material(c,I6_PopupVertical,110,63,99,114,true);Canvas text=c;text.font_face=source_label_face(c.a.content.english(),5);
  text.fitted(a.content.world_string("main_menu_ui","ENABLE_TUTORIALS.title"),114,74,91,14,6,6,Gold,true,true);
  text.font_face=0;text.bold_face=5;ftue_paragraph(text,a.content.world_string("main_menu_ui","ENABLE_TUTORIALS.description"),120,85,80,63,5,true);
  for(int i=0;i<2;++i)ftue_button(c,110+i*65,148,34,14,i,a.content.world_string("main_menu_ui",i?"GENERIC_POPUP.confirm":"GENERIC_POPUP.cancel"),false);
  return;
 }
 if(id==FtueId::AscensionSingleplayer){
  const uint32_t age=a.ftue.age(a.map_clock_ms),at=age>1000?age-1000:0;Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*255*room_curve::linear(at,300)/4096));
  const int t=int(room_curve::linear(at,300)),u=t-4096;const int ease=4096+int(int64_t(u)*u/4096*(int64_t(11066)*u/4096+6970)/4096);
  const int dy=19*(4096-ease)/4096;ftue_material(layer,I6_PopupVertical,97,48+dy,126,144,true);
  layer.ui(U_TopBarAscension,104,56+dy,12,12);Canvas title=layer;title.font_face=source_label_face(c.a.content.english(),5);title.fitted(ftue_text(a,"ASCENSION_SINGLEPLAYER_FTUE_TITLE"),116,57+dy,88,14,6,6,Gold,true,true);
  layer.bold_face=5;ftue_paragraph(layer,ftue_text(a,"ASCENSION_SINGLEPLAYER_FTUE_DESCRIPTION"),111,73+dy,98,76,5);
  layer.rect(135,151+dy,49,1,rgb(126,112,98));ftue_paragraph(layer,ftue_text(a,"ASCENSION_SINGLEPLAYER_FTUE_DISCLAIMER"),111,156+dy,98,23,5);
  if(age>=1000)ftue_button(layer,132,178+dy,56,19,1,ftue_text(a,"CONFIRM_BUTTON"));return;
 }
 int x=105,y=96,w=110,h=49;const char*prefix="";
 switch(id){
 case FtueId::MapSelect:{prefix="MAP_SELECT";WorldMapLayout layout;const auto*m=world_current_map(a.game);if(m&&world_map_layout(a.game,layout)&&unsigned(a.ftue.target)<m->count){const auto p=layout.points[a.ftue.target];const int py=p.y+a.map_view(layout).origin;x=p.x+24;y=py-96;ftue_arrow(c,p.x+6,py-36,-534,873);}break;}
 case FtueId::CanPlayCards:prefix="CAN_PLAY_CARDS_FTUE";x=88;w=146;h=65;ftue_arrow(c,59,168);break;
 case FtueId::CannotPlayCard:prefix="CANNOT_PLAY_CARD_FTUE";ftue_arrow(c,252,169,0,1024,true);break;
 case FtueId::ObtainPotion:prefix="POTION_FTUE";ftue_arrow(c,126,42,0,1024,false,true);break;
 case FtueId::CombatReward:prefix="REWARDS_FTUE";x=190;y=151;ftue_arrow(c,221,126,887,512);break;
 case FtueId::PowerCard:prefix="POWER_FTUE";x=191;y=179;ftue_arrow(c,230,129,973,320,false,false,37,31);break;
 case FtueId::RelicReward:prefix="RELIC_FTUE";x=198;y=145;w=122;h=54;{const int ry=a.ftue.target==-2?124:104+RewardPresentation::row_offset(unsigned(mx(0,a.ftue.target)))-a.reward_presentation.list_scroll.offset();ftue_arrow(c,184,ry+20,0,1024,false,true);}break;
 case FtueId::Merchant:prefix="MERCHANT_FTUE";x=26;ftue_arrow(c,201,97);break;
 case FtueId::RestSite:prefix="REST_SITE_FTUE";y=140;break;
 case FtueId::Shuffle:prefix="SHUFFLE_FTUE";y=150;ftue_arrow(c,42,192);ftue_arrow(c,278,192,0,1024,true);break;
 default:return;
 }
 // Keep only the nine-pixel 4:3 overhang of the authored relic panel in
 // bounds. Text and controls retain their original relative geometry.
 x=mn(mx(0,x),320-w);y=mn(mx(0,y),240-h-2);
 ftue_material(c,I9_FtuePopup,x,y,w,h);
 char key[64]{};TextBuffer t{key,sizeof key,0};t.text(prefix);t.text(id==FtueId::MapSelect?"_TITLE":"_TITLE");
 Canvas title=c;title.font_face=source_label_face(c.a.content.english(),5);title.fitted(ftue_text(a,key),x+(w==146?19:16),y+(w==146?7:4),w-(w==146?41:23),11,5,5,Gold,false,false,true);
 t.n=0;key[0]=0;t.text(prefix);t.text("_DESCRIPTION");Canvas body=c;body.bold_face=5;
 const int bx=w==146?12:7,by=w==146?20:14,bw=w-bx-(w==146?10:7),bh=h-by-12;
 ftue_paragraph(body,ftue_text(a,key),x+bx,y+by,bw,bh,4,false,4);
 ftue_button(c,x+w/2-26,y+h-13,52,14,1,ftue_text(a,"CONFIRM_BUTTON"));
}
