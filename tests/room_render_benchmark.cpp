// Actual-resource CPU/I/O measurement, never an original-game or device-FPS
// oracle. Build this unchanged driver against each frozen runtime's headers
// and archive. Scene fixtures use the production map/relic/event controllers;
// unrelated routing rooms receive explicit outcomes, not player-play claims.
#include "app.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include "resource_file_cursor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
using Clock=std::chrono::steady_clock;
namespace {
static App app;static SaveData before,after;static unsigned save_attempts=0;
static const char*capture_directory=nullptr;
struct Io {uint64_t calls=0,bytes=0,seeks=0;};
struct Source {FILE*file=nullptr;uint32_t length=0;ResourceFileCursor cursor{};Io io{};FILE*trace=nullptr;FILE*frames=nullptr;const char*scene="",*window="";};
int seek_file(void*f,long offset,int origin){return std::fseek(static_cast<FILE*>(f),offset,origin);}
unsigned read_file(void*out,unsigned size,unsigned count,void*f){return unsigned(std::fread(out,size,count,static_cast<FILE*>(f)));}
bool read_at(void*context,uint32_t offset,void*out,uint32_t count){
 auto&s=*static_cast<Source*>(context);bool sought=false;
 const bool ok=offset<=s.length&&count<=s.length-offset&&s.cursor.read(s.file,offset,out,count,seek_file,read_file,sought);
 ++s.io.calls;s.io.bytes+=count;s.io.seeks+=sought;
 if(s.trace)std::fprintf(s.trace,"{\"scene\":\"%s\",\"window\":\"%s\",\"offset\":%u,\"bytes\":%u,\"seek\":%s,\"ok\":%s}\n",s.scene,s.window,offset,count,sought?"true":"false",ok?"true":"false");
 return ok;
}
bool save_game(const State&){++save_attempts;return false;}
uint64_t pixels(){uint64_t hash=1469598103934665603ULL;for(uint16_t p:app.pixels){hash=(hash^uint8_t(p))*1099511628211ULL;hash=(hash^uint8_t(p>>8))*1099511628211ULL;}return hash;}
bool capture(const Source&source){
 if(!capture_directory)return true;char path[1024];
 const int length=std::snprintf(path,sizeof path,"%s/%s-%s.ppm",capture_directory,source.scene,source.window);
 if(length<0||length>=int(sizeof path))return false;
 FILE*f=std::fopen(path,"wb");if(!f)return false;bool good=std::fprintf(f,"P6\n320 240\n255\n")>0;
 uint8_t row[ScreenW*3];for(unsigned y=0;y<ScreenH&&good;++y){
  for(unsigned x=0;x<ScreenW;++x){const uint16_t pixel=app.pixels[y*ScreenW+x];
   row[x*3]=uint8_t((pixel>>11)*255/31);row[x*3+1]=uint8_t(((pixel>>5)&63)*255/63);row[x*3+2]=uint8_t((pixel&31)*255/31);}
  good=std::fwrite(row,1,sizeof row,f)==sizeof row;
 }return std::fclose(f)==0&&good;
}
double elapsed(Clock::time_point start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
struct Cost {
 unsigned frames=0;double ms=0,maximum=0;Io io{};
 void add(Clock::time_point start,const Io&before,const Source&source){
  const double took=elapsed(start);++frames;ms+=took;if(took>maximum)maximum=took;
  io.calls+=source.io.calls-before.calls;io.bytes+=source.io.bytes-before.bytes;io.seeks+=source.io.seeks-before.seeks;
 }
 void output(const Source&source)const{
  const auto images=app.content.image_cache_usage();
  std::printf("{\"scene\":\"%s\",\"window\":\"%s\",\"frames\":%u,\"cpu_ms\":%.6f,\"mean_ms\":%.6f,\"max_ms\":%.6f,\"source_calls\":%llu,\"source_bytes\":%llu,\"source_seeks\":%llu,\"image_entries\":%u,\"image_pages\":%u,\"last_pixels_fnv64\":\"%016llx\"}\n",source.scene,source.window,frames,ms,frames?ms/frames:0,maximum,(unsigned long long)io.calls,(unsigned long long)io.bytes,(unsigned long long)io.seeks,images.entries,images.pages,(unsigned long long)pixels());std::fflush(stdout);
 }
};
bool finish_fixture_room(){
 app.game.original_events={};if(world_room_is_combat(app.game))app.game.phase=Phase::Reward;
 return world_finish_room(app.game)&&world_valid_run(app.game);
}
bool shop_route(const reference::ReferenceMap&m,unsigned point,unsigned depth=0){
 if(point>=m.count||depth>reference::MaxRows+2)return false;
 if(m.points[point].type==reference::PointType::Shop)return true;
 const auto&children=m.points[point].children;
 for(unsigned i=0;i<children.used;++i)if(shop_route(m,children.slots[i],depth+1))return true;return false;
}
bool setup(unsigned scene){
 new_run(app.game,0x7195c3,false,Character::Ironclad);
 if(!world_set_seed(app.game,"NSPIRE-WORLD-017")||!world_begin_run(app.game))return false;
 if(scene==1){
  for(unsigned guard=0;guard<120;++guard){
   if(app.game.world.room_open&&app.game.world.room_type==unsigned(WorldRoom::Shop))break;
   if(world_can_travel(app.game)){
    uint8_t ids[reference::Columns];const unsigned count=world_available_points(app.game,ids,reference::Columns);const auto*m=world_current_map(app.game);
    if(!m||!count)return false;unsigned chosen=reference::NoPoint;
    for(unsigned i=0;i<count;++i)if(shop_route(*m,ids[i])){chosen=ids[i];break;}
    if(chosen==reference::NoPoint||!world_choose_point(app.game,chosen))return false;
   }else if(!finish_fixture_room())return false;
  }
  if(app.game.phase!=Phase::Shop)return false;
 }else if(scene!=5){
  if(!finish_fixture_room())return false;
  if(scene==2){
   if(!gain_world_relic(app.game,WorldRelicGlassEye))return false;
   const uint16_t selected=0;if(!world_relic_select(app.game,&selected,1)||app.game.world_relics.choice!=WorldRelicChoice::OfferCard)return false;
  }else if(scene==3||scene==4){
   if(!original_event_begin(app.game,OriginalEventId::CrystalSphere,31,1))return false;app.game.gold=250;
   if(!original_event_choose(app.game,1))return false;
   if(scene==4&&!original_event_crystal_click(app.game,5,5))return false;
  }
 }
 app.title=app.paused=app.help=app.library=app.character_select=app.run_settings=false;
 app.animation=app.map_clock_ms=app.tick_remainder=0;
 app.reset_presentation();app.pointer.visible=false;
 if(scene==0)app.open_map_window(false);
 return validate(app.game);
}
bool frame(Source&source,Cost&cost,unsigned step){
 const auto io=source.io;const auto start=Clock::now();if(step)app.advance(step);app.render();cost.add(start,io,source);
 if(source.frames&&std::fprintf(source.frames,"%s %s %u %016llx\n",source.scene,source.window,cost.frames,(unsigned long long)pixels())<0)return false;
 return app.content.healthy();
}
bool run_scene(Source&source,unsigned scene,unsigned frames){
 static const char*names[]={"map","merchant","card_reward","crystal_full","crystal_revealed","ancient"};
 source.scene=names[scene];source.window="open_and_fixture";Cost init,entry;
 auto io=source.io;auto start=Clock::now();
 if(!app.init({&source,source.length,read_at,nullptr},{save_game})||!setup(scene))return false;
 init.add(start,io,source);init.output(source);source.window="first_and_preparation";
 if(!frame(source,entry,0))return false;
 for(unsigned i=0;app.combat_preparation.active;++i)if(i>=1024||!frame(source,entry,33))return false;
 if(scene==1){
  HitRegion open{};bool found=false;
  for(int i=0;i<app.hit_count;++i)if(app.hits[i].kind==HitKind::UiAction&&app.hits[i].value==int(UiAction::OpenShop)){open=app.hits[i];found=true;break;}
  if(!found)return false;io=source.io;start=Clock::now();
  app.pointer_move(open.x+open.w/2,open.y+open.h/2);app.pointer_button(true);app.pointer_button(false);app.pointer_move(310,35);app.render();entry.add(start,io,source);
  if(!app.shop_open)return false;
 }
 entry.output(source);if(!capture(source)||!save(app.game,before))return false;const auto phase=app.game.phase;
 static const char*windows[]={"animation_cycle_1","animation_cycle_2","animation_cycle_3"};
 for(const char*window:windows){source.window=window;Cost cost;
  for(unsigned i=0;i<frames;++i)if(!frame(source,cost,33)||app.combat_preparation.active||app.game.phase!=phase)return false;
  cost.output(source);if(!capture(source))return false;
 }
 const bool unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size)&&!save_attempts;
 unsigned controls=0;for(int i=0;i<app.hit_count;++i)controls+=app.hits[i].enabled;
 std::printf("{\"scene\":\"%s\",\"canonical_state_unchanged\":%s,\"enabled_controls\":%u,\"save_callback_attempts\":%u}\n",source.scene,unchanged?"true":"false",controls,save_attempts);std::fflush(stdout);
 return unchanged&&controls;
}
}
int main(int argc,char**argv){
 if(argc<2||argc>5){std::fprintf(stderr,"Usage: room_render_benchmark REAL_PACK [FRAMES_PER_WINDOW] [EXISTING_CAPTURE_DIRECTORY|-] [FRAME_TRANSCRIPT]\n");return 2;}
 unsigned frames=argc>=3?unsigned(std::strtoul(argv[2],nullptr,10)):180;if(frames<10||frames>3600)return 2;
 if(argc>=4&&std::strcmp(argv[3],"-"))capture_directory=argv[3];
 Source source;source.file=std::fopen(argv[1],"rb");if(!source.file||std::fseek(source.file,0,SEEK_END))return 3;
 const long length=std::ftell(source.file);if(length<40||uint64_t(length)>MaxContentFile)return 3;source.length=uint32_t(length);
 if(const char*trace=std::getenv("NSPIRE_RESOURCE_TRACE")){source.trace=std::fopen(trace,"wb");if(!source.trace)return 3;}
 if(argc==5){source.frames=std::fopen(argv[4],"wb");if(!source.frames)return 3;}
 std::printf("{\"operation\":\"scope\",\"pack_bytes\":%u,\"frames_per_window\":%u,\"windows\":3,\"step_ms\":33,\"method\":\"Frozen native renderer; real range-read resources; host CPU intervals without pacing; frame hashes outside timed intervals; fixtures do not certify whole-run play\"}\n",source.length,frames);
 for(unsigned i=0;i<6;++i)if(!run_scene(source,i,frames)){std::fprintf(stderr,"room benchmark failed scene %u phase %u healthy %u\n",i,unsigned(app.game.phase),app.content.healthy());return 10+int(i);}
 const bool trace_ok=!source.trace||!std::fclose(source.trace),frames_ok=!source.frames||!std::fclose(source.frames);return !std::fclose(source.file)&&trace_ok&&frames_ok?0:4;
}
