static unsigned auxiliary_alpha(const App&a){
 const auto&v=a.auxiliary;
 if(v.closing){const unsigned age=a.map_clock_ms-v.closing_at;return age>=1000?0:255u-age*255u/1000;}
 const unsigned age=a.map_clock_ms-v.opened;return age>=2000?255:age*255u/2000;
}
static const char*auxiliary_credit_line(const char*p,char*out,unsigned capacity){
 unsigned n=0;while(*p&&*p!='\n'){if(n+1<capacity)out[n++]=*p;++p;}out[n]=0;return *p=='\n'?p+1:p;
}
static int auxiliary_credit_height(Canvas&c,const char*text,int width,int size=12){
 const auto metrics=[&](const RichChar&ch,int px){return c.text_metric(ch,px);};
 return mx(1,fitted_text::line_count(text,width,size,metrics))*(size+1)+2;
}
static void auxiliary_credit_text(Canvas&c,const char*text,int x,int y,int width,int height,uint16_t ink=White,bool center=true){
 if(y<240&&y+height>0)c.paragraph_fitted(text,x,y,width,height,12,ink,12,center);
}
static void auxiliary_credit_right(Canvas&c,const char*text,int x,int y,int width,int height,uint16_t color){
 if(y>=240||y+height<=0)return;
 constexpr int size=12,step=13;int widths[64]{};
 const auto metric=[&](const RichChar&ch,int px){return c.text_metric(ch,px);};
 const int rows=mn(64,mx(1,fitted_text::line_count(text,width,size,metric)));
 const fitted_text::Plan plan{size,rows,rows,step,false};
 fitted_text::visit(text,width,plan,metric,[&](const RichChar&ch,int at,int row,int px){widths[row]=at+metric(ch,px).advance;});
 Canvas ink=c;ink.clip_w=mn(ink.clip_w,x+width);ink.clip_h=mn(ink.clip_h,y+height);
 fitted_text::visit(text,width,plan,metric,[&](const RichChar&ch,int at,int row,int px){
  const auto glyph=c.a.content.glyph(ch.codepoint,px,c.font_style(ch));ink.character(ch.codepoint,glyph,x+width-widths[row]+at,y+row*step,color);});
}
static void auxiliary_port_credit(Canvas&layer,int&y){
 const auto&a=layer.a;y+=12;
 char localized_heading[192]{},localized_author[256]{};
 const char*payload=a.content.string(PortCreditsString);
 const char*heading=a.content.language()==2||a.content.language()==3?"Nspire 移植":"Nspire port";
 const char*author=a.content.language()==2?"zyf722（基于 Codex）":a.content.language()==3?"zyf722（基於 Codex）":"zyf722 (with Codex)";
 if(*payload){const char*next=auxiliary_credit_line(payload,localized_heading,sizeof localized_heading);
  auxiliary_credit_line(next,localized_author,sizeof localized_author);if(*localized_heading&&*localized_author){heading=localized_heading;author=localized_author;}}
 // Match the source LOC/TRAILER major-section label, including its green
 // Kreon/locale bold face and the authored spacing before the author line.
 Canvas section=layer;section.font_face=5;
 section.text_fitted(heading,12,y+26,296,12,rgb(127,255,0),12,true);
 auxiliary_credit_text(layer,author,12,y+44,296,22,rgb(255,246,226),true);y+=82;
}
static void auxiliary_credits_screen(Canvas&c){
 auto&a=c.a;auto&v=a.auxiliary;c.rect(0,0,320,240,0);a.hit_count=0;Canvas layer=c;layer.opacity=uint8_t(auxiliary_alpha(a));
 int y=114-v.credits_position/256;const char*text=a.content.string(AuxiliaryCreditsString);char line[1024]{};
 const uint16_t cream=rgb(255,246,226),roles=rgb(135,206,235);
 for(const char*p=text;*p;){const char*begin=p;p=auxiliary_credit_line(p,line,sizeof line);const char kind=line[0];const char*body=line+1;
  if(kind=='H'||kind=='h'||kind=='B'||kind=='s'||kind=='t'){const int height=kind=='H'?60:kind=='s'?32:kind=='t'?22:42;if(y+height>=0&&y<240){Canvas header=layer;if(kind=='B')header.font_face=5;header.text_fitted(body,12,y+height-16,296,12,kind=='B'?rgb(127,255,0):Gold,12,true);}y+=height;}
  else if(kind=='S'){int amount=0;for(const char*q=body;*q>='0'&&*q<='9';++q)amount=amount*10+(*q-'0');y+=amount;}
  else if(kind=='I'){
   const int id=*body-'0';if(id==1)auxiliary_port_credit(layer,y);static constexpr int widths[]={124,100,95,45,57},heights[]={69,27,29,42,57};
   if(id>=0&&id<5){if(y<240&&y+heights[id]>0){const int image=id==4?a.content.world_image("gameover","RUN_SUMMARY_MERCHANT"):a.content.ui_image(I8_CreditsLogo+id);
    layer.picture(image,(320-widths[id])/2,y+(id==0?3:0),widths[id],heights[id]-(id==0?6:0));}y+=heights[id];}
  }else if(kind=='R'){
   char*right=nullptr;for(char*q=line+1;*q;++q)if(*q=='\t'){*q=0;right=q+1;break;}
   if(!right)continue;const int height=mx(auxiliary_credit_height(layer,body,136),auxiliary_credit_height(layer,right,136));
   auxiliary_credit_right(layer,body,19,y,136,height,roles);auxiliary_credit_text(layer,right,165,y,136,height,cream,false);y+=height;
  }else if(kind=='M'||kind=='P'){
   // Chaotic credit ordering belongs only to this modal. Never consume any
   // run RNG stream for modder/playtester shuffles.
   const char*names[128];unsigned count=0;const char*q=begin;
   while(*q==kind&&count<128){names[count++]=q+1;while(*q&&*q!='\n')++q;if(*q)++q;}
   p=q;uint32_t rng=v.opened^0x6c8e9cf5u^unsigned(kind);
   for(unsigned i=count;i>1;--i){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;const unsigned j=rng%i;const char*swap=names[i-1];names[i-1]=names[j];names[j]=swap;}
   const unsigned columns=kind=='P'?2:1;const int width=columns==2?142:296;
   for(unsigned row=0;row<count;row+=columns){char cells[3][192]{};int height=11;
    for(unsigned column=0;column<columns&&row+column<count;++column){auxiliary_credit_line(names[row+column],cells[column],sizeof cells[column]);height=mx(height,auxiliary_credit_height(layer,cells[column],width));}
    for(unsigned column=0;column<columns&&row+column<count;++column)auxiliary_credit_text(layer,cells[column],columns==2?12+int(column)*154:12,y,width,height,cream,true);
    y+=height;
   }
  }else if(kind=='C'){const int height=auxiliary_credit_height(layer,body,296);auxiliary_credit_text(layer,body,12,y,296,height,cream,true);y+=height;}
 }
 // The port block is inserted at the source Trailer → FMOD boundary.
 c.hit(0,0,320,240,HitKind::AuxiliaryControl,AuxiliaryBack,!v.closing&&a.map_clock_ms-v.opened>=2000u);
 if(a.map_clock_ms-v.opened>=2000u)native_original_back(layer,v.opened);
}
static void auxiliary_screen(Canvas&c){
 if(c.a.auxiliary.screen==AuxiliaryScreen::Credits)auxiliary_credits_screen(c);
}
