// Pinned sources: scenes/combat/health_bar.tscn and NHealthBar.cs, v0.111.0.
// Raw original pixels live only in the owner's optional native UI directory.
// This adapter preserves those textures' end caps, masks and modulation.
void App::sync_health_bars(){
 const bool visible=!title&&(combat_phase(game.phase)||int32_t(combat_linger_until-animation)>0);
 if(!visible){if(health_bars_active)for(auto&bar:health_bars)bar.reset();health_bars_active=false;return;}
 const uint16_t encounter=game.enemy_world.active?uint16_t(256+unsigned(game.enemy_world.encounter)):game.encounter;
 if(!health_bars_active||health_bar_seed!=game.seed||health_bar_floor!=game.floor||
    health_bar_combat!=game.world.combat_count||health_bar_encounter!=encounter){
  for(auto&bar:health_bars)bar.reset();health_bars_active=true;health_bar_seed=game.seed;
  health_bar_floor=game.floor;health_bar_combat=game.world.combat_count;health_bar_encounter=encounter;
 }
 const uint32_t now=animation*33;
 const auto observe=[&](int slot,uint32_t identity,int width){const auto v=combat_health(slot);
  auto&motion=health_bars[slot];const bool restored=!motion.valid||motion.identity!=identity;
  motion.observe(identity,v.hp,v.max_hp,health_bar::inner_width(width),now);motion.observe_block(v.block,now,restored);};
 observe(0,1+unsigned(game.character),combat_player_pose().width);
 const bool pet=game.character==Character::Necrobinder||game.expansion.osty_max_hp>0;
 if(pet)observe(MaxEnemies+1,1,combat_player_pose(true).width);else health_bars[MaxEnemies+1].reset();
 for(int i=0;i<MaxEnemies;++i){
  if(i>=game.enemy_count){health_bars[i+1].reset();continue;}
  const auto&model=game.enemy_world.models[i];
  const uint32_t identity=ui_feedback_bound(*this)?combat_feedback.identity[i+1]:game.enemy_world.active?(uint32_t(model.combat_id)<<16)|(1+unsigned(model.kind)):uint32_t(i+1);
  observe(i+1,identity,combat_enemy_pose(i).width);
 }
}

