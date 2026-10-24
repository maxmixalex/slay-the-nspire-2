// v0.111.0 NPowerAppliedVfx / NPowerRemovedVfx and NDamageNumVfx.
// Timings are authored milliseconds; positions are adapted to 320x240.
static void combat_power_flashes(Canvas&c){
 const auto&a=c.a;const auto&v=a.combat_status;
 for(unsigned i=0;i<v.flash_count;++i){const auto&f=v.flashes[i];const int ms=int32_t(a.animation-f.at)*33;
  if(ms<0||ms>=1050||f.actor>unsigned(a.game.enemy_count))continue;
  const uint32_t identity=ui_feedback_bound(a)?a.combat_feedback.identity[f.actor]:ui_feedback_identity(a.game,f.actor);if(identity!=f.identity)continue;
  const auto pose=f.actor?a.combat_enemy_pose(f.actor-1):a.combat_player_pose();const int x=pose.center_x,y=pose.center_y;
  const auto p=a.content.image(ui_status_image(a,f.id));if(!p.pixels||p.format>3)continue;
  // NPowerFlashVfx's two initial tweens run in parallel for400ms. The
  // subsequent scale400ms and opacity250ms run sequentially: total1050ms.
  const unsigned grow=ms<=400?0u:ms>=800?4096u:unsigned(ms-400)*4096u/400u;
  const int size=17+int(grow*2u/4096u),left=x-size/2,top=y-size/2;
  const unsigned opacity=ms<=800?255u:255u*CombatCardMotion::sine(unsigned(1050-ms)*1024u/250u)/1024u;
  for(int yy=mx(0,-top);yy<mn(size,c.clip_h-top);++yy)for(int xx=mx(0,-left);xx<mn(size,c.clip_w-left);++xx){
   const unsigned at=unsigned(yy*p.h/size)*p.w+unsigned(xx*p.w/size);const uint8_t*pixel=p.palette?p.palette+2*p.pixels[at]:p.pixels+2*at;
   const uint16_t color=uint16_t(pixel[0]|uint16_t(pixel[1])<<8);const unsigned alpha=p.alpha?p.alpha[at]:255;
   // power_flash_vfx.gdshader samples the icon twice, multiplies RGB by3
   // and uses blend_add. Preserve this instead of a white rectangular glow.
   const unsigned r=color>>11,g=(color>>5)&63,b=color&31;
   const uint16_t lit=uint16_t((mn(31,int(r*r*3u/31u))<<11)|(mn(63,int(g*g*3u/63u))<<5)|mn(31,int(b*b*3u/31u)));
   c.paint((top+yy)*c.pitch+left+xx,lit,div255(div255(alpha*alpha)*opacity),presentation::Add);
  }
 }
}
static void combat_power_popups(Canvas&c){
 const auto&a=c.a;const auto&v=a.combat_status;
 int occupied[CombatStatus::Actors][3]{},heights[CombatStatus::Actors][3]{};uint8_t rows[CombatStatus::Actors]{};
 const auto place_title=[&](unsigned actor,int desired,int height){
  // Preserve the native up/down motion until two titles meet. At the
  // calculator's scale, pack their small text rectangles around that point;
  // icons retain their authored motion and no status is painted over another.
  desired=mx(25,mn(139-height,desired));int chosen=desired;
  for(int delta=0;delta<115;++delta){bool found=false;
   for(unsigned side=0;side<2;++side){const int y=desired+(side?delta:-delta);if(y<25||y+height>139)continue;bool clear=true;
    for(unsigned row=0;row<rows[actor];++row)if(y<occupied[actor][row]+heights[actor][row]+2&&y+height+2>occupied[actor][row])clear=false;
    if(clear){chosen=y;found=true;break;}
   }if(found)break;
  }
  if(rows[actor]<3){const unsigned row=rows[actor]++;occupied[actor][row]=chosen;heights[actor][row]=height;}return chosen;
 };
 for(unsigned i=0;i<v.popup_count;++i){const auto&p=v.popups[i];const int ms=int32_t(a.animation-p.at)*33;
  if(ms<0||ms>=(p.removed?1750:1250)||p.actor>unsigned(a.game.enemy_count))continue;
  const uint32_t identity=ui_feedback_bound(a)?a.combat_feedback.identity[p.actor]:ui_feedback_identity(a.game,p.actor);
  if(identity!=p.identity)continue;
  const auto pose=p.actor?a.combat_enemy_pose(p.actor-1):a.combat_player_pose();const int x=pose.center_x,cy=pose.center_y-int(p.lane)*18;
  if(p.removed){
   const unsigned alpha=ms<250?TurnBannerPresentation::ease(unsigned(ms),250):ms<750?4096u:unsigned(1750-ms)*4096u/1000u;
   const unsigned rest=unsigned(2000-ms)*1024u/2000u,q2=rest*rest/1024u,moved=1024u-q2*q2/1024u;
   const int y=place_title(p.actor,cy-24-int(moved*32u/1024u),21);
   Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/4096u);
   const char*title=ui_status_title(a,p.id);const int width=mn(110,c.width(title,10)),left=mx(18,mn(318-width,x-width/2));
   text.picture(ui_status_image(a,p.id),left-16,y+2,13,13,192);
   health_label(text,title,left,y,width,10,rgb(255,246,226),Ink);
   const char*ended=a.content.world_string("vfx","POWER_WEARS_OFF");if(!*ended)ended=a.content.english()?"Wears Off":"效果结束";
   text.text(ended,left,y+11,8,rgb(255,246,226),110);continue;
  }
  // Icon + echo use the authored 0.4 -> 0.8 scale and low opacity.
  const unsigned growth=TurnBannerPresentation::ease(unsigned(ms),1250);const int size=17+int(growth*17/4096);
  const unsigned icon_alpha=ms<250?unsigned(64*CombatCardMotion::sine(ms*1024/250)/1024):unsigned(64*CombatCardMotion::sine((1250-ms)*1024/1000)/1024);
  c.picture(ui_status_image(a,p.id),x-size/2,cy-size/2,size,size,icon_alpha);
  c.picture(ui_status_image(a,p.id),x-size*11/20,cy-size*11/20,size*11/10,size*11/10,icon_alpha/2);
  if(ms>=1000)continue;
  const unsigned alpha=ms<250?TurnBannerPresentation::ease(unsigned(ms),250):TurnBannerPresentation::ease(unsigned(1000-ms),750);
  const unsigned moved=CombatCardMotion::cubic(unsigned(ms),1250);const int y=place_title(p.actor,cy-33+(p.buff?-1:1)*int(moved*17/1024),11);
  const char*title=ui_status_title(a,p.id);const int width=mn(130,c.width(title,10)),left=mx(2,mn(318-width,x-width/2));
  Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/4096);
  health_label(text,title,left,y,width,10,p.buff?rgb(127,255,0):rgb(255,85,85),Ink);
 }
}
// Rasterize each fetched glyph once; scaling and outline share this small
// alpha-only scratch line. There is no new sprite pack or persistent cache.
static void combat_damage_text(Canvas&c,const char*label,int cx,int cy,int scale,uint16_t color){
 constexpr int W=128,H=40;uint8_t mask[W*H]{};
 int width=0,height=0;
 for(const char*p=label;*p;){const auto g=c.a.content.glyph(utf8(p),16);if(width+mx(g.w,g.advance)>=W)break;
  if(g.pixels){unsigned pos=0;
   const auto pixel=[&](unsigned at,unsigned alpha){const unsigned yy=at/g.w,xx=at-yy*g.w;if(yy<H)mask[yy*W+width+xx]=uint8_t(alpha);};
   if(g.encoded_size){for(unsigned at=0;at+1<g.encoded_size&&pos<unsigned(g.w)*g.h;at+=2){unsigned n=mn(g.pixels[at],int(unsigned(g.w)*g.h-pos));const unsigned alpha=g.pixels[at+1];while(n--)pixel(pos++,alpha);}}
   else for(unsigned at=0;at<unsigned(g.w)*g.h;++at)pixel(at,g.pixels[at]);
  }width+=g.advance;height=mx(height,g.h);
 }
 if(!width||!height||scale<=0)return;
 const int w=mx(1,width*scale/1024),h=mx(1,height*scale/1024),left=cx-w/2,top=cy-h/2;
 int columns[320];const int start=mx(-2,-left),end=mn(w+2,c.clip_w-left);
 for(int x=start;x<end;++x)columns[x-start]=health_bar::bounded(x*1024/scale,0,width-1);
 for(int y=mx(-2,-top);y<mn(h+2,c.clip_h-top);++y){
  const int sy=health_bar::bounded(y*1024/scale,0,height-1);
  for(int x=start;x<end;++x){const int sx=columns[x-start];unsigned alpha=0,outline=0;
   if(x>=0&&x<w&&y>=0&&y<h)alpha=mask[sy*W+sx];
   for(int yy=mx(0,sy-1);yy<=mn(height-1,sy+1);++yy)for(int xx=mx(0,sx-1);xx<=mn(width-1,sx+1);++xx)outline=mx(outline,mask[yy*W+xx]);
   const int at=(top+y)*c.pitch+left+x;if(outline)c.paint(at,rgb(54,51,47),outline);if(alpha)c.paint(at,color,alpha);
  }
 }
}
static void combat_damage_label(Canvas&c,int value,int cx,int cy,int scale,uint16_t color){
 char label[16]{};TextBuffer b{label,sizeof label,0};b.number(value);combat_damage_text(c,label,cx,cy,scale,color);
}
static void combat_damage_popups(Canvas&c){
 const auto&a=c.a;const auto&v=a.combat_status;
 for(unsigned i=0;i<v.damage_count;++i){const auto&d=v.damages[i];const int ms=int32_t(a.animation-d.at)*33;
  const bool pet=d.actor==MaxEnemies+1;
  if(ms<0||ms>=2000||(!pet&&d.actor>unsigned(a.game.enemy_count)))continue;
  const uint32_t identity=ui_feedback_bound(a)?a.combat_feedback.identity[d.actor]:ui_feedback_identity(a.game,d.actor);if(identity!=d.identity)continue;
  const auto p=pet?a.combat_player_pose(true):d.actor?a.combat_enemy_pose(d.actor-1):a.combat_player_pose();
  const int origin_x=p.center_x,origin_y=p.center_y;
  if(d.flags&CoreDamageResult::FullyBlocked){
   // NDamageBlockedVfx: 2s Quad.Out rise/scale, Cubic.Out blue->white,
   // with a delayed Sine.In fade. It is a localized word, never HP delta 0.
   const unsigned q=unsigned(ms)*1024u/2000u,progress=1024u-(1024u-q)*(1024u-q)/1024u;
   const int cx=origin_x+(int(d.variation%41u)-20)*4/21;
   const int cy=origin_y-10+int(d.variation%21u)*4/21-int(progress*250u*4u/(1024u*21u));
   const int scale=(1024-int(progress*410u/1024u))*64*4/(21*16);
   const unsigned alpha=ms<=500?255u:255u*CombatCardMotion::sine(unsigned(2000-ms)*1024u/1500u)/1024u;
   Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/255u);
   const unsigned color=CombatCardMotion::cubic(unsigned(ms),2000u)*255u/1024u;
   const char*label=a.content.world_string("vfx","BLOCKED");if(!*label)label=a.content.english()?"Blocked":"格挡";
   combat_damage_text(text,label,cx,cy,scale,blend(rgb(33,192,255),rgb(255,246,226),color));continue;
  }
  // The original integration uses 2000px/s^2 gravity, initial vertical
  // velocity -800..-700px/s, and a small independent horizontal scatter.
  const int cx=origin_x+int(d.vx)*ms*4/21000+int(d.variation%5)-2;
  const int cy=origin_y+(int(d.vy)*ms+ms*ms)*4/21000;
  const unsigned u=unsigned(ms)*1024u/2000u,alpha=255u*(1024u-u*u/1024u)/1024u;
  const unsigned rest=ms<1200?unsigned(1200-ms)*1024u/1200u:0u;
  const int base=(1200+int(d.variation))*64*1024*4/(1000*21*16),scale=base*(1024+int(1536u*rest/1024u*rest/1024u))/1024;
  if(cy>250||cy<-50)continue;
  Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/255u);
  const unsigned color=CombatCardMotion::cubic(unsigned(ms),500u)*255u/1024u;
  combat_damage_label(text,d.amount,cx,cy,scale,blend(rgb(247,42,20),rgb(255,246,226),color));
 }
}
