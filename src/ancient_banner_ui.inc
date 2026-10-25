static void ancient_scaled_glyph(Canvas&c,Glyph glyph,int x,int y,unsigned scale,uint16_t color){
 if(!glyph.pixels||!scale)return;
 uint8_t mask[40*40];const unsigned length=unsigned(glyph.w)*glyph.h;
 if(!length||length>sizeof mask)return;
 if(glyph.encoded_size){unsigned at=0;
  for(unsigned i=0;i+1<glyph.encoded_size&&at<length;i+=2){const unsigned end=mn(length,at+glyph.pixels[i]);while(at<end)mask[at++]=glyph.pixels[i+1];}
  while(at<length)mask[at++]=0;
 }else for(unsigned i=0;i<length;++i)mask[i]=glyph.pixels[i];
 const int width=mx(1,int((glyph.w*scale+2048)/4096));
 uint8_t columns[64];if(width>64)return;for(int xx=0;xx<width;++xx)columns[xx]=uint8_t(xx*glyph.w/width);
 for(int yy=mx(0,-y);yy<mn(glyph.h,c.clip_h-y);++yy)for(int xx=mx(0,-x);xx<mn(width,c.clip_w-x);++xx){
  const unsigned alpha=mask[yy*glyph.w+columns[xx]];if(alpha)c.paint((y+yy)*c.pitch+x+xx,color,alpha);
 }
}
static void native_ancient_banner(Canvas&c){
 auto&a=c.a;auto&motion=a.ancient_banner;motion.observe(a.game,a.map_clock_ms);
 const uint32_t age=a.map_clock_ms-motion.start;const auto id=a.game.original_events.id;
 char title[256]{},key[96]{};const char*text=a.content.world_title("ancients",original_event_key(id));
 unsigned n=0;for(;*text&&n+1<sizeof title;++text)title[n++]=*text>='a'&&*text<='z'?char(*text-'a'+'A'):*text;
 TextBuffer b{key,sizeof key,0};b.text(original_event_key(id));b.text(".epithet");
 const char*epithet=a.content.world_string("ancients",key);
 Canvas name=c,description=c;name.font_face=source_label_face(a.content.english(),4);description.font_face=source_label_face(a.content.english(),6);
 if(age>=5500){
  const int x=int(48*4*AncientBannerPresentation::circ(age-5500,2000)/(21*4096));
  name.text(title,x,219,10,rgb(255,246,226),198);
  description.opacity=uint8_t(div255(description.opacity*motion.epithet_alpha(a.map_clock_ms)));
  description.text(epithet,x,227,4,rgb(255,246,226),198);return;
 }
 const int root=-200+100*int(AncientBannerPresentation::circ(age,4000))/4096;
 const int offset=18+(age<=1000?0:24*int(room_curve::cubic_out(age-1000,2000))/4096);
 const int center=(630+root+offset-42)*4/21,y=center-7;
 const unsigned spacing=1000*4*(4096-room_curve::expo_out(age,3000))/21;
 const unsigned scale=a.text_effects?AncientBannerPresentation::spring(age):4096;
 const unsigned red=age<4500?0:room_curve::expo_out(age-4500,1000);
 const auto title_color=rgb(239,200*(4096-red)/4096,81*(4096-red)/4096);
 const auto epithet_color=rgb(135,206*(4096-red)/4096,235*(4096-red)/4096);
 name.opacity=uint8_t(div255(name.opacity*motion.alpha(a.map_clock_ms)));
 const int width=name.width(title,14);int before=0,count=0,center_index=0;
 const char*p=title;while(*p){const auto cp=utf8(p);const auto metric=a.content.glyph_metrics(cp,14,name.font_face);
  if(!center_index&&2*(before+metric.advance)>width)center_index=count*4096+(width*2048-before*4096)/mx(1,metric.advance);
  before+=metric.advance;++count;
 }
 p=title;int x=(320-width)/2,index=0;
 while(*p){const auto cp=utf8(p);const auto glyph=a.content.glyph(cp,14,name.font_face);
  const int dx=a.text_effects?int(int64_t(index*4096+2048-center_index)*spacing/(4096*4096)):0;
  Canvas letter=name;if(!a.text_effects)letter.opacity=uint8_t(div255(letter.opacity*unsigned(mn(255,mx(0,int(age*765/1000)-index*4)))));
  Canvas shadow=letter;shadow.opacity=uint8_t(div255(shadow.opacity*96));
  ancient_scaled_glyph(shadow,glyph,x+dx+1,y+1,scale,rgb(0,0,0));
  ancient_scaled_glyph(letter,glyph,x+dx,y,scale,title_color);
  x+=glyph.advance;++index;
 }
 description.opacity=uint8_t(div255(description.opacity*motion.epithet_alpha(a.map_clock_ms)));
 description.centered(epithet,0,(630+root+offset)*4/21-3,320,4,epithet_color);
}