namespace {
// The four source patches are at most 22x16. Copying a tiny decoded patch
// gives masked layers a stable view even if the next image evicts a cache
// entry; no large actor frame is decoded or copied by this component.
struct HealthPatch {
 uint16_t color[22*16]{};uint8_t alpha[22*16]{};int width=0,height=0;
 bool read(const Content&content,int image){
  const auto p=content.image(image);width=height=0;
  if(!p.pixels||!p.w||!p.h||p.w>22||p.h>16)return false;
  width=p.w;height=p.h;for(int i=0;i<width*height;i++){color[i]=0;alpha[i]=0;}
  if(p.format==4){const uint8_t*run=p.pixels;
   for(int y=0;y<height;++y){const unsigned spans=run[0]|unsigned(run[1])<<8;run+=2;
    for(unsigned s=0;s<spans;++s){const unsigned x=run[0]|unsigned(run[1])<<8,n=run[2]|unsigned(run[3])<<8,mode=run[4];run+=5;
     for(unsigned j=0;j<n;++j){const unsigned at=y*width+x+j;color[at]=run[0]|uint16_t(run[1])<<8;alpha[at]=mode?run[2]:255;run+=mode?3:2;}
    }
   }
  }else for(int i=0;i<width*height;i++){
   const uint8_t*pixel=p.palette?p.palette+2*p.pixels[i]:p.pixels+2*i;
   color[i]=pixel[0]|uint16_t(pixel[1])<<8;alpha[i]=p.alpha?p.alpha[i]:255;
  }return true;
 }
};
static uint16_t health_tint(uint16_t color,unsigned tint){
 // Expand before tinting so an opaque white source produces the authored
 // HP tint exactly when quantized to RGB565, instead of rounding 5-bit
 // channels before their multiplication.
 const unsigned r=color>>11,g=(color>>5)&63,b=color&31;
 return rgb(div255(((r<<3)|(r>>2))*((tint>>16)&255)),
            div255(((g<<2)|(g>>4))*((tint>>8)&255)),
            div255(((b<<3)|(b>>2))*(tint&255)));
}
static void health_patch(Canvas&c,const HealthPatch&p,int x,int y,int width,int height,int margin,int cap,unsigned tint,unsigned alpha=255){
 if(width<=0||height<=0||!p.width)return;
 int source[320];const int left=mx(0,-x),right=mn(width,c.clip_w-x);
 for(int xx=left;xx<right;++xx)source[xx-left]=health_bar::patch_source(xx,width,p.width,margin,cap);
 for(int yy=mx(0,-y);yy<mn(height,c.clip_h-y);++yy){const int row=mn(p.height-1,(2*yy+1)*p.height/(2*height))*p.width;
  for(int xx=left;xx<right;++xx){const int at=row+source[xx-left];
   c.paint((y+yy)*c.pitch+x+xx,health_tint(p.color[at],tint),div255(unsigned(p.alpha[at])*alpha));
  }
 }
}
static void health_fallback_body(Canvas&c,int x,int y,int width,bool blocked){
 for(int row=0;row<health_bar::Height;++row){const int cut=row<3?3-row:row-2;
  c.rect(x+cut,y+row,width-2*cut,1,rgb(13,32,37));
  if(blocked){const auto stroke=rgb(180,226,255);
   if(row==0||row==health_bar::Height-1)c.rect(x+cut,y+row,width-2*cut,1,stroke);
   else{c.rect(x+cut,y+row,1,1,stroke);c.rect(x+width-cut-1,y+row,1,1,stroke);}
  }
 }
}
static void health_fill(Canvas&c,const HealthPatch&fill,const HealthPatch&mask,int x,int y,int width,int left,int right,unsigned tint){
 if(left>=right)return;
 const int span=right-left;
 for(int yy=0;yy<health_bar::ForegroundHeight;++yy){
  if(y+yy<0||y+yy>=c.clip_h)continue;
  const int mask_row=mask.height?mn(mask.height-1,(2*yy+1)*mask.height/(2*health_bar::ForegroundHeight))*mask.width:0;
  // The original fill extends 4 source px above/below its 10px mask.
  const int fill_row=fill.height?mn(fill.height-1,((2*yy+1)*10+8*health_bar::ForegroundHeight)*fill.height/(36*health_bar::ForegroundHeight))*fill.width:0;
  for(int xx=mx(left,-x);xx<mn(right,c.clip_w-x);++xx){
   unsigned clip=255;
   if(mask.width)clip=mask.alpha[mask_row+health_bar::patch_source(xx,width,mask.width,6)];
   else{const int cut=yy==0||yy==health_bar::ForegroundHeight-1?2:1;if(xx<cut||xx>=width-cut)clip=0;}
   if(!clip)continue;
   uint16_t color=yy<health_bar::ForegroundHeight/2?rgb(255,255,255):rgb(176,176,176);unsigned alpha=255;
   if(fill.width){const int at=fill_row+health_bar::patch_source(xx-left,span,fill.width,6);color=fill.color[at];alpha=fill.alpha[at];}
   c.paint((y+yy)*c.pitch+x+xx,health_tint(color,tint),div255(alpha*clip));
  }
 }
}
static void health_label(Canvas&c,const char*label,int x,int y,int width,int size,uint16_t color,uint16_t outline,bool full_outline=false,int center_height=0){
 size=c.a.content.font_size(size,c.font_face,false,c.sparse_scene_sizes);
 const int origin=x+mx(0,(width-c.width(label,size))/2);const char*text=label;
 int pen64=0;uint32_t previous=0;
 // Each glyph is fetched once for all outline passes. Repeated text() calls
 // would redo the directory/cache lookup for every outline direction.
 Glyph glyphs[32];int positions[32];unsigned count=0;
 while(*text&&count<32){const uint32_t cp=utf8(text);const auto glyph=c.a.content.glyph(cp,size,c.font_face);
  pen64+=c.a.content.glyph_kerning64(previous,cp,size,c.font_face);previous=cp;
  const int advance= glyph.advance64?glyph.advance64:int(glyph.advance)*64;
  if(!center_height&&origin+text_pixel(pen64+advance)>x+width)break;
  glyphs[count]=glyph;positions[count++]=origin+text_pixel(pen64)+glyph.bearing_x;pen64+=advance;
 }
 if(center_height){
  // Count badges center the visible digits, not the font's advance box or
  // padded ascent/descent rows. The original fonts have different bearings
  // across locales; reusing a fixed+3 Y offset misaligned their red badges.
  int left=x+width,right=x,top=255,bottom=0;
  for(unsigned i=0;i<count;++i){const auto g=glyphs[i];if(!g.pixels||!g.w||!g.h)continue;
   const auto span=[&](unsigned row,unsigned column,unsigned length){
    left=mn(left,positions[i]+int(column));right=mx(right,positions[i]+int(column+length));
    top=mn(top,int(row));bottom=mx(bottom,int(row+1));
   };
   if(g.encoded_size){unsigned at=0,row=0,column=0;const unsigned pixels=unsigned(g.w)*g.h;
    for(unsigned n=0;n+1<g.encoded_size&&at<pixels;n+=2){
     unsigned length=mn(unsigned(g.pixels[n]),pixels-at);at+=length;
     while(length){const unsigned take=mn(length,unsigned(g.w)-column);
      if(g.pixels[n+1])span(row,column,take);column+=take;length-=take;
      if(column==g.w){column=0;++row;}}
    }
   }else for(unsigned row=0;row<g.h;++row){
    const auto*line=g.pixels+row*g.w;unsigned first=g.w,last=0;
    for(unsigned column=0;column<g.w;++column)if(line[column]){if(first==g.w)first=column;last=column+1;}
    if(last>first)span(row,first,last-first);
   }
  }
  if(right>left&&bottom>top){const int shift=x+(width-(right-left))/2-left;
   for(unsigned i=0;i<count;++i)positions[i]+=shift;y+=(center_height-(bottom-top))/2-top;}
 }
 // Complete every outline first: a tightly spaced following character's
 // left stroke must not paint over the preceding character's foreground.
 for(unsigned i=0;i<count;++i){const auto glyph=glyphs[i];const int px=positions[i];
  Canvas shadow=c;shadow.opacity=uint8_t(div255(unsigned(shadow.opacity)*(full_outline?208u:160u)));
  if(full_outline){shadow.glyph(glyph,px-1,y,outline);shadow.glyph(glyph,px+1,y,outline);shadow.glyph(glyph,px,y-1,outline);shadow.glyph(glyph,px,y+1,outline);}
  else shadow.glyph(glyph,px+1,y+1,outline);
 }
 for(unsigned i=0;i<count;++i)c.glyph(glyphs[i],positions[i],y,color);
}
static void health_amount(TextBuffer&b,int value){
 if(value>=1000000){b.number(value/1000000);b.ch('M');}
 else if(value>=10000){b.number(value/1000);b.ch('k');}else b.number(value);
}
static void health_block_break(Canvas&c,const HealthBarMotion&motion,int x,int y){
 const unsigned elapsed=c.a.animation*33u-motion.block_break_ms;
 if(!motion.block_breaking||elapsed>=600u||c.a.content.version()<Ui7ContentVersion)return;
 const unsigned p=HealthBarMotion::block_break_progress(elapsed),alpha=(4096u-HealthBarMotion::block_break_progress(elapsed,true))*255u/4096u;
 for(unsigned side=0;side<2;++side){HealthPatch patch;
  if(!patch.read(c.a.content,c.a.content.ui_image(side?I7_BlockBreakRight:I7_BlockBreakLeft)))continue;
  const int dx=side?4096+13*int(p):-4096-15*int(p),dy=side?-11*4096+10*int(p):-10*4096+9*int(p);
  // Scene scales each128px fragment by .5. At the room's 1/6 scale it
  // spans10.67px, centered on the shield's original control center.
  const int cx=(x-2)*1024+dx/24,cy=(y+3)*1024+dy/24;
  const unsigned angle=(side?275u:232u)*p/4096u;
  const int sn=(side?1:-1)*int(CombatCardMotion::sine(angle)),cs=int(CombatCardMotion::sine(1024u-angle));
  const int left=mx(0,cx/1024-8),right=mn(c.clip_w,cx/1024+9),top=mx(0,cy/1024-8),bottom=mn(c.clip_h,cy/1024+9);
  for(int yy=top;yy<bottom;++yy)for(int xx=left;xx<right;++xx){
   const int rx=xx*1024+512-cx,ry=yy*1024+512-cy;
   const int u=(rx*cs+ry*sn)/1024+5461,v=(-rx*sn+ry*cs)/1024+5461;
   if(u<0||v<0||u>=10923||v>=10923)continue;
   const int at=(v*patch.height/10923)*patch.width+u*patch.width/10923;
   c.paint(yy*c.pitch+xx,patch.color[at],div255(unsigned(patch.alpha[at])*alpha));
  }
 }
}
}

