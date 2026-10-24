#include "app.hpp"
#include "combat_layout.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace spire;
namespace {
using Bytes=std::vector<uint8_t>;
static App app;
static SaveData before,after,disk;
static bool disk_exists;
static unsigned checks;
static const char* captures;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"health UI:%d %s (hp=%d enemy=%d accelerant=%d phase=%u)\n",__LINE__,#x,app.game.hp,app.game.enemies[0].hp,app.game.silent.powers[unsigned(SPower::Accelerant)],unsigned(app.game.phase));std::exit(1);}}while(0)
static void put(Bytes&b,unsigned at,uint32_t value,unsigned size=4){CHECK(at+size<=b.size());for(unsigned i=0;i<size;++i)b[at+i]=uint8_t(value>>(8*i));}
static uint32_t crc(const Bytes&b){uint32_t c=~0u;for(uint8_t value:b){c^=value;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
static unsigned group(Bytes&b,const Bytes&raw,unsigned type,unsigned id){
 if(b.size()%2)b.push_back(0);const unsigned at=unsigned(b.size());b.resize(at+32);
 const unsigned fields[]={0x31524754,type,id,unsigned(raw.size()),unsigned(raw.size())+4,crc(raw),1,0};
 for(unsigned i=0;i<8;++i)put(b,at+i*4,fields[i]);
 b.push_back(1);b.push_back(uint8_t((raw.size()-1)>>8));b.push_back(uint8_t(raw.size()-1));
 b.insert(b.end(),raw.begin(),raw.end());b.push_back(0);return at;
}
static Bytes profile(bool native){
 // Synthetic rectangles and one-pixel glyphs make mask clipping, exact
 // modulation and label colors measurable. No original pixels are embedded.
 constexpr unsigned images=173,glyphs=4*95;
 const unsigned auxiliary=native?unsigned(NativeUiAuxiliaryCount):unsigned(U_Count);
 const unsigned glyph_at=40+images*16,string_at=glyph_at+glyphs*16;
 const unsigned clip_at=string_at+ContentStrings*8,card_at=clip_at+AnimationKinds*16;
 const unsigned ui_at=card_at+CardKinds*16,base=ui_at+auxiliary*4;
 Bytes b(base+(native?96:0));put(b,0,0x3250534e);put(b,4,native?NativeUiContentVersion:ContentVersion);put(b,8,0x41cef1ea);
 put(b,16,images);put(b,20,glyphs);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
 const unsigned empty_picture=unsigned(b.size());b.resize(b.size()+82*60*3);
 for(unsigned i=0;i<images;++i){put(b,40+i*16,empty_picture);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,48+i*16,1);put(b,52+i*16,82*60*3);}
 for(unsigned i=0;i<13;++i){
  unsigned width=16,height=16;
  if(i==0){width=20;height=16;}if(i==1){width=22;height=16;}
  if(i==2){width=12;height=10;}if(i==3){width=15;height=16;}
  if(i==4){width=50;height=28;}if(i==5){width=32;height=32;}
  if(i>=10){width=128;height=64;}
  const unsigned image=160+i,at=unsigned(b.size());b.resize(b.size()+width*height*3);
  put(b,40+image*16,at);put(b,44+image*16,width,2);put(b,46+image*16,height,2);put(b,48+image*16,1);put(b,52+image*16,width*height*3);
  for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){const unsigned p=y*width+x;put(b,at+p*2,0xffff,2);
   b[at+width*height*2+p]=i==1?uint8_t(y==0||y==height-1||x==0||x==width-1?255:0):
      i==2?uint8_t(x<2||x>=width-2?0:255):255;
  }
  if(native)put(b,ui_at+(U_HealthBarBackground+i)*4,image);
 }
 const unsigned pixel=unsigned(b.size());b.push_back(255);const unsigned empty=unsigned(b.size());b.push_back(0);
 unsigned index=0;for(unsigned size:{8u,10u,12u,16u})for(unsigned cp=32;cp<127;++cp,++index){
  const unsigned at=glyph_at+index*16;put(b,at,cp);put(b,at+4,pixel);put(b,at+8,size,2);b[at+10]=b[at+11]=1;put(b,at+12,1,2);
 }
 for(unsigned i=0;i<ContentStrings;++i)put(b,string_at+i*8,empty);
 for(unsigned i=0;i<AnimationKinds;++i){put(b,clip_at+i*16+4,1,2);put(b,clip_at+i*16+6,33,2);}
 if(native){
  const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,base+80,base+64,770,85,524288,617323,0,0};
  for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
  Bytes tile(770);tile[0]=tile[1]=1;Bytes record(18);put(record,0,2);put(record,4,4);put(record,8,2);
  const unsigned tiles=group(b,tile,1,0),indexes=group(b,record,2,1);
  put(b,base+64,tiles);put(b,base+68,32+unsigned(tile.size())+4);put(b,base+72,indexes);put(b,base+76,32+unsigned(record.size())+4);
  put(b,base+80,base);put(b,base+92,unsigned(record.size()));
  put(b,56,base+80);put(b,60,1,2);put(b,62,1,2);put(b,64,6);put(b,68,16);
 }
 put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();++i){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static bool store(const State&s){disk_exists=save(s,disk);return disk_exists;}
static bool restore(State&s){return disk_exists&&load(s,disk.bytes,disk.size);}
static void snapshot(){CHECK(save(app.game,before));}
static void unchanged(){CHECK(save(app.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static uint16_t rgb(unsigned r,unsigned g,unsigned b){return uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));}
static uint16_t pixel(int x,int y){CHECK(x>=0&&x<320&&y>=0&&y<240);return app.pixels[y*320+x];}
static void setup(Character character=Character::Ironclad){
 new_run(app.game,73,false,character);auto&s=app.game;s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=s.enemy_count=1;
 s.hp=s.max_hp=80;s.energy=99;s.deck_count=1;s.card_count=s.hand.count=0;s.enemies[0]={80,80,0,0,0,0};
 app.title=app.paused=app.help=app.library=app.character_select=app.map_preview.open=false;
 app.pointer.visible=false;app.animation=1000;app.reset_presentation();CHECK(validate(s));app.render();
}
static void capture(const char*name){
 if(!captures)return;const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
 std::fprintf(f,"P6\n320 240\n255\n");uint8_t row[320*3];
 for(int y=0;y<240;++y){for(int x=0;x<320;++x){const auto c=pixel(x,y);row[x*3]=uint8_t((c>>11)*255/31);row[x*3+1]=uint8_t(((c>>5)&63)*255/63);row[x*3+2]=uint8_t((c&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void native_pixels(){
 setup();const auto pose=enemy_pose(1,0);const int left=pose.x-pose.width/2+health_bar::Inset;
 CHECK(pixel(50,131)==rgb(241,55,62));CHECK(pixel(left+20,pose.hp+2)==rgb(241,55,62));
 // The mask's synthetic transparent cap must reveal the dark background,
 // even though the source fill is opaque. Stretching the complete mask
 // would incorrectly turn much more of the bar transparent.
 CHECK(pixel(left,pose.hp+2)!=rgb(241,55,62));CHECK(pixel(left+2,pose.hp+2)==rgb(241,55,62));
 app.game.block=12;app.game.enemies[0].block=7;snapshot();app.render();unchanged();
 CHECK(pixel(50,131)==rgb(59,111,163));CHECK(pixel(left+20,pose.hp+2)==rgb(59,111,163));
 CHECK(app.health_bars[0].block_entering&&app.health_bars[0].block_alpha(app.animation*33)==0);
 const auto entered=app.health_bars[0].block_start_ms;app.render();CHECK(app.health_bars[0].block_start_ms==entered);
 app.animation+=16;app.render();unchanged();CHECK(app.health_bars[0].block_alpha(app.animation*33)==255);
 CHECK(pixel(20,126)==0xffff); // The appended full-resolution block skin after its .5s entry.
 capture("health-native-block");
 auto&e=app.game.enemies[0];e.hp=60;e.block=0;e.poison=20;
 app.game.silent.power_count=1;app.game.silent.power_order[0]=unsigned(SPower::Accelerant);
 app.game.silent.powers[unsigned(SPower::Accelerant)]=1;app.game.expansion.doom[0]=10;
 snapshot();app.render();unchanged();
 CHECK(pixel(left+5,pose.hp+2)==rgb(90,66,165));
 CHECK(pixel(left+10,pose.hp+2)==rgb(241,55,62));
 CHECK(pixel(left+25,pose.hp+2)==rgb(121,192,60));
 app.game.expansion.doom[0]=21;app.render();CHECK(pixel(left+10,pose.hp+2)==rgb(90,66,165));
 // Next-turn Accelerant damage is 20+19+18+17, lethal despite 20 stacks.
 app.game.silent.powers[unsigned(SPower::Accelerant)]=3;app.render();
 CHECK(pixel(left+10,pose.hp+2)==rgb(121,192,60));
 const int label_x=pose.x-pose.width/2+(pose.width-5)/2;
 CHECK(pixel(label_x,pose.hp-2)==rgb(118,255,64));capture("health-native-lethal-poison");
 setup(Character::Necrobinder);app.game.expansion.osty_hp=app.game.expansion.osty_max_hp=10;app.game.block=12;
 app.render();CHECK(pixel(112,133)==rgb(59,111,163));
 CHECK(pixel(99,129)==rgb(16,20,22)); // Tracked owner block does not add a second shield.
 CHECK(encounter_setup(app.game,EncounterId::WaterfallGiantBoss));app.game.enemy_world.models[0].data[2]=1;
 app.render();CHECK(enemy_infinite_hp(app.game,0));CHECK(pixel(235,131)==0xffff);
 capture("health-native-infinite");
}
static void lifecycle(){
 setup();CHECK(app.health_bars_active&&app.health_bars[0].valid);
 app.game.hp=40;app.game.enemies[0].hp=40;snapshot();app.render();unchanged();
 CHECK(app.health_bars[0].delayed&&app.health_bars[1].delayed);
 const auto start=app.health_bars[0].start_ms;std::vector<uint16_t>same_frame(app.pixels,app.pixels+320*240);
 for(int i=0;i<4;++i){app.render();CHECK(app.health_bars[0].start_ms==start);CHECK(!std::memcmp(same_frame.data(),app.pixels,same_frame.size()*2));}unchanged();
 app.animation+=40;app.render();CHECK(app.health_bars[0].sample(app.animation*33)<58);
 app.game.hp=20;app.render();const int held=app.health_bars[0].sample(app.animation*33);CHECK(held>14);
 app.game.hp=70;app.render();CHECK(!app.health_bars[0].delayed);
 app.animation+=31;app.render();CHECK(app.health_bars[0].sample(app.animation*33)==50);
 app.input(Key::Save);CHECK(disk_exists);app.game.hp=10;app.render();CHECK(app.health_bars[0].delayed);
 app.input(Key::Load);CHECK(app.game.hp==70&&!app.health_bars_active&&!app.health_bars[0].valid);
 app.render();CHECK(app.health_bars[0].sample(app.animation*33)==50&&!app.health_bars[0].delayed);
 app.game.phase=Phase::Map;app.render();CHECK(!app.health_bars_active&&!app.health_bars[1].valid);
 app.game.phase=Phase::Combat;app.game.hp=40;app.render();CHECK(app.health_bars[0].sample(app.animation*33)==29);
 app.game.enemies[0].hp=0;app.render();CHECK(app.health_bars[1].sample(app.animation*33)==0);
 app.game.enemies[0].hp=60;app.render();CHECK(app.health_bars[1].sample(app.animation*33)==42&&!app.health_bars[1].delayed);
 CHECK(encounter_setup(app.game,EncounterId::WaterfallGiantBoss));app.render();
 app.game.enemies[0].hp/=2;app.render();CHECK(app.health_bars[1].delayed);
 ++app.game.enemy_world.models[0].combat_id;app.render();CHECK(!app.health_bars[1].delayed);
 for(unsigned field=0;field<3;++field){app.game.enemies[0].hp/=2;app.render();CHECK(app.health_bars[1].delayed);
  if(field==0)++app.game.floor;if(field==1)++app.game.world.combat_count;if(field==2)++app.game.seed;
  app.render();CHECK(!app.health_bars[1].delayed);
 }
 app.title=true;app.render();CHECK(!app.health_bars_active);app.title=false;
 setup();app.game.enemy_count=0;app.render();CHECK(!app.health_bars[1].valid);
}
static void crowd(){
 setup();for(unsigned count=1;count<=MaxEnemies;++count){app.game.enemy_count=uint8_t(count);
  for(unsigned i=0;i<count;++i)app.game.enemies[i]={int(40+i),80,int16_t(i+1),0,0,0,0,int16_t(i)};
  app.render();for(unsigned i=0;i<count;++i){const auto pose=enemy_pose(count,i);CHECK(pose.x-pose.width/2>=0&&pose.x+pose.width/2<=320);CHECK(app.health_bars[i+1].valid);}
 }capture("health-native-eight-enemies");
}
}
int main(int argc,char**argv){
 captures=argc>1?argv[1]:nullptr;const auto native=profile(true);CHECK(app.init(native.data(),native.size(),{store,restore}));
 native_pixels();lifecycle();crowd();
 const auto legacy=profile(false);CHECK(app.init(legacy.data(),legacy.size(),{store,restore}));setup();
 CHECK(app.content.world_image("common","HEALTH_BAR_BG")==-1);app.game.block=10;app.game.enemies[0].poison=12;app.render();
 CHECK(app.content.healthy()&&app.health_bars[0].valid);capture("health-historical-fallback");
 std::printf("health UI: %u checks, production renderer, masks, forecasts, block, infinity, repeated renders, load and creature lifecycle\n",checks);
}
