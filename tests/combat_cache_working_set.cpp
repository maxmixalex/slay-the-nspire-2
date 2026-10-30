// Actual-resource diagnostic. Logical source IO and deterministic frame-cache
// residency are measured here; host elapsed intervals are not calculator FPS.
#include "app.hpp"
#include "resource_file_cursor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace spire;
using Clock=std::chrono::steady_clock;
static App app;
static std::vector<uint8_t> run_bytes,progress_bytes;
static bool load_run(State&s){return load(s,run_bytes.data(),run_bytes.size());}
static bool load_progress(UiProgress&p){return decode_ui_progress(progress_bytes.data(),progress_bytes.size(),p);}
static bool save_run(const State&){return true;}
static bool save_progress(const UiProgress&){return true;}
static bool clear_run(){return true;}
struct Source {
    FILE* file{};uint32_t size{};ResourceFileCursor cursor{};
    uint64_t calls=0,bytes=0,seeks=0;const char*phase="open";unsigned frame=0;
};
static int seek_file(void*f,long off,int origin){return std::fseek(static_cast<FILE*>(f),off,origin);}
static unsigned read_file(void*out,unsigned n,unsigned count,void*f){return unsigned(std::fread(out,n,count,static_cast<FILE*>(f)));}
static bool read_resource(void*ctx,uint32_t offset,void*out,uint32_t count){
    auto&s=*static_cast<Source*>(ctx);bool sought=false;
    const bool ok=offset<=s.size&&count<=s.size-offset&&s.cursor.read(s.file,offset,out,count,seek_file,read_file,sought);
    ++s.calls;s.bytes+=count;s.seeks+=sought;
    if(s.phase[0]=='c')std::printf("{\"operation\":\"read\",\"phase\":\"%s\",\"frame\":%u,\"offset\":%u,\"bytes\":%u,\"ok\":%s}\n",s.phase,s.frame,offset,count,ok?"true":"false");
    return ok;
}
static std::vector<uint8_t> read(const char*path){
    FILE*f=std::fopen(path,"rb");if(!f||std::fseek(f,0,SEEK_END))std::exit(2);
    const long n=std::ftell(f);if(n<=0||std::fseek(f,0,SEEK_SET))std::exit(2);
    std::vector<uint8_t>b(static_cast<size_t>(n));if(std::fread(b.data(),1,b.size(),f)!=b.size()||std::fclose(f))std::exit(2);return b;
}
static void usage(const char*stage){
    const auto u=app.content.image_cache_usage();const auto&p=app.combat_preparation;
    unsigned resident=0;for(unsigned i=0;i<p.count;++i)resident+=app.content.image_cached(int(p.images[i]));
    std::printf("{\"operation\":\"cache\",\"phase\":\"%s\",\"entries\":%u,\"pages\":%u,\"planned\":%u,\"planned_pages\":%u,\"planned_resident\":%u,\"bounded\":%s,\"hp\":%d}\n",
        stage,u.entries,u.pages,p.count,p.pages,resident,p.bounded?"true":"false",app.game.hp);
}
static bool window(Source&s,const char*name,unsigned frames){
    s.phase=name;const auto calls=s.calls,bytes=s.bytes,seeks=s.seeks;const auto decodes=app.content.retained_decode_serial();const auto start=Clock::now();double maximum=0;
    for(unsigned i=0;i<frames;++i){s.frame=i;const auto begin=Clock::now();app.advance(33);app.render();
        const double ms=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();if(ms>maximum)maximum=ms;
        if(!app.content.healthy()||app.title||app.combat_preparation.active)return false;
    }
    const double ms=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
    std::printf("{\"operation\":\"window\",\"phase\":\"%s\",\"frames\":%u,\"host_elapsed_ms\":%.3f,\"max_host_ms\":%.3f,\"calls\":%llu,\"bytes\":%llu,\"seeks\":%llu,\"retained_decodes\":%u}\n",
        name,frames,ms,maximum,(unsigned long long)(s.calls-calls),(unsigned long long)(s.bytes-bytes),(unsigned long long)(s.seeks-seeks),app.content.retained_decode_serial()-decodes);usage(name);std::fflush(stdout);return true;
}
static void input(Source&s,const char*name,Key key){
    s.phase=name;const auto calls=s.calls,bytes=s.bytes;const auto decodes=app.content.retained_decode_serial();app.input(key);app.render();
    std::printf("{\"operation\":\"input\",\"phase\":\"%s\",\"calls\":%llu,\"bytes\":%llu,\"retained_decodes\":%u,\"shop_open\":%s}\n",
        name,(unsigned long long)(s.calls-calls),(unsigned long long)(s.bytes-bytes),app.content.retained_decode_serial()-decodes,app.shop_open?"true":"false");
}
int main(int argc,char**argv){
    if(argc!=4){std::fprintf(stderr,"Usage: combat_cache_working_set LOGICAL_PACK SAVE PROGRESS\n");return 2;}
    run_bytes=read(argv[2]);progress_bytes=read(argv[3]);FILE*f=std::fopen(argv[1],"rb");if(!f||std::fseek(f,0,SEEK_END))return 2;
    const long n=std::ftell(f);if(n<=0||uint64_t(n)>MaxContentFile||std::fseek(f,0,SEEK_SET))return 2;
    Source source{f,uint32_t(n)};Callbacks cb{save_run,load_run,clear_run};cb.load_progress=load_progress;cb.save_progress=save_progress;
    if(!app.init(ResourceSource{&source,uint32_t(n),read_resource,nullptr},cb))return 3;
    source.phase="title";for(unsigned i=0;i<610;++i){app.advance(33);app.render();}
    std::printf("{\"operation\":\"title\",\"has_save\":%s,\"ftue\":%s,\"cursor\":%d}\n",app.has_save?"true":"false",app.ftue.active()?"true":"false",app.cursor);
    app.input(Key::Confirm);app.render();
    // Continue uses the original800ms outgoing fade before replacing title.
    for(unsigned i=0;app.title&&i<120;++i){app.advance(33);app.render();}
    if(app.title){std::fprintf(stderr,"Continue did not enter combat\n");return 4;}
    source.phase="prepare";unsigned updates=0;
    while(app.combat_preparation.active){if(++updates>4096)return 5;app.advance(33);app.render();}
    if((app.game.phase!=Phase::Combat&&app.game.phase!=Phase::Shop)||!app.content.healthy())return 6;
    std::printf("{\"operation\":\"preparation\",\"updates\":%u,\"enemies\":%d}\n",updates,app.game.enemy_count);usage("prepared");
    if(app.game.phase==Phase::Shop){
        if(!window(source,"closed_first",790))return 11; //18s Continue wait plus8s closed.
        input(source,"cold_open_input",Key::Confirm);if(!app.shop_open)return 12;
        if(!window(source,"cold_open",152))return 13;
        input(source,"cold_close_input",Key::Back);if(app.shop_open)return 14;
        if(!window(source,"closed_middle",122))return 15;
        input(source,"cold_reopen_input",Key::Confirm);if(!app.shop_open)return 16;
        if(!window(source,"cold_reopen",152))return 17;
        input(source,"cold_reclose_input",Key::Back);if(app.shop_open)return 18;
        if(!window(source,"closed_last",31))return 19;
        std::fclose(f);return 0;
    }
    for(int i=0;i<app.game.enemy_count;++i){const auto&m=app.enemy_presentation[i];const auto c=app.content.clip(m.clip);unsigned pages=0;
        for(unsigned j=0;j<c.count;++j)pages+=app.content.image_cache_pages(int(c.first+j));
        std::printf("{\"operation\":\"enemy_clip\",\"model\":%u,\"clip\":%d,\"first\":%u,\"count\":%u,\"frame_ms\":%u,\"pages\":%u}\n",m.model,m.clip,c.first,c.count,c.frame_ms,pages);
    }
    if(!window(source,"combat_idle1",720))return 7;
    source.phase="combat_input";app.input(Key::EndTurn);app.render();
    if(!window(source,"combat_after_turn",720))return 8;
    if(!window(source,"combat_idle2",720))return 9;
    if(!window(source,"combat_idle3",720))return 10;
    std::fclose(f);return 0;
}
