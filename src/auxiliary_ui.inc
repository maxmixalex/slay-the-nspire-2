static const char*auxiliary_label(App&a,const char*key,const char*eng,const char*zhs){
 const char*text=a.content.world_string("settings_ui",key);return *text?text:a.content.english()?eng:zhs;
}
static const char*auxiliary_port_label(App&a,const char*key,const char*eng,const char*zhs){
 const char*text=a.content.world_string("auxiliary",key);return *text?text:a.content.english()?eng:zhs;
}
static void auxiliary_rounded(Canvas&c,int x,int y,int w,int h,int radius,uint16_t ink){
 for(int row=0;row<h;++row){const int edge=row<radius?radius-row-1:row>=h-radius?row-(h-radius):0;
  c.rect(x+edge,y+row,w-edge*2,1,ink);}
}
static unsigned auxiliary_alpha(const App&a){
 const auto&v=a.auxiliary;if(v.closing){const unsigned duration=v.screen==AuxiliaryScreen::Credits?1000u:250u,age=a.map_clock_ms-v.closing_at;
  return age>=duration?0:255u-age*255u/duration;}
 const unsigned duration=v.screen==AuxiliaryScreen::Credits?2000u:v.screen==AuxiliaryScreen::PatchNotes?250u:1u,age=a.map_clock_ms-v.opened;
 return age>=duration?255:age*255u/duration;
}
static void auxiliary_patch_button(Canvas&c){
 auto&a=c.a;const bool open=a.auxiliary.screen==AuxiliaryScreen::PatchNotes;
 const bool hover=a.pointer.visible&&a.pointer.x>=303&&a.pointer.x<315&&a.pointer.y>=3&&a.pointer.y<15;
 Canvas icon=c;if(!hover)icon.brightness=204;
 if(a.content.ui_image(I8_PatchNotesIcon)>=0)icon.ui(I8_PatchNotesIcon,303,3,12,12);
 else icon.centered("...",301,2,16,12,White);
 c.hit(303,3,12,12,HitKind::AuxiliaryControl,open?AuxiliaryBack:AuxiliaryOpenPatchNotes,!a.auxiliary.closing);
}
static void auxiliary_date(uint32_t date,char*out,unsigned capacity){
 static const char*const months[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
 const unsigned month=date/100%100;TextBuffer text{out,capacity,0};text.text(month>=1&&month<=12?months[month-1]:"");
 text.ch(' ');text.number(int(date%100));text.text(", ");text.number(int(date/10000));
}
static void auxiliary_notes_screen(Canvas&c){
 auto&a=c.a;auto&v=a.auxiliary;title_background(c);title_logo(c);c.dim(0,0,320,240,210);a.hit_count=0;
 Canvas layer=c;layer.opacity=uint8_t(auxiliary_alpha(a));layer.bold_face=5;const auto&note=AuxiliaryNotes[v.note];
 auxiliary_measure_note(a);
 const int offset=v.scroll.offset();char date[48]{};auxiliary_date(note.date,date,sizeof date);
 layer.text_fitted(date,68,8-offset,184,10,Gold,8,true,true);
 int top=28-offset;
 for(unsigned i=0;i<note.chunks;++i){const int height=v.note_lines[i]*11;
  if(top<240&&top+height>0){const int skip=top<0?(-top)/11:0;
   layer.paragraph(a.content.string(note.text+i),68,top+skip*11,184,mn(int(v.note_lines[i])-skip,(240-mx(0,top))/11+2),skip,8,rgb(255,246,226));}
  top+=height;
 }
 c.hit(60,0,220,240,HitKind::AuxiliaryControl,AuxiliaryScrollArea,!v.closing);
 if(v.note+1<AuxiliaryNoteCount){layer.ui(I8_PatchNotesArrow,23,108,24,24);c.hit(23,108,24,24,HitKind::AuxiliaryControl,AuxiliaryPrevious,!v.closing);}
 if(v.note){const auto image=a.content.image(a.content.ui_image(I8_PatchNotesArrow));layer.picture_region(image,273,108,0,0,24,24,true);c.hit(273,108,24,24,HitKind::AuxiliaryControl,AuxiliaryNext,!v.closing);}
 if(v.scroll.maximum){
  layer.ui(I6_ScrollbarEdge,303,25,8,8);ui_picture_transform(layer,a.content.ui_image(I6_ScrollbarEdge),307,211,8,8,0,-1024);
  layer.ui(I6_ScrollbarTrack,303,33,8,174);const int y=v.scroll.thumb_y(33,154);
  layer.ui(I6_ScrollbarGrabber,299,y,16,20);c.hit(298,25,18,190,HitKind::AuxiliaryControl,AuxiliaryScrollThumb,!v.closing);
 }
 native_back(layer,7,211,84,28,"返回",!v.closing);c.hit(3,207,94,33,HitKind::AuxiliaryControl,AuxiliaryBack,!v.closing);
 auxiliary_patch_button(layer);
}
static const char*auxiliary_credit_line(const char*p,char*out,unsigned capacity){
 unsigned n=0;while(*p&&*p!='\n'){if(n+1<capacity)out[n++]=*p;++p;}out[n]=0;return *p=='\n'?p+1:p;
}
static int auxiliary_credit_height(Canvas&c,const char*text,int width,int size=8){
 const auto metrics=[&](const RichChar&ch,int px){const auto g=c.a.content.glyph_metrics(ch.codepoint,px,rich_font_style(ch));return fitted_text::Metric{g.advance,mx(int(g.w),int(g.advance))};};
 return mx(1,fitted_text::line_count(text,width,size,metrics))*(size+1)+2;
}
static void auxiliary_credit_text(Canvas&c,const char*text,int x,int y,int width,int height,uint16_t ink=White,bool center=true){
 if(y<240&&y+height>0)c.paragraph_fitted(text,x,y,width,height,8,ink,8,center);
}
static void auxiliary_credits_screen(Canvas&c){
 auto&a=c.a;auto&v=a.auxiliary;c.rect(0,0,320,240,0);a.hit_count=0;Canvas layer=c;layer.opacity=uint8_t(auxiliary_alpha(a));
 int y=114-v.credits_position/256;const char*text=a.content.string(AuxiliaryCreditsString);char line[1024]{};
 const uint16_t cream=rgb(255,246,226),roles=rgb(135,206,235);
 for(const char*p=text;*p;){const char*begin=p;p=auxiliary_credit_line(p,line,sizeof line);const char kind=line[0];const char*body=line+1;
  if(kind=='H'||kind=='h'||kind=='B'||kind=='s'||kind=='t'){const int height=kind=='H'?46:kind=='s'?23:kind=='t'?15:31;if(y+height>=0&&y<240){Canvas header=layer;if(kind=='B')header.font_face=5;header.text_fitted(body,12,y+height-12,296,10,kind=='B'?rgb(127,255,0):Gold,8,true);}y+=height;}
  else if(kind=='S'){int amount=0;for(const char*q=body;*q>='0'&&*q<='9';++q)amount=amount*10+(*q-'0');y+=amount;}
  else if(kind=='I'){
   const int id=*body-'0';static constexpr int widths[]={124,100,95,45,57},heights[]={69,27,29,42,57};
   if(id>=0&&id<5){if(y<240&&y+heights[id]>0){const int image=id==4?a.content.world_image("gameover","RUN_SUMMARY_MERCHANT"):a.content.ui_image(I8_CreditsLogo+id);
    layer.picture(image,(320-widths[id])/2,y+(id==0?3:0),widths[id],heights[id]-(id==0?6:0));}y+=heights[id];}
  }else if(kind=='R'){
   char*right=nullptr;for(char*q=line+1;*q;++q)if(*q=='\t'){*q=0;right=q+1;break;}
   if(!right)continue;const int height=mx(auxiliary_credit_height(layer,body,136),auxiliary_credit_height(layer,right,136));
   auxiliary_credit_text(layer,body,19,y,136,height,roles,false);auxiliary_credit_text(layer,right,165,y,136,height,cream,false);y+=height;
  }else if(kind=='M'||kind=='P'){
   // Chaotic credit ordering belongs only to this modal. Never consume any
   // run RNG stream for modder/playtester shuffles.
   const char*names[128];unsigned count=0;const char*q=begin;
   while(*q==kind&&count<128){names[count++]=q+1;while(*q&&*q!='\n')++q;if(*q)++q;}
   p=q;uint32_t rng=v.opened^0x6c8e9cf5u^unsigned(kind);
   for(unsigned i=count;i>1;--i){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;const unsigned j=rng%i;const char*swap=names[i-1];names[i-1]=names[j];names[j]=swap;}
   const unsigned columns=kind=='P'?3:1;const int width=columns==3?92:296;
   for(unsigned row=0;row<count;row+=columns){char cells[3][192]{};int height=11;
    for(unsigned column=0;column<columns&&row+column<count;++column){auxiliary_credit_line(names[row+column],cells[column],sizeof cells[column]);height=mx(height,auxiliary_credit_height(layer,cells[column],width));}
    for(unsigned column=0;column<columns&&row+column<count;++column)auxiliary_credit_text(layer,cells[column],columns==3?12+int(column)*102:12,y,width,height,cream,true);
    y+=height;
   }
  }else if(kind=='C'){const int height=auxiliary_credit_height(layer,body,296);auxiliary_credit_text(layer,body,12,y,296,height,cream,true);y+=height;}
 }
 c.hit(0,0,320,240,HitKind::AuxiliaryControl,AuxiliaryBack,!v.closing&&a.map_clock_ms-v.opened>=2000u);
 if(a.map_clock_ms-v.opened>=2000u)native_back(layer,7,211,84,28,"返回",!v.closing);
}
static const char*feedback_category_key(unsigned index){
 static const char*const keys[]={"FEEDBACK_CATEGORY.bug","FEEDBACK_CATEGORY.balance","FEEDBACK_CATEGORY.feedback","FEEDBACK_CATEGORY.translation"};return keys[index<4?index:2];
}
static void auxiliary_feedback_screen(Canvas&c){
 auto&a=c.a;auto&v=a.auxiliary;a.hit_count=0;c.dim(0,0,320,240,173);
 const bool english=a.content.english();const uint16_t pink=rgb(232,154,198),purple=rgb(69,0,78),ink=rgb(255,246,226);
 auxiliary_rounded(c,92,65,140,115,6,Ink);auxiliary_rounded(c,90,63,140,115,6,pink);
 c.text_fitted(auxiliary_port_label(a,"FEEDBACK_TITLE","Local feedback report","本地反馈报告"),60,48,200,10,ink,8,true);
 auxiliary_rounded(c,93,67,134,56,3,v.editing?rgb(105,0,117):purple);
 if(v.draft_size){Canvas text=c;text.pixels+=70*text.pitch+97;if(text.coverage)text.coverage+=70*text.pitch+97;text.clip_w=126;text.clip_h=48;
  auxiliary_draft_layout(a,[&](unsigned,unsigned cp,int x,int row){if(cp&&cp!='\n'&&row>=int(v.draft_scroll)&&row<int(v.draft_scroll)+4)text.character(cp,a.content.glyph(cp,8),x,(row-v.draft_scroll)*11,ink);});
 }else c.paragraph(auxiliary_label(a,"FEEDBACK_DESCRIPTION_PLACEHOLDER","Enter up to8000 characters of feedback.","输入不超过8000字的反馈。"),97,70,126,4,0,8,rgb(188,153,190));
 if(v.editing&&(a.map_clock_ms-v.opened)%1000u<550u){auxiliary_draft_layout(a,[&](unsigned at,unsigned,int x,int row){if(at==v.caret&&row>=int(v.draft_scroll)&&row<int(v.draft_scroll)+4)c.rect(97+x,70+(row-v.draft_scroll)*11,1,9,ink);});}
 c.hit(93,67,134,56,HitKind::AuxiliaryControl,AuxiliaryFeedbackText);
 if(!a.pointer.visible&&v.focus==0)c.box(92,66,136,58,Gold);
 char count[32]{};TextBuffer number{count,sizeof count,0};number.number(v.draft_units);number.text("/8000");c.text_fitted(count,182,124,45,6,purple,6,true);
 c.text_fitted(auxiliary_label(a,"FEEDBACK_CATEGORY_LABEL","Category:","反馈分类："),94,128,78,6,ink,6);
 auxiliary_rounded(c,94,134,58,11,2,rgb(120,86,145));c.text_fitted(auxiliary_label(a,feedback_category_key(v.category),"Feedback","反馈"),97,136,49,6,ink,6,true);
 c.ui(I7_SettingsArrowDown,145,138,4,4);c.hit(94,134,58,11,HitKind::AuxiliaryControl,AuxiliaryFeedbackCategory);
 if(!a.pointer.visible&&v.focus==1)c.box(93,133,60,13,Gold);
 c.text_fitted(auxiliary_label(a,"FEEDBACK_EMOJI_LABEL","How do you feel?","你的感受："),94,152,96,6,ink,6);
 for(int i=0;i<5;++i){const int x=93+i*16,y=158;const bool focus=(!a.pointer.visible&&v.focus==i+2)||progress_hover(a,x,y,15,15);
  auxiliary_rounded(c,x,y,15,15,2,rgb(24,24,24));Canvas icon=c;if(v.emoji!=i&&!focus)icon.opacity=153;
  icon.ui(I8_FeedbackHappy+i,x+2,y+2,12,12);if(v.emoji==i||focus)c.box(x-1,y-1,17,17,Gold);
  c.hit(x,y,15,15,HitKind::AuxiliaryControl,AuxiliaryFeedbackEmojiFirst+i);
 }
 const bool export_focus=(!a.pointer.visible&&v.focus==7)||progress_hover(a,189,158,38,15);
 auxiliary_rounded(c,189,158,38,15,3,purple);auxiliary_rounded(c,190,159,36,13,2,rgb(78,162,242));
 c.text_fitted(auxiliary_port_label(a,"FEEDBACK_EXPORT","Export","导出"),191,161,34,8,ink,8,true);if(export_focus)c.box(188,157,40,17,Gold);
 c.hit(189,158,38,15,HitKind::AuxiliaryControl,AuxiliaryFeedbackExport, a.callbacks.write_feedback!=nullptr);
 const int flower=v.export_result==1?I8_FeedbackFlowerHappy:export_focus?I8_FeedbackFlowerAnticipation:I8_FeedbackFlower;
 const int tilt=v.export_result==1?CombatCardMotion::sine(int(a.map_clock_ms%1000u)*4096/1000)*143/1024:0;
 ui_picture_transform(c,a.content.ui_image(I8_FeedbackSun),42,43,49,49,0,1024);
 ui_picture_transform(c,a.content.ui_image(I8_FeedbackCupcake),72,193,59,59,0,1024);
 ui_picture_transform(c,a.content.ui_image(flower),289,209,69,68,tilt,1024-(tilt*tilt/2048));
 const char*status=v.export_result==1?auxiliary_port_label(a,"FEEDBACK_EXPORTED","Report exported locally.","报告已导出到本地。"):
  v.export_result==2?auxiliary_port_label(a,"FEEDBACK_FAILED","Export failed. Your draft is retained.","导出失败，草稿已保留。"):auxiliary_port_label(a,"FEEDBACK_EXPORT_INFO","Saves a text file on this device.","将文本报告保存到此设备。");
 c.paragraph_fitted(status,91,184,140,24,8,v.export_result==2?Red:ink,8,true);
 if(v.category_open){
  const int count=english?3:4;auxiliary_rounded(c,94,146,86,count*16+4,2,purple);
  for(int i=0;i<count;++i){const int y=148+i*16;const bool focused=v.category==i||progress_hover(a,96,y,82,16);
   if(focused)c.rect(96,y,82,16,rgb(101,82,134));c.text_fitted(auxiliary_label(a,feedback_category_key(unsigned(i)),"Feedback","反馈"),100,y+3,74,8,ink,8);
   c.hit(96,y,82,16,HitKind::AuxiliaryControl,AuxiliaryFeedbackCategoryFirst+i);
  }
 }
 native_back(c,7,211,84,28,"返回",true);c.hit(3,207,94,33,HitKind::AuxiliaryControl,AuxiliaryBack);
}
static void auxiliary_screen(Canvas&c){
 auto&a=c.a;const auto screen=a.auxiliary.screen;
 if(screen==AuxiliaryScreen::Credits){auxiliary_credits_screen(c);return;}
 if(screen==AuxiliaryScreen::PatchNotes){auxiliary_notes_screen(c);return;}
 if(screen==AuxiliaryScreen::Feedback){
  // The underlying room/settings remain live and cannot receive pointer
  // focus. Recompose once rather than allocating a second full framebuffer.
  const bool mouse=a.pointer.visible;a.pointer.visible=false;a.auxiliary.screen=AuxiliaryScreen::None;a.render();
  a.auxiliary.screen=screen;a.pointer.visible=mouse;auxiliary_feedback_screen(c);
 }
}
