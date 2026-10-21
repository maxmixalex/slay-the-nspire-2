// Owner-pack diagnostic, deliberately not registered as a synthetic CTest.
// Compile this same source with SPIRE_PREPARATION_BASELINE and a frozen older
// tree's headers/library for a comparison without changing either runtime.
// No source, save or resource file is modified. Times are host CPU elapsed
// intervals without 30 Hz pacing, never calculator or browser display FPS.
#include "app.hpp"
#include "resource_file_cursor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace spire;
using Clock=std::chrono::steady_clock;
static App app;
static SaveData original,canonical_before,canonical_after;
static unsigned save_attempts=0;
struct Io {uint64_t calls=0,bytes=0,seeks=0;double milliseconds=0;};
struct Source {FILE* file;uint32_t length;ResourceFileCursor cursor{};Io io{};};
static int seek_file(void*file,long offset,int origin){return std::fseek(static_cast<FILE*>(file),offset,origin);}
static unsigned read_file(void*output,unsigned size,unsigned count,void*file){return unsigned(std::fread(output,size,count,static_cast<FILE*>(file)));}
static double milliseconds(Clock::time_point begin){return std::chrono::duration<double,std::milli>(Clock::now()-begin).count();}
static bool read_resource(void*context,uint32_t offset,void*output,uint32_t count){
    auto&s=*static_cast<Source*>(context);const auto started=Clock::now();bool sought=false;
    bool good=offset<=s.length&&count<=s.length-offset&&s.cursor.read(s.file,offset,output,count,seek_file,read_file,sought);
    ++s.io.calls;s.io.bytes+=count;s.io.seeks+=sought;s.io.milliseconds+=milliseconds(started);return good;
}
static bool load_game(State&s){return load(s,original.bytes,original.size);}
static bool save_game(const State&){++save_attempts;return false;}
struct Mark {Clock::time_point time;Io io;};
static Mark mark(const Source&s){return {Clock::now(),s.io};}
struct Cost {
    unsigned samples=0;double elapsed=0,maximum=0;Io io{};
    void add(const Mark&start,const Source&s){
        const double duration=milliseconds(start.time);++samples;elapsed+=duration;if(duration>maximum)maximum=duration;
        io.calls+=s.io.calls-start.io.calls;io.bytes+=s.io.bytes-start.io.bytes;io.seeks+=s.io.seeks-start.io.seeks;
        io.milliseconds+=s.io.milliseconds-start.io.milliseconds;
    }
    void merge(const Cost&other){samples+=other.samples;elapsed+=other.elapsed;if(other.maximum>maximum)maximum=other.maximum;
        io.calls+=other.io.calls;io.bytes+=other.io.bytes;io.seeks+=other.io.seeks;io.milliseconds+=other.io.milliseconds;}
    void write(const char*phase)const{
        std::printf("{\"operation\":\"stage\",\"phase\":\"%s\",\"samples\":%u,\"cpu_elapsed_ms\":%.3f,\"max_sample_ms\":%.3f,"
                    "\"source_calls\":%llu,\"source_bytes\":%llu,\"source_seeks\":%llu,\"source_cpu_ms\":%.3f}\n",
            phase,samples,elapsed,maximum,(unsigned long long)io.calls,(unsigned long long)io.bytes,(unsigned long long)io.seeks,io.milliseconds);
    }
};
static bool preparing(){
#ifdef SPIRE_PREPARATION_BASELINE
    return false;
#else
    return app.combat_preparation.active;
#endif
}
static uint32_t framebuffer_hash(){
    uint32_t value=2166136261u;for(uint16_t pixel:app.pixels){value=(value^uint8_t(pixel))*16777619u;value=(value^uint8_t(pixel>>8))*16777619u;}return value;
}
int main(int argc,char**argv){
    if(argc<3||argc>4){std::fprintf(stderr,"Usage: combat_preparation_benchmark REAL_PACK CANONICAL_COMBAT_SAVE [FRAMES]\n");return 2;}
    unsigned frames=720;if(argc==4){char*end=nullptr;const unsigned long value=std::strtoul(argv[3],&end,10);
        if(!argv[3][0]||*end||!value||value>3600)return 2;frames=unsigned(value);}
    FILE*file=std::fopen(argv[2],"rb");if(!file)return 3;
    original.size=std::fread(original.bytes,1,sizeof original.bytes,file);
    const bool save_ok=std::fgetc(file)==EOF&&!std::ferror(file);std::fclose(file);if(!save_ok)return 3;
    file=std::fopen(argv[1],"rb");if(!file)return 4;
    if(std::fseek(file,0,SEEK_END)){std::fclose(file);return 4;}const long length=std::ftell(file);
    if(length<=0||uint64_t(length)>MaxContentFile){std::fclose(file);return 4;}
    Source source{file,uint32_t(length)};Cost opened,title,input,preparation,first,total;
    auto started=mark(source);
    if(!app.init({&source,source.length,read_resource,nullptr},{save_game,load_game})||!app.title){std::fclose(file);return 5;}
    opened.add(started,source);started=mark(source);app.render();title.add(started,source);
    started=mark(source);app.input(Key::Confirm);input.add(started,source);
    if(app.title||app.game.phase!=Phase::Combat||!save(app.game,canonical_before)){std::fclose(file);return 6;}
    const unsigned start_animation=app.animation;unsigned preparation_updates=0;
    started=mark(source);app.render();
    if(preparing()){
        preparation.add(started,source);
        while(preparing()){
            if(++preparation_updates>1024){std::fclose(file);return 7;}
            started=mark(source);app.advance(33);preparation.add(started,source);
            if(app.animation!=start_animation||!app.content.healthy()){std::fclose(file);return 8;}
            if(preparing()){started=mark(source);app.render();preparation.add(started,source);}
        }
        started=mark(source);app.render();first.add(started,source);
    }else first.add(started,source);
    if(!app.content.healthy()){std::fclose(file);return 9;}
    std::printf("{\"operation\":\"scope\",\"pack_bytes\":%u,\"save_bytes\":%zu,\"frames\":%u,\"elapsed_ms_per_frame\":33,"
                "\"preparation_updates\":%u,\"preparation_pacing_floor_ms\":%u,\"seed\":%u,\"enemy_count\":%d,"
                "\"method\":\"Host steady-clock CPU intervals, no frame pacing or compositor; source is ranged FILE with cursor. Includes preparation plus first visible and all declared windows.\"}\n",
        source.length,original.size,frames,preparation_updates,preparation_updates*33,app.game.seed,app.game.enemy_count);
#ifndef SPIRE_PREPARATION_BASELINE
    const auto&p=app.combat_preparation;unsigned resident=0;
    for(unsigned i=0;i<p.count;++i)resident+=app.content.image_cached(int(p.images[i]));
    std::printf("{\"operation\":\"prepared_set\",\"images\":%u,\"pages\":%u,\"cold_images\":%u,\"bounded\":%s,"
                "\"resident_after_first_visible\":%u,\"source_calls\":%u,\"source_bytes\":%llu}\n",
        p.count,p.pages,p.cold_images,p.bounded?"true":"false",resident,p.read_calls,(unsigned long long)p.read_bytes);
#endif
    opened.write("open");title.write("title");input.write("continue_input");preparation.write("preparation");first.write("first_visible");
    total.merge(input);total.merge(preparation);total.merge(first);total.write("through_first_visible");
    std::printf("{\"operation\":\"frame\",\"frame\":-1,\"rgb565_fnv1a\":%u}\n",framebuffer_hash());std::fflush(stdout);
    for(unsigned base=0;base<frames;base+=180){Cost window;const unsigned stop=base+180<frames?base+180:frames;
        for(unsigned i=base;i<stop;++i){started=mark(source);app.advance(33);app.render();window.add(started,source);
            if(!app.content.healthy()||app.game.phase!=Phase::Combat||preparing()){std::fclose(file);return 10;}}
        char name[32];std::snprintf(name,sizeof name,"frames_%u_%u",base,stop-1);window.write(name);total.merge(window);
        std::printf("{\"operation\":\"frame\",\"frame\":%u,\"rgb565_fnv1a\":%u}\n",stop-1,framebuffer_hash());std::fflush(stdout);
    }
    total.write("complete_combat_observation");
    const bool unchanged=save(app.game,canonical_after)&&canonical_before.size==canonical_after.size&&
        !std::memcmp(canonical_before.bytes,canonical_after.bytes,canonical_before.size)&&!save_attempts;
    std::printf("{\"operation\":\"canonical_state\",\"unchanged\":%s,\"save_callback_attempts\":%u}\n",unchanged?"true":"false",save_attempts);
    const bool closed=std::fclose(file)==0;return unchanged&&closed?0:11;
}
