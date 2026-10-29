// Actual-resource native comparison. CPU time is never calculator FPS.
// Frame fingerprints stay outside timed intervals and must agree between
// independently built checkpoints before comparing their timing samples.
#include "app.hpp"
#include "runtime_render_parts.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <vector>
using namespace spire;
static App app;
static SaveData before,after;
static uint64_t starts[unsigned(RuntimeRenderPart::Count)],parts[unsigned(RuntimeRenderPart::Count)];
static uint64_t cpu(){timespec now{};clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&now);return uint64_t(now.tv_sec)*1000000000u+uint64_t(now.tv_nsec);}
static void profile(RuntimeRenderPart part,bool begin){const auto index=unsigned(part);if(begin)starts[index]=cpu();else parts[index]+=cpu()-starts[index];}
static uint64_t fingerprint(){uint64_t hash=1469598103934665603ULL;for(uint16_t pixel:app.pixels){hash=(hash^uint8_t(pixel))*1099511628211ULL;hash=(hash^uint8_t(pixel>>8))*1099511628211ULL;}return hash;}
int main(int argc,char**argv){
 if(argc<2||argc>4){std::fprintf(stderr,"Usage: random_render_benchmark LOGICAL_PACK [FRAMES] [TRANSCRIPT]\n");return 2;}
 const unsigned frames=argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):300;
 if(frames<10||frames>3600)return 2;
 FILE*file=std::fopen(argv[1],"rb");if(!file||std::fseek(file,0,SEEK_END))return 3;
 const long size=std::ftell(file);if(size<=0||size>256*1024*1024||std::fseek(file,0,SEEK_SET))return 3;
 std::vector<uint8_t> data(size);if(std::fread(data.data(),1,data.size(),file)!=data.size()||std::fclose(file))return 3;
 FILE*transcript=argc>3?std::fopen(argv[3],"wb"):nullptr;if(argc>3&&!transcript)return 3;
 if(!app.init(data.data(),unsigned(data.size()),{}))return 4;
 new_run(app.game,91919,true);
 app.title=app.character_select=app.character_random=true;app.selected_character=Character::Ironclad;
 app.progress.tutorial_flags=FtueFresh|FtueSeenMask;app.map_clock_ms=24000;app.render();
 for(unsigned frame=0;frame<60;++frame){app.advance(33);app.render();}
 if(!save(app.game,before))return 5;
 app.callbacks.profile_render=profile;uint64_t total=0,peak=0;
 for(unsigned frame=0;frame<frames;++frame){
  app.advance(33);const auto start=cpu();app.render();const auto elapsed=cpu()-start;
  total+=elapsed;if(elapsed>peak)peak=elapsed;
  if(transcript&&std::fprintf(transcript,"%u %016llx\n",frame,(unsigned long long)fingerprint())<0)return 6;
 }
 const bool unchanged=save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size);
 std::printf("{\"frames\":%u,\"cpu_ms\":%.6f,\"max_ms\":%.6f,\"water_ms\":%.6f,\"light_ms\":%.6f,\"dots_ms\":%.6f,\"canonical_unchanged\":%s,\"native_cpu_not_device_fps\":true}\n",
  frames,total/1000000.,peak/1000000.,parts[0]/1000000.,parts[1]/1000000.,parts[2]/1000000.,unchanged?"true":"false");
 return unchanged&&app.content.healthy()&&(!transcript||!std::fclose(transcript))?0:7;
}
