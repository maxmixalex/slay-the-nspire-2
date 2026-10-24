// v0.111.0 NPowerAppliedVfx / NPowerRemovedVfx and NDamageNumVfx.
// Timings are authored milliseconds; positions are adapted to 320x240.
static void combat_power_popups(Canvas&c){
 const auto&a=c.a;const auto&v=a.combat_status;
 for(unsigned i=0;i<v.popup_count;++i){const auto&p=v.popups[i];const int ms=int32_t(a.animation-p.at)*33;
  if(ms<0||ms>=(p.removed?1750:1250)||p.actor>unsigned(a.game.enemy_count))continue;
  const uint32_t identity=ui_feedback_bound(a)?a.combat_feedback.identity[p.actor]:ui_feedback_identity(a.game,p.actor);
  if(identity!=p.identity)continue;
  const auto pose=enemy_pose(a.game,p.actor?p.actor-1:0);const int x=p.actor?pose.x:60,cy=(p.actor?pose.feet-32:91)-int(p.lane)*18;
  if(p.removed){
   const unsigned alpha=ms<250?TurnBannerPresentation::ease(unsigned(ms),250):ms<750?4096u:unsigned(1750-ms)*4096u/1000u;
   const unsigned rest=unsigned(2000-ms)*1024u/2000u,q2=rest*rest/1024u,moved=1024u-q2*q2/1024u;
   const int y=cy-24-int(moved*32u/1024u);
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
  const unsigned moved=CombatCardMotion::cubic(unsigned(ms),1250);const int y=cy-33+(p.buff?-1:1)*int(moved*17/1024);
  const char*title=ui_status_title(a,p.id);const int width=mn(130,c.width(title,10)),left=mx(2,mn(318-width,x-width/2));
  Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/4096);
  health_label(text,title,left,y,width,10,p.buff?rgb(127,255,0):rgb(255,85,85),Ink);
 }
}
// Rasterize each fetched glyph once; scaling and outline share this small
// alpha-only scratch line. There is no new sprite pack or persistent cache.
static void combat_damage_label(Canvas&c,int value,int cx,int cy,int scale,uint16_t color){
 constexpr int W=128,H=40;uint8_t mask[W*H]{};
 char label[16]{};TextBuffer b{label,sizeof label,0};b.number(value);int width=0,height=0;
 for(const char*p=label;*p;++p){const auto g=c.a.content.glyph(unsigned(*p),16);if(width+g.w>=W)break;
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
static void combat_damage_popups(Canvas&c){
 const auto&a=c.a;const auto&v=a.combat_status;
 for(unsigned i=0;i<v.damage_count;++i){const auto&d=v.damages[i];const int ms=int32_t(a.animation-d.at)*33;
  if(ms<0||ms>=2000||d.actor>unsigned(a.game.enemy_count))continue;
  const uint32_t identity=ui_feedback_bound(a)?a.combat_feedback.identity[d.actor]:ui_feedback_identity(a.game,d.actor);if(identity!=d.identity)continue;
  const auto p=enemy_pose(a.game,d.actor?d.actor-1:0);
  // The original integration uses 2000px/s^2 gravity, initial vertical
  // velocity -800..-700px/s, and a small independent horizontal scatter.
  const int cx=(d.actor?p.x:60)+int(d.vx)*ms/6000+int(d.variation%5)-2;
  const int cy=(d.actor?p.feet-30:91)+(int(d.vy)*ms+ms*ms)*218/1080000;
  const unsigned u=unsigned(ms)*1024u/2000u,alpha=255u*(1024u-u*u/1024u)/1024u;
  const unsigned rest=ms<1200?unsigned(1200-ms)*1024u/1200u:0u;
  const int base=(1200+int(d.variation))*64*1024/(1000*6*16),scale=base*(1024+int(1536u*rest/1024u*rest/1024u))/1024;
  if(cy>250||cy<-50)continue;
  Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/255u);
  const unsigned color=CombatCardMotion::cubic(unsigned(ms),500u)*255u/1024u;
  combat_damage_label(text,d.amount,cx,cy,scale,blend(rgb(247,42,20),rgb(255,246,226),color));
 }
}
