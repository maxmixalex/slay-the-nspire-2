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
  health_bars[slot].observe(identity,v.hp,v.max_hp,health_bar::inner_width(width),now);};
 observe(0,1+unsigned(game.character),62);
 const bool pet=game.character==Character::Necrobinder||game.expansion.osty_max_hp>0;
 if(pet)observe(MaxEnemies+1,1,34);else health_bars[MaxEnemies+1].reset();
 for(int i=0;i<MaxEnemies;++i){
  if(i>=game.enemy_count){health_bars[i+1].reset();continue;}
  const auto&model=game.enemy_world.models[i];
  const uint32_t identity=game.enemy_world.active?(uint32_t(model.combat_id)<<16)|(1+unsigned(model.kind)):uint32_t(i+1);
  observe(i+1,identity,enemy_pose(game.enemy_count,i).width);
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
static void health_label(Canvas&c,const char*label,int x,int y,int width,int size,uint16_t color,uint16_t outline){
 int at=x+mx(0,(width-c.width(label,size))/2);const char*text=label;
 // Each glyph is fetched once for all outline passes. Repeated text() calls
 // would redo the directory/cache lookup for every outline direction.
 Glyph glyphs[32];int positions[32];unsigned count=0;
 while(*text&&count<32){const auto glyph=c.a.content.glyph(utf8(text),size);if(at+glyph.advance>x+width)break;
  glyphs[count]=glyph;positions[count++]=at;at+=glyph.advance;
 }
 // Complete every outline first: a tightly spaced following character's
 // left stroke must not paint over the preceding character's foreground.
 for(unsigned i=0;i<count;++i){const auto glyph=glyphs[i];const int px=positions[i];
  c.glyph(glyph,px-1,y,outline);c.glyph(glyph,px+1,y,outline);
  c.glyph(glyph,px,y-1,outline);c.glyph(glyph,px,y+1,outline);
 }
 for(unsigned i=0;i<count;++i)c.glyph(glyphs[i],positions[i],y,color);
}
static void health_amount(TextBuffer&b,int value){
 if(value>=1000000){b.number(value/1000000);b.ch('M');}
 else if(value>=10000){b.number(value/1000);b.ch('k');}else b.number(value);
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
  health_label(*this,label,x,y+(health_bar::Height-size)/2,width,size,color,outline);
 }
 if(v.block>0){const int block=a.content.world_image("common","HEALTH_BAR_BLOCK"),bx=x-10,by=y-5;
  picture(block>=0?block:A_Block,bx,by,16,16);char label[16]{};TextBuffer b{label,sizeof label,0};b.number(v.block);
  if(this->width(label,8)>16){b.n=0;label[0]=0;health_amount(b,v.block);}
  health_label(*this,label,bx,by+4,16,8,cream,rgb(11,85,107));
 }
}
