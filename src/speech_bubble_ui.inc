// NSpeechBubbleVfx is a room child, independent of the inventory rug's text.
// Project the source Control and per-glyph fly-in directly, with no surface.
static int speech_pixel(int q){return (q+(q<0?-2048:2048))/4096;}
struct SpeechRotation {
 int sine,cosine;
 static constexpr int PivotX=684*4*4096/21,PivotY=435*4*4096/21;
 void point(int&x,int&y)const{const int dx=x-PivotX,dy=y-PivotY;
  x=PivotX+(dx*cosine-dy*sine)/1024;y=PivotY+(dx*sine+dy*cosine)/1024;}
};
struct SpeechClip {
 SpeechRotation rotation;int x,y,w,h;
 bool contains(int px,int py)const{
  const int dx=px*4096-SpeechRotation::PivotX,dy=py*4096-SpeechRotation::PivotY;
  const int local_x=SpeechRotation::PivotX+(dx*rotation.cosine+dy*rotation.sine)/1024;
  const int local_y=SpeechRotation::PivotY+(-dx*rotation.sine+dy*rotation.cosine)/1024;
  return local_x>=x&&local_x<x+w&&local_y>=y&&local_y<y+h;
 }
};
static void speech_glyph(Canvas&c,Glyph glyph,int x,int y,int sine,int cosine,unsigned alpha,const SpeechClip*clip=nullptr){
 if(!glyph.pixels||!glyph.w||!glyph.h||!alpha)return;
 Canvas ink=c;ink.opacity=uint8_t(div255(ink.opacity*alpha));
 const int shadow_x=(2*4*4096/21)*cosine/1024-(2*4*4096/21)*sine/1024;
 const int shadow_y=(2*4*4096/21)*sine/1024+(2*4*4096/21)*cosine/1024;
 unsigned position=0;const unsigned pixels=unsigned(glyph.w)*glyph.h;
 const auto emit=[&](unsigned coverage){
  const int column=int(position%glyph.w),row=int(position/glyph.w);++position;if(!coverage)return;
  const int qx=x+(column*cosine-row*sine)*4,qy=y+(column*sine+row*cosine)*4;
  const int sx=speech_pixel(qx+shadow_x),sy=speech_pixel(qy+shadow_y),xx=speech_pixel(qx),yy=speech_pixel(qy);
  if(sx>=0&&sy>=0&&sx<ink.clip_w&&sy<ink.clip_h&&(!clip||clip->contains(sx,sy)))ink.paint(sy*ink.pitch+sx,0,div255(coverage*128));
  if(xx>=0&&yy>=0&&xx<ink.clip_w&&yy<ink.clip_h&&(!clip||clip->contains(xx,yy)))ink.paint(yy*ink.pitch+xx,rgb(255,246,226),coverage);
 };
 if(glyph.encoded_size){for(unsigned i=0;i+1<glyph.encoded_size&&position<pixels;i+=2)
  for(unsigned n=0;n<glyph.pixels[i]&&position<pixels;++n)emit(glyph.pixels[i+1]);}
 else for(unsigned i=0;i<pixels;++i)emit(glyph.pixels[i]);
}
static void fake_merchant_welcome(Canvas&c){
 auto&a=c.a;const auto&welcome=a.room_presentation.merchant.welcome;
 const auto frame=welcome.sample(a.map_clock_ms);if(!frame.alpha||!a.content.has_speech_bubble_assets())return;
 Canvas layer=c;layer.opacity=uint8_t(div255(layer.opacity*frame.alpha));layer.brightness=uint8_t(div255(layer.brightness*frame.brightness));
 const SpeechRotation rotation{CombatCardMotion::sine(frame.angle),CombatCardMotion::sine(frame.angle+1024)};
 // _Process replaces Contents' initial (-1,3) with (0,sin(t)*2).
 const int bob=room_curve::sine_turn(2047+frame.age*2934/1000)*8/21;
 int x=591*4*4096/21,y=387*4*4096/21+bob;
 int shadow_x=x+int(frame.scale)*54/21,shadow_y=y+int(frame.scale)*30/21;
 rotation.point(x,y);rotation.point(shadow_x,shadow_y);
 const auto picture=a.content.image(a.content.ui_image(I10_FakeMerchantWelcome));
 const int width=int(picture.w)*int(frame.scale)/4096,height=int(picture.h)*int(frame.scale)/4096;
 Canvas shadow=layer;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.opacity=uint8_t(div255(shadow.opacity*64));
 ui_picture_transform(shadow,picture,speech_pixel(shadow_x),speech_pixel(shadow_y),width,height,rotation.sine,rotation.cosine);
 ui_picture_transform(layer,picture,speech_pixel(x),speech_pixel(y),width,height,rotation.sine,rotation.cosine);
 char key[80]{},text[512]{};TextBuffer b{key,sizeof key,0};b.text("FAKE_MERCHANT.talk.welcome.line");b.number(welcome.line);
 a.localized_text("events",key,text,sizeof text);if(!*text)return;
 layer.bold_face=5;layer.opacity=uint8_t(div255(layer.opacity*230));
 const auto metric=[&](const RichChar&ch,int px){return layer.text_metric(ch,px);};
 auto plan=native_room_compact_plan(layer,text,53,25);
 const int overflow=mx(0,plan.total*plan.step+2-25),scroll=text_overflow_offset(frame.age,overflow,12),first=scroll/plan.step;
 // Keep the original per-glyph fly-in and root rotation, but expose the
 // complete message through its own rotated text viewport instead of
 // replacing the last visible line with an ellipsis.
 plan.rows=mn(plan.total,first+4);plan.truncated=false;int widths[4]{};
 fitted_text::visit(text,53,plan,metric,[&](const RichChar&ch,int at,int row,int px){if(row>=first)widths[row-first]=at+metric(ch,px).width;});
 const int top=overflow?-scroll:mx(0,(25-(plan.total*plan.step+2))/2);
 const SpeechClip clip{rotation,451*4*4096/21,303*4*4096/21+bob,53*4096,25*4096};
 const unsigned visible=room_rich_characters(text)*frame.reveal/4096;
 fitted_text::visit(text,53,plan,metric,[&](const RichChar&ch,int at,int row,int px){
  if(row<first||ch.index>=visible)return;
  // RichTextFlyIn: elapsed*3-index*.015, QuadOut translation/rotation.
  const int elapsed=int(frame.age)*3-int(ch.index)*15;
  const unsigned q=unsigned(mn(4096,mx(0,elapsed*4096/1000))),inverse=4096-q;
  const unsigned offset=a.text_effects?room_curve::multiply(inverse,inverse):0;
  const int angle=a.text_effects?-228*int(4096-room_curve::multiply(q,q))/4096:0;
  const int sine=CombatCardMotion::sine(angle),cosine=CombatCardMotion::sine(angle+1024);
  const int combined_sine=(rotation.sine*cosine+rotation.cosine*sine)/1024;
  const int combined_cosine=(rotation.cosine*cosine-rotation.sine*sine)/1024;
  int xx=451*4*4096/21+(at+mx(0,(53-widths[row-first])/2))*4096+int(offset)*59*4/21;
  int yy=303*4*4096/21+(top+row*plan.step)*4096+bob+int(offset)*39*4/21;
  rotation.point(xx,yy);
  speech_glyph(layer,a.content.glyph(ch.codepoint,px,layer.font_style(ch)),xx,yy,combined_sine,combined_cosine,255*q/4096,overflow?&clip:nullptr);
 });
}
