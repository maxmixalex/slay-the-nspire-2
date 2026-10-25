// Event options and merchant dialogue retain rich-bold text and vertical
// alignment, with a10px LCD floor instead of shrinking into unreadable text.
static fitted_text::Plan native_room_compact_plan(const Canvas&c,const char*text,int w,int h){
 const auto metric=[&](const RichChar&ch,int size){return c.text_metric(ch,size);};
 const int minimum=c.a.content.font_size(10);int size=c.a.content.font_size(10);
 fitted_text::Plan plan;
 for(;;){
  const int total=fitted_text::line_count(text,w,size,metric),rows=mx(0,(h-2)/(size+1));
  plan={size,mn(rows,total),total,size+1,total>rows};
  if(!plan.truncated||size<=minimum)break;
  const int next=c.a.content.font_size(size-1,0,true);if(next>=size)break;size=mx(minimum,next);
 }
 return plan;
}
static fitted_text::Plan native_room_compact_text(Canvas c,const char*text,int x,int y,int w,int h,bool centered=false){
 c.bold_face=5;
 const auto metric=[&](const RichChar&ch,int size){return c.text_metric(ch,size);};
 auto plan=native_room_compact_plan(c,text,w,h);
 if(!plan.rows||x<0||y<0||x>=c.clip_w||y>=c.clip_h)return plan;
 int widths[32]{};plan.rows=mn(plan.rows,32);
 if(centered)fitted_text::visit(text,w,plan,metric,[&](const RichChar&ch,int at,int row,int px){widths[row]=at+metric(ch,px).width;});
 Canvas clipped=c;clipped.pixels+=y*c.pitch+x;clipped.clip_w=mn(w,c.clip_w-x);clipped.clip_h=mn(h,c.clip_h-y);
 Canvas shadow=clipped;shadow.opacity=uint8_t(div255(shadow.opacity*64));
 const int top=mx(0,(h-(plan.rows*plan.step+2))/2);
 const uint16_t colors[]={rgb(255,246,226),Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
 static const int wave[]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
 fitted_text::visit(text,w,plan,metric,[&](const RichChar&ch,int at,int row,int px){
  if(ch.index>=c.text_visible_characters)return;
  const auto glyph=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));
  int xx=at+(centered?mx(0,(w-widths[row])/2):0),yy=top+row*plan.step;
  if(c.a.text_effects&&ch.style.wave)yy+=wave[(c.a.animation/2+ch.index*2)%16];
  if(c.a.text_effects&&ch.style.thinky_dots)yy+=rich_thinky_offset(c.a.map_clock_ms,ch.index-ch.style.thinky_start);
  if(c.a.text_effects&&ch.style.jitter)xx+=int((c.a.animation/2+ch.index*7)%3)-1;
  const auto ink=c.a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,c.a.map_clock_ms,at*26000/mx(1,px)):rich_color(ch.style.color,colors);
  shadow.animated_character(ch,glyph,xx+1,yy,0,true);clipped.animated_character(ch,glyph,xx,yy,ink);
 });return plan;
}
// A clipped full paragraph supplies both event scrolling and focused option
// marquees. No glyph is painted outside the visible source texture interior.
static int native_room_scroll_text(Canvas c,const char*text,int x,int y,int w,int h,int offset,int size=10){
 c.bold_face=5;size=c.a.content.font_size(size);
 const auto metric=[&](const RichChar&ch,int px){return c.text_metric(ch,px);};
 const int rows=fitted_text::line_count(text,w,size,metric),step=size+1;
 const int left=mx(0,x),top=mx(0,y),right=mn(c.clip_w,x+w),bottom=mn(c.clip_h,y+h);
 if(w<=0||h<=0||right<=left||bottom<=top)return rows*step+2;
 c.pixels+=top*c.pitch+left;if(c.coverage)c.coverage+=top*c.pitch+left;c.clip_w=right-left;c.clip_h=bottom-top;
 const fitted_text::Plan plan{size,rows,rows,step,false};
 const uint16_t colors[]={rgb(255,246,226),Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
 fitted_text::visit(text,w,plan,metric,[&](const RichChar&ch,int at,int row,int px){
  int xx=x-left+at,yy=y-top+row*step-offset;if(yy+px+3<0||yy>=c.clip_h||ch.index>=c.text_visible_characters)return;
  static const int wave[]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
  if(c.a.text_effects&&ch.style.wave)yy+=wave[(c.a.animation/2+ch.index*2)%16];
  if(c.a.text_effects&&ch.style.thinky_dots)yy+=rich_thinky_offset(c.a.map_clock_ms,ch.index-ch.style.thinky_start);
  if(c.a.text_effects&&ch.style.jitter)xx+=int((c.a.animation/2+ch.index*7)%3)-1;
  const auto color=c.a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,c.a.map_clock_ms,at*26000/mx(1,px)):rich_color(ch.style.color,colors);
  const auto glyph=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));
  Canvas shadow=c;shadow.opacity=uint8_t(div255(shadow.opacity*64));shadow.character(ch.codepoint,glyph,xx+1,yy,0,true);
  c.character(ch.codepoint,glyph,xx,yy,color);
 });return rows*step+2;
}
static int native_room_marquee_offset(int overflow,uint32_t age){
 if(overflow<=0)return 0;
 // One second to read each end; twelve pixels/second keeps 10px lines calm.
 const uint32_t travel=uint32_t(overflow)*1000/12,cycle=2000+travel*2,t=age%cycle;
 if(t<1000)return 0;if(t<1000+travel)return int((t-1000)*12/1000);
 if(t<2000+travel)return overflow;return mx(0,overflow-int((t-2000-travel)*12/1000));
}
