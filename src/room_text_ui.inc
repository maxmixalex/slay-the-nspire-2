// The event option and merchant dialogue RichTextLabels can shrink below the
// shared card fitter's 5px floor. Keep their original projected 5 -> 4px band,
// rich-bold family and vertical alignment without adding a raster/cache pool.
static fitted_text::Plan native_room_compact_plan(const Canvas&c,const char*text,int w,int h){
 const auto metric=[&](const RichChar&ch,int size){const auto g=c.a.content.glyph_metrics(ch.codepoint,size,c.font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.advance),int(g.w))};};
 const int minimum=c.a.content.font_size(4);int size=c.a.content.font_size(5);
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
 const auto metric=[&](const RichChar&ch,int size){const auto g=c.a.content.glyph_metrics(ch.codepoint,size,c.font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.advance),int(g.w))};};
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
  const auto ink=c.a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,c.a.map_clock_ms,at*26000/mx(1,px)):colors[ch.style.color];
  shadow.character(ch.codepoint,glyph,xx+1,yy,0,true);clipped.character(ch.codepoint,glyph,xx,yy,ink);
 });return plan;
}
