// Private-resource rendering measurement. Public code contains no source text
// or artwork; host CPU timings do not establish calculator frame rates.
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
struct Source {FILE*file=nullptr;uint32_t length=0;ResourceFileCursor cursor{};uint64_t calls=0,bytes=0,seeks=0;};
int seek_file(void*f,long at,int origin){return std::fseek(static_cast<FILE*>(f),at,origin);}
unsigned read_file(void*out,unsigned size,unsigned count,void*f){return unsigned(std::fread(out,size,count,static_cast<FILE*>(f)));}
bool read_at(void*context,uint32_t at,void*out,uint32_t count){
 auto&s=*static_cast<Source*>(context);bool sought=false;
 const bool okay=at<=s.length&&count<=s.length-at&&s.cursor.read(s.file,at,out,count,seek_file,read_file,sought);
 ++s.calls;s.bytes+=count;s.seeks+=sought;return okay;
}
bool local_report(const char*,unsigned){return true;}
uint64_t fingerprint(){uint64_t hash=1469598103934665603ULL;
 for(uint16_t pixel:app.pixels){hash=(hash^uint8_t(pixel))*1099511628211ULL;hash=(hash^uint8_t(pixel>>8))*1099511628211ULL;}return hash;
}
bool capture(const char*directory,const char*name){
 if(!directory)return true;char path[1024];const int length=std::snprintf(path,sizeof path,"%s/%s.ppm",directory,name);
 if(length<0||length>=int(sizeof path))return false;FILE*f=std::fopen(path,"wb");if(!f)return false;
 bool good=std::fprintf(f,"P6\n320 240\n255\n")>0;uint8_t line[ScreenW*3];
 for(unsigned y=0;y<ScreenH&&good;++y){for(unsigned x=0;x<ScreenW;++x){const auto p=app.pixels[y*ScreenW+x];
  line[x*3]=uint8_t((p>>11)*255/31);line[x*3+1]=uint8_t(((p>>5)&63)*255/63);line[x*3+2]=uint8_t((p&31)*255/31);}
  good=std::fwrite(line,1,sizeof line,f)==sizeof line;
 }return !std::fclose(f)&&good;
}
bool run(Source&source,unsigned scene,unsigned frames,const char*directory){
 static const char*names[]={"notes-latest","credits-start","credits-roles","credits-localization","credits-groups","feedback-empty","feedback-editor"};
 if(!app.init({&source,source.length,read_at,nullptr},{}))return false;
 // The title screen intentionally has no active save. Use a valid canonical
 // state so read-only modal rendering can be checked with the real codec.
 new_run(app.game,0x41cef1eau,true);app.reset_presentation();
 app.callbacks.write_feedback=local_report;app.pointer.visible=false;
 app.open_auxiliary(scene==0?AuxiliaryScreen::PatchNotes:scene<5?AuxiliaryScreen::Credits:AuxiliaryScreen::Feedback);
 for(unsigned i=0;i<20;++i)app.advance(100);
 if(scene>0&&scene<5){static const int offsets[]={0,0,400,1100,2300};app.auxiliary.credits_position=app.auxiliary.credits_target=offsets[scene]*256;}
 if(scene==6){app.auxiliary_control(AuxiliaryFeedbackText);
  const char*sample="Keyboard input remains local.\nCards, maps and encounters.";
  for(const char*p=sample;*p;++p)app.auxiliary_text(uint8_t(*p));
 }
 const auto cold_start=Clock::now();const uint64_t cold_reads=source.calls;app.render();
 const double cold_ms=std::chrono::duration<double,std::milli>(Clock::now()-cold_start).count();
 if(!app.content.healthy()||!capture(directory,names[scene])||!save(app.game,before))return false;
 const auto start_hash=fingerprint();const auto calls=source.calls,bytes=source.bytes,seeks=source.seeks;double total=0,peak=0;
 for(unsigned i=0;i<frames;++i){const auto start=Clock::now();app.render();
  const double took=std::chrono::duration<double,std::milli>(Clock::now()-start).count();total+=took;if(took>peak)peak=took;
  if(!app.content.healthy()||fingerprint()!=start_hash)return false;
 }
 const bool unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size);
 std::printf("{\"scene\":\"%s\",\"frames\":%u,\"first_render_ms\":%.6f,\"first_render_reads\":%llu,\"mean_render_ms\":%.6f,\"maximum_render_ms\":%.6f,\"source_calls\":%llu,\"source_bytes\":%llu,\"source_seeks\":%llu,\"frame_fnv64\":\"%016llx\",\"canonical_state_unchanged\":%s}\n",names[scene],frames,cold_ms,(unsigned long long)(calls-cold_reads),total/frames,peak,(unsigned long long)(source.calls-calls),(unsigned long long)(source.bytes-bytes),(unsigned long long)(source.seeks-seeks),(unsigned long long)start_hash,unchanged?"true":"false");
 std::fflush(stdout);return unchanged;
}
}
int main(int argc,char**argv){
 if(argc<2||argc>4){std::fprintf(stderr,"Usage: auxiliary_render_benchmark REAL_PACK [FRAMES] [EXISTING_CAPTURE_DIRECTORY]\n");return 2;}
 const unsigned frames=argc>=3?unsigned(std::strtoul(argv[2],nullptr,10)):120;if(!frames||frames>3600)return 2;
 Source source;source.file=std::fopen(argv[1],"rb");if(!source.file||std::fseek(source.file,0,SEEK_END))return 3;
 const long size=std::ftell(source.file);if(size<40||uint64_t(size)>MaxContentFile)return 3;source.length=uint32_t(size);
 std::printf("{\"method\":\"Frozen native renderer with real range-read resources; stationary whole-frame hashes outside timed intervals; no gameplay or device FPS claim\",\"pack_bytes\":%u}\n",source.length);
 for(unsigned scene=0;scene<7;++scene)if(!run(source,scene,frames,argc==4?argv[3]:nullptr)){std::fprintf(stderr,"auxiliary benchmark failed scene %u\n",scene);return 10+int(scene);}
 return std::fclose(source.file)?4:0;
}
