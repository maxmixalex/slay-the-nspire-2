// Actual-pack range-reader benchmark for the production history tooltip.
// The bounded journal below is a synthetic stress fixture, not a claim that
// these events were played. Real command observations have their own suite.
#include "app.hpp"
#include "resource_file_cursor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
using Clock=std::chrono::steady_clock;
namespace {
static App app;static SaveData before,after;
static FILE*transcript=nullptr;static const char*capture_directory=nullptr;
struct Io {uint64_t calls=0,bytes=0,seeks=0;};
struct Source {FILE*file=nullptr;uint32_t length=0;ResourceFileCursor cursor{};Io io{};};
int seek_file(void*f,long offset,int origin){return std::fseek(static_cast<FILE*>(f),offset,origin);}
unsigned read_file(void*out,unsigned size,unsigned count,void*f){return unsigned(std::fread(out,size,count,static_cast<FILE*>(f)));}
bool read_at(void*context,uint32_t offset,void*out,uint32_t count){
 auto&s=*static_cast<Source*>(context);bool sought=false;
 const bool ok=offset<=s.length&&count<=s.length-offset&&s.cursor.read(s.file,offset,out,count,seek_file,read_file,sought);
 ++s.io.calls;s.io.bytes+=count;s.io.seeks+=sought;return ok;
}
uint64_t pixel_hash(){uint64_t h=1469598103934665603ULL;for(uint16_t p:app.pixels){h=(h^uint8_t(p))*1099511628211ULL;h=(h^uint8_t(p>>8))*1099511628211ULL;}return h;}
bool capture(const char*scene,const char*window){
 if(!capture_directory)return true;char path[1024];const int n=std::snprintf(path,sizeof path,"%s/%s-%s.ppm",capture_directory,scene,window);
 if(n<0||n>=int(sizeof path))return false;FILE*f=std::fopen(path,"wb");if(!f)return false;
 bool good=std::fprintf(f,"P6\n320 240\n255\n")>0;uint8_t row[ScreenW*3];
 for(unsigned y=0;y<ScreenH&&good;++y){for(unsigned x=0;x<ScreenW;++x){const uint16_t p=app.pixels[y*ScreenW+x];
   row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}
  good=std::fwrite(row,1,sizeof row,f)==sizeof row;}
 return std::fclose(f)==0&&good;
}
bool fixture(Source&source,unsigned scenario){
 if(!app.init({&source,source.length,read_at,nullptr},{}))return false;
 new_run(app.game,0x7195c3,false,Character::Ironclad);app.reset_presentation();
 app.title=true;app.map_clock_ms=app.animation=0;app.progress_loaded=true;app.progress_dirty=false;clear_ui_progress(app.progress);
 auto&p=app.progress;p.count=p.next_history=2;p.next_serial=3;
 for(unsigned i=0;i<2;++i){auto&r=p.history[i];r.serial=i+1;r.character=unsigned(Character::Ironclad);r.outcome=3;r.abandoned=1;r.partial=1;
  r.floor_count=1;r.floor=1;r.hp=app.game.hp;r.max_hp=app.game.max_hp;r.gold=app.game.gold;r.seed=0x7195c3;r.seed_text[0]='A';
  r.floors[0].floor=1;r.floors[0].hp=r.hp;r.floors[0].max_hp=r.max_hp;r.floors[0].gold=r.gold;r.floors[0].room=unsigned(WorldRoom::Event);
  r.deck_count=app.game.deck_count;for(unsigned j=0;j<r.deck_count;++j)r.deck[j].card=app.game.deck[j];
  p.journal.owners[i].serial=i+1;p.journal.owners[i].observe(0,true);
 }
 for(unsigned i=0;i<UiJournalEntries;++i){
  const unsigned owner=scenario==0?1:scenario==1?(i>=UiJournalEntries-20?1:0):(i%2);
  const Card card{CardKind(i%CardKinds),uint8_t(i&1)};RunHistoryEvent kind;unsigned model=unsigned(card_token(card)),detail=0,flags=0;
  switch(i%7){
   case 0:kind=RunHistoryEvent::CardObtained;break;
   case 1:kind=RunHistoryEvent::CardRemoved;break;
   case 2:kind=RunHistoryEvent::CardTransformed;detail=unsigned(card_token({CardKind((i+1)%CardKinds),0}));break;
   case 3:kind=RunHistoryEvent::CardEnchanted;flags=unsigned(EventEnchantment::Sharp);detail=4;break;
   case 4:kind=RunHistoryEvent::PotionUsed;model=1+i%PotionKinds;break;
   case 5:kind=RunHistoryEvent::RelicSkipped;model=i%WorldRelicKinds;break;
   default:kind=RunHistoryEvent::CardSkipped;break;
  }
  if(!p.journal.append(owner,0,kind,model,detail,flags))return false;
 }
 app.open_history();app.pointer.visible=false;app.history_presentation.focus=0;
 for(unsigned i=0;i<50;++i)app.advance(33);app.render();
 return app.history_tooltip.source==1&&app.history_tooltip.maximum>5&&app.content.healthy()&&save(app.game,before);
}
bool window(Source&source,const char*scene,const char*name,unsigned frames,bool scrolling){
 const Io start_io=source.io;double total=0,maximum=0;uint64_t sequence=1469598103934665603ULL;
 for(unsigned i=0;i<frames;++i){
  const auto start=Clock::now();
  const int next=scrolling?int(uint64_t(i)*unsigned(app.history_tooltip.maximum)/(frames-1)):0;
  app.history_tooltip.scroll(next-app.history_tooltip.offset);app.advance(33);app.render();
  const double ms=std::chrono::duration<double,std::milli>(Clock::now()-start).count();total+=ms;if(ms>maximum)maximum=ms;
  if(!app.content.healthy()||app.history_tooltip.source!=1)return false;
  // Pixel hashing and transcript I/O are deliberately outside the timer.
  const uint64_t pixels=pixel_hash();sequence=(sequence^pixels)*1099511628211ULL;
  if(transcript)std::fprintf(transcript,"%s %s %u %d %016llx\n",scene,name,i,int(app.history_tooltip.offset),(unsigned long long)pixels);
 }
 const bool unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size);
 const auto cache=app.content.image_cache_usage();
 std::printf("{\"scene\":\"%s\",\"window\":\"%s\",\"frames\":%u,\"mean_ms\":%.6f,\"max_ms\":%.6f,\"source_calls\":%llu,\"source_bytes\":%llu,\"source_seeks\":%llu,\"entries\":%u,\"scroll_maximum\":%d,\"image_pages\":%u,\"pixel_sequence_fnv64\":\"%016llx\",\"canonical_state_unchanged\":%s}\n",scene,name,frames,total/frames,maximum,
  (unsigned long long)(source.io.calls-start_io.calls),(unsigned long long)(source.io.bytes-start_io.bytes),(unsigned long long)(source.io.seeks-start_io.seeks),app.progress.journal.count,int(app.history_tooltip.maximum),cache.pages,(unsigned long long)sequence,unchanged?"true":"false");
 std::fflush(stdout);return unchanged&&capture(scene,name);
}
}
int main(int argc,char**argv){
 if(argc<2||argc>5){std::fprintf(stderr,"usage: %s PACK [FRAMES=180] [CAPTURE_DIRECTORY] [TRANSCRIPT]\n",argv[0]);return 2;}
 const unsigned frames=argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):180;if(frames<2||frames>20000)return 2;
 if(argc>3)capture_directory=argv[3];if(argc>4){transcript=std::fopen(argv[4],"wb");if(!transcript)return 2;}
 Source source;source.file=std::fopen(argv[1],"rb");if(!source.file)return 2;
 if(std::fseek(source.file,0,SEEK_END)!=0)return 2;const long length=std::ftell(source.file);if(length<=0||uint64_t(length)>MaxContentFile)return 2;source.length=uint32_t(length);
 const char*names[]={"dense_floor","short_tail_floor","interleaved_floor"};bool good=true;
 for(unsigned scene=0;scene<3&&good;++scene){good=fixture(source,scene);if(good)good=window(source,names[scene],"fixed_1",frames,false)&&window(source,names[scene],"fixed_2",frames,false)&&window(source,names[scene],"scroll_1",frames,true)&&window(source,names[scene],"scroll_2",frames,true);}
 if(transcript)good=(std::fclose(transcript)==0)&&good;good=(std::fclose(source.file)==0)&&good;return good?0:1;
}