void Canvas::hp(const HealthBarValues&v,const HealthBarMotion&motion,int x,int y,int width){
 width=health_bar::bounded(width,0,320);const int inner=health_bar::inner_width(width);if(!inner)return;
 const auto layers=health_bar::segments(v,inner);HealthPatch patch,mask;
 if(patch.read(a.content,a.content.world_image("common","HEALTH_BAR_BG")))
  health_patch(*this,patch,x+1,y,width-2,health_bar::Height,9,3,0x2a606b,192);
 else health_fallback_body(*this,x,y,width,layers.blocked);
 if(layers.blocked&&patch.read(a.content,a.content.world_image("common","HEALTH_BAR_STROKE")))
  health_patch(*this,patch,x+1,y,width-2,health_bar::Height,9,3,0xb4e2ff);
 mask.read(a.content,a.content.world_image("common","HEALTH_BAR_MASK"));
 patch.read(a.content,a.content.world_image("common","HEALTH_BAR_FILL"));
 const int fx=x+health_bar::Inset,fy=y+1;
 if(v.hp>0){
  const int trail=motion.valid?motion.sample(a.animation*33):layers.current;
  if(trail>layers.current)health_fill(*this,patch,mask,fx,fy,inner,0,trail,0xff7f00);
  health_fill(*this,patch,mask,fx,fy,inner,layers.poison_left,layers.poison_right,0x79c03c);
  health_fill(*this,patch,mask,fx,fy,inner,0,layers.hp,v.infinite?0xc5bbed:layers.blocked?0x3b6fa3:0xf1373e);
  // The pinned Doom shader has animated noise. Its correct region/tint is
  // retained here; this static raster adapter does not claim shader parity.
  health_fill(*this,patch,mask,fx,fy,inner,0,layers.doom_right,0x5a42a5);
 }
 const uint16_t cream=rgb(255,246,226);
 if(v.infinite&&v.hp>0){const int icon=a.content.world_image("common","HEALTH_BAR_INFINITY");
  if(icon>=0)picture(icon,x+(width-18)/2,y-2,18,10);
  else{const int middle=x+width/2;const int points[][2]={{-7,0},{-5,-2},{-3,-2},{3,2},{5,2},{7,0},{5,-2},{3,-2},{-3,2},{-5,2},{-7,0}};
   for(unsigned i=1;i<sizeof points/sizeof points[0];++i)line(middle+points[i-1][0],y+3+points[i-1][1],middle+points[i][0],y+3+points[i][1],cream);
  }
 }else{
  char label[32]{};TextBuffer b{label,sizeof label,0};
  if(v.hp<=0){const char*dead=a.content.world_string("gameplay_ui","HEALTH_BAR.DEAD");b.text(*dead?dead:"已击败");}
  else{b.number(v.hp);b.ch('/');b.number(v.max_hp);}
  int size=this->width(label,10)<=width-2?10:8;
  if(v.hp>0&&this->width(label,size)>width-2){b.n=0;label[0]=0;health_amount(b,v.hp);b.ch('/');health_amount(b,v.max_hp);}
  const auto color=layers.poison_lethal?rgb(118,255,64):layers.doom_lethal?rgb(251,141,255):cream;
  const auto outline=v.hp<=0?rgb(144,0,0):layers.poison_lethal?rgb(7,71,0):layers.doom_lethal?rgb(45,18,99):layers.blocked?rgb(27,48,69):rgb(144,0,0);
  health_label(*this,label,x,y+(health_bar::Height-size)/2,width,size,color,outline,true);
 }
 if(v.block>0){const int block=a.content.world_image("common","HEALTH_BAR_BLOCK"),bx=x-10,by=y-5+motion.block_y(a.animation*33);
  Canvas shield=*this;shield.opacity=uint8_t(div255(unsigned(opacity)*motion.block_alpha(a.animation*33)));
  shield.picture(block>=0?block:A_Block,bx,by,16,16);char label[16]{};TextBuffer b{label,sizeof label,0};b.number(v.block);
  if(this->width(label,8)>16){b.n=0;label[0]=0;health_amount(b,v.block);}
  health_label(shield,label,bx,by+4,16,8,cream,rgb(11,85,107),true);
 }
 health_block_break(*this,motion,x,y);
}
