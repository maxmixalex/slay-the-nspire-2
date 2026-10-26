// Locked ancient_event_layout / ancient_dialogue_line scenes, projected from
// NGame's 1680x1260 viewport. Text keeps a declared10px calculator minimum;
// its full line height grows and the clipped history remains reviewable.
namespace ancient_layout {
constexpr int ClipX=16,ClipY=81,ClipW=288,Bottom=234,NextBottom=217;
constexpr int OptionsX=20,OptionsW=280,OptionH=35,DialogueX=34;
constexpr int TextW=220,Font=10,TextStep=11;
inline int option_y(unsigned index){return int(index)*37;}
inline int options_height(unsigned count){return count?option_y(count-1)+OptionH:0;}
}
static unsigned ancient_font_face(const RichChar&ch){
 // Both ancient scenes explicitly use Kreon Bold; the dialogue additionally
 // selects Bitter Medium Italic. These UI8 families have their own metrics,
 // unlike the generic rich-text style bands used by other screens. Combined
 // bold/italic and inline icons retain the original theme/default selection.
 const unsigned style=rich_font_style(ch);return style==1?5u:style==2?6u:style;
}
static fitted_text::Metric ancient_text_metric(const Canvas&c,const RichChar&ch,int size){
 const auto glyph=c.a.content.glyph_metrics(ch.codepoint,size,ancient_font_face(ch));
 return {glyph.advance,mx(int(glyph.advance),int(glyph.w)+glyph.bearing_x),glyph.advance64,
  ch.previous_font==rich_font_style(ch)?c.a.content.glyph_kerning64(ch.previous,ch.codepoint,size,ancient_font_face(ch)):0};
}
static int ancient_text_rows(const Canvas&c,const char*text,int width){
 return fitted_text::line_count(text,width,ancient_layout::Font,[&](const RichChar&ch,int size){return ancient_text_metric(c,ch,size);});
}
static void ancient_text(Canvas&c,const char*text,int x,int y,int width,int rows,int total=0){
 if(rows<=0)return;
 const uint32_t rainbow_time=c.rainbow_elapsed(text,x,y,7);
 const auto metric=[&](const RichChar&ch,int size){return ancient_text_metric(c,ch,size);};
 if(!total)total=rows;
 const fitted_text::Plan plan{ancient_layout::Font,rows,total,ancient_layout::TextStep,total>rows};
 const uint16_t colors[]={rgb(255,246,226),Gold,rgb(244,122,102),Blue,Green,rgb(190,145,217),rgb(236,164,92),rgb(104,210,205),White,Muted};
 static const int wave[]={0,1,1,2,2,2,1,1,0,-1,-1,-2,-2,-2,-1,-1};
 fitted_text::visit(text,width,plan,metric,[&](const RichChar&ch,int at,int row,int size){
  int yy=y+row*ancient_layout::TextStep;if(yy+size+3<0||yy>=c.clip_h)return;
  const auto glyph=c.a.content.glyph(ch.codepoint,size,ancient_font_face(ch));
  if(c.a.text_effects&&ch.style.wave)yy+=wave[(c.a.animation/2+ch.index*2)%16];
  if(c.a.text_effects&&ch.style.thinky_dots)yy+=rich_thinky_offset(c.a.map_clock_ms,ch.index-ch.style.thinky_start);
  const int xx=x+at+(c.a.text_effects&&ch.style.jitter?int((c.a.animation/2+ch.index*7)%3)-1:0);
  const auto ink=c.a.text_effects&&ch.style.rainbow?rich_rainbow_rgb565(ch.style.rainbow_parameters,rainbow_time,at*26000/mx(1,size)):rich_color(ch.style.color,colors);
  Canvas shadow=c;shadow.opacity=uint8_t(div255(shadow.opacity*64));shadow.character(ch.codepoint,glyph,xx+1,yy,Ink,true);
  c.character(ch.codepoint,glyph,xx,yy,ink);
 });
}
static AncientButtonColor ancient_character_color(Character character){
 static constexpr AncientButtonColor colors[]={{0x59,0x07,0x00,255},{0x28,0x47,0x19,255},{0x13,0x44,0x6b,255},{0x52,0x37,0x1d,255},{0x6b,0x46,0x58,255}};
 return colors[unsigned(character)<CharacterKinds?unsigned(character):0];
}
static void ancient_tail(Canvas&c,int x,int y,bool right){
 // A 30-pixel local surface scales the existing original tail once and then
 // mirrors its atlas region; it does not add another permanent image/cache.
 uint16_t pixels[5*6]{};uint8_t alpha[5*6]{};
 Canvas tile{c.a,pixels,5,5,6,alpha};tile.ui(I7_AncientDialogueTail,0,0,5,6);
 Picture tail{reinterpret_cast<const uint8_t*>(pixels),alpha,nullptr,5,6,0};
 c.picture_region(tail,x,y,0,0,5,6,right);
}
static int ancient_measure(Canvas&c,unsigned options){
 auto&a=c.a;auto&d=a.ancient_dialogue;int total=0;
 if(!d.measured){
  char text[8192]{};
  for(unsigned i=0;i<d.count;++i){a.ancient_dialogue_text(i,text,sizeof text);
   d.heights[i]=uint16_t(mx(13,ancient_text_rows(c,text,ancient_layout::TextW)*ancient_layout::TextStep+4));}
 }
 for(unsigned i=0;i<d.count&&i<=d.current;++i)total+=int(d.heights[i])-(i?1:0);
 if(!d.measured){
  const int bottom=d.last()?ancient_layout::Bottom:ancient_layout::NextBottom;
  const int target=bottom-total-(d.last()?ancient_layout::options_height(options)+(d.count?2:0):0);
  d.move_to(target,a.map_clock_ms,!d.laid_out);d.laid_out=d.measured=true;
  d.max_scroll=int16_t(mx(0,ancient_layout::ClipY-target));
 }
 return total;
}
static void ancient_dialogue_lines(Canvas&c,Canvas&clipped,int top){
 auto&a=c.a;auto&d=a.ancient_dialogue;int y=top;bool history_hovered=false;
 for(unsigned i=0;i<d.current&&i<d.count;++i){
  if(a.pointer.visible&&a.pointer.x>=ancient_layout::DialogueX&&a.pointer.x<ancient_layout::DialogueX+258&&
     a.pointer.y>=mx(ancient_layout::ClipY,y)&&a.pointer.y<mn(ancient_layout::ClipY+clipped.clip_h,y+int(d.heights[i])))history_hovered=true;
  y+=int(d.heights[i])-1;
 }
 d.hover_history(history_hovered,a.map_clock_ms);y=top;
 char text[8192]{};
 for(unsigned i=0;i<d.count&&i<=d.current;++i){
  const int height=d.heights[i],local_y=y-ancient_layout::ClipY;
  if(local_y+height>0&&local_y<clipped.clip_h){
   Canvas line=clipped;const unsigned alpha=i==d.current?255*room_curve::linear(a.map_clock_ms-d.start,100)/room_curve::Q:d.history_alpha(a.map_clock_ms);
   line.opacity=uint8_t(div255(line.opacity*alpha));
   const bool character=d.character_lines&(1u<<i);const auto color=character?ancient_character_color(a.game.character):native_ancient_dialogue_color(a.game.original_events.id);
   const int bubble_x=(character?34:54)-ancient_layout::ClipX,tail_x=(character?264:51)-ancient_layout::ClipX;
   if(a.content.version()>=RoomUi7ContentVersion){
    line.ui(character?U_TopBarIronclad+int(a.game.character):I7_AncientSpeakerStart+int(native_ancient_index(a.game.original_events.id)),
      (character?274:32)-ancient_layout::ClipX,local_y+(height-16)/2,16,16);
    Canvas shadow=line;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;
    shadow.nine_ui(I7_AncientDialogue,bubble_x,local_y+1,232,height,6,6,6,6,64);
    shadow.opacity=uint8_t(div255(shadow.opacity*64));ancient_tail(shadow,tail_x+(character?1:0),local_y+(height-6)/2+1,character);
    Canvas bubble=line;bubble.tint_red=color.red;bubble.tint_green=color.green;bubble.tint_blue=color.blue;
    bubble.nine_ui(I7_AncientDialogue,bubble_x,local_y,232,height,6,6,6,6);
    ancient_tail(bubble,tail_x,local_y+(height-6)/2,character);
   }else line.dim(bubble_x,local_y,232,height,145);
   a.ancient_dialogue_text(i,text,sizeof text);const int rows=mx(1,(height-4)/ancient_layout::TextStep);
   ancient_text(line,text,(character?40:60)-ancient_layout::ClipX,local_y+(height-rows*ancient_layout::TextStep)/2,ancient_layout::TextW,rows);
  }
  y+=height-1;
 }
}
static void ancient_next_button(Canvas&c){
 auto&a=c.a;auto&d=a.ancient_dialogue;if(d.last())return;
 char key[AncientDialoguePresentation::KeyCapacity]{},label[512]{};
 unsigned last_dot=0,n=0;while(d.keys[d.current][n]&&n+1<sizeof key){key[n]=d.keys[d.current][n];if(key[n]=='.')last_dot=n;++n;}
 key[last_dot]=0;TextBuffer b{key,sizeof key,last_dot};b.text(".next");a.localized_text("ancients",key,label,sizeof label,int(a.game.original_events.id));
 Canvas next=c;const unsigned age=a.map_clock_ms-d.entered;
 next.opacity=uint8_t(div255(next.opacity*(age<=500?0:255*room_curve::linear(age-500,1000)/room_curve::Q)));
 next.font_face=6;
 const int width=mn(190,next.width(label,10)+2),x=284-width;next.text_fitted(label,x,222,width,10,rgb(239,200,81),10,false,true);
 char hotkey[16]{};input_key_label(a.input_bindings.keys[a.keyboard_only?1:0][unsigned(InputCommand::Confirm)],hotkey,sizeof hotkey);
 Canvas keycap=next;keycap.font_face=5;const int key_width=mx(14,mn(36,keycap.width(hotkey,8)+6));
 keycap.nine_ui(I8_KeyboardKeycap,x-key_width-5,220,key_width,14,5,0,5,0);
 keycap.text_fitted(hotkey,x-key_width-3,222,key_width-4,8,rgb(255,246,226),8,true);
 const unsigned phase=age%1000;const int displacement=phase<400?int(room_curve::sine_in_out(phase,400)):4096-2*int(room_curve::sine_in_out(phase-400,600));
 const int dx=displacement>=2688?1:displacement<=-2688?-1:0;
 Canvas arrow=next;arrow.tint_red=239;arrow.tint_green=200;arrow.tint_blue=81;
 arrow.ui(I8_AncientNextArrow,290+dx,220,8,14);
 // Source uses the whole DialogueHitbox. The HUD is drawn later and keeps
 // its own controls on top; releasing here advances only this UI sequence.
 c.hit(0,22,320,218,HitKind::AncientDialogueNext);
}
static void ancient_option(Canvas&c,Canvas&clipped,int option,int y){
 auto&a=c.a;const auto&e=a.game.original_events;auto&d=a.ancient_dialogue;
 const bool enabled=original_event_option_enabled(a.game,option);
 const int x=ancient_layout::OptionsX,w=ancient_layout::OptionsW,h=ancient_layout::OptionH;
 const bool selected=enabled&&(a.pointer.visible||a.map_clock_ms-d.start>=800)&&merchant_hovered(a,option,x,y,w,h);
 const bool pressed=selected&&a.pointer.held&&a.pointer.captured.kind==HitKind::Choice&&a.pointer.captured.value==option&&a.pointer.captured.enabled;
 const auto visual=a.room_presentation.event.buttons[option].observe(pressed?2:selected?1:0,a.map_clock_ms);
 const int width=w*visual.scale/4096,height=h*visual.scale/4096,left=x+(w-width)/2-ancient_layout::ClipX,top=y+(h-height)/2-ancient_layout::ClipY;
 int image=a.content.world_image("events","ANCIENT_OPTION");if(image<0)image=a.content.ui_image(U_EventButton);
 const auto color=native_ancient_button_color(e.id);Canvas button=clipped;
 button.tint_red=uint8_t(div255(button.tint_red*color.red));button.tint_green=uint8_t(div255(button.tint_green*color.green));button.tint_blue=uint8_t(div255(button.tint_blue*color.blue));
 // The ancient Image has no HSV shader; its original RGBA stays intact.
 button.picture(image,left,top,width,height,color.alpha);
 if(visual.glow&&enabled){
  image=a.content.world_image("events","ANCIENT_OPTION_OUTLINE");if(image<0)image=a.content.ui_image(U_EventOutline);
  Canvas outline=clipped;outline.tint_red=uint8_t(div255(outline.tint_red*120));outline.tint_blue=uint8_t(div255(outline.tint_blue*254));
  outline.picture(image,left-3,top-3,width+6,height+6,unsigned(visual.glow)*192/255,presentation::Add);
 }
 const bool icon=!e.finished&&option<e.offer_count&&e.offers[option]<WorldRelicKinds;
 if(icon)clipped.picture(a.relic_face_image(e.offers[option],int(a.game.character)),x+5-ancient_layout::ClipX,y+8-ancient_layout::ClipY,18,18);
 char label[256]{},description[4096]{},combined[4480]{};native_event_option(a,option,false,label,sizeof label);native_event_option(a,option,true,description,sizeof description);
 TextBuffer text{combined,sizeof combined,0};
 if(*label){text.text(enabled?"[gold][b]":"[red][b]");text.text(label);text.text(enabled?"[/b][/gold]":"[/b][/red]");if(*description)text.ch('\n');}text.text(description);
 Canvas words=clipped;if(!enabled)words.opacity=uint8_t(div255(words.opacity*179));
 const int inset=icon?29:10,tx=left+inset,ty=top+6,tw=width-inset-10,th=height-12;
 const auto plan=native_room_compact_plan(words,combined,tw,th);
 const int overflow=mx(0,plan.total*plan.step+2-th);
 native_room_scroll_text(words,combined,tx,ty,tw,th,
  selected?native_room_marquee_offset(overflow,a.map_clock_ms-a.room_presentation.event.buttons[option].start):0);
 const int hit_y=mx(ancient_layout::ClipY,y),bottom=mn(ancient_layout::ClipY+clipped.clip_h,y+h);
 if(bottom>hit_y)c.hit(x,hit_y,w,bottom-hit_y,HitKind::Choice,option,enabled);
}
