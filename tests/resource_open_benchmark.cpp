// Actual-pack host diagnostic, deliberately not a registered synthetic test.
// Timings include this host/build's instrumentation; they are not device FPS.
#include "app.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace spire;
using Clock=std::chrono::steady_clock;
struct FileSource {
    FILE* file;uint32_t length;
    struct Phase {double milliseconds=0;unsigned reports=0;uint32_t completed=0,total=0;};
    Phase phases[unsigned(ResourceStage::Count)]{};
    ResourceStage phase=ResourceStage::Count;Clock::time_point changed{};
};
static void progress(void* context,ResourceStage stage,uint32_t completed,uint32_t total){
    auto& source=*static_cast<FileSource*>(context);const auto now=Clock::now();
    if(stage!=source.phase){
        if(source.phase!=ResourceStage::Count)source.phases[unsigned(source.phase)].milliseconds+=
            std::chrono::duration<double,std::milli>(now-source.changed).count();
        source.phase=stage;source.changed=now;
    }
    auto& phase=source.phases[unsigned(stage)];++phase.reports;phase.completed=completed;phase.total=total;
}
static bool read_at(void* context,uint32_t offset,void* output,uint32_t count){
    auto& source=*static_cast<FileSource*>(context);
    return offset<=source.length&&count<=source.length-offset&&
        std::fseek(source.file,long(offset),SEEK_SET)==0&&
        std::fread(output,1,count,source.file)==count;
}
static double elapsed(Clock::time_point start){
    return std::chrono::duration<double,std::milli>(Clock::now()-start).count();
}
static void stats(const ResourceReadStats& before,const ResourceReadStats& after){
    std::printf("\"callbacks\":%u,\"callback_bytes\":%llu,\"reader_page_hits\":%u,"
                "\"reader_page_misses\":%u,\"range_bypasses\":%u",
        after.callbacks-before.callbacks,
        (unsigned long long)(after.callback_bytes-before.callback_bytes),
        after.cache_hits-before.cache_hits,after.cache_misses-before.cache_misses,
        after.bypasses-before.bypasses);
}
int main(int argc,char** argv){
    if(argc<2){std::fprintf(stderr,"Usage: resource_open_benchmark REAL_PACK [IMAGE_ID ...]\n");return 2;}
    FILE* file=std::fopen(argv[1],"rb");
    if(!file){std::perror("open pack");return 2;}
    if(std::fseek(file,0,SEEK_END)!=0){std::fclose(file);return 2;}
    const long length=std::ftell(file);
    if(length<=0||uint64_t(length)>MaxContentFile){std::fclose(file);return 2;}
    for(int i=2;i<argc;i++){
        char* end=nullptr;const unsigned long id=std::strtoul(argv[i],&end,10);
        if(!argv[i][0]||*end||id>65534){std::fprintf(stderr,"Invalid image ID\n");std::fclose(file);return 2;}
    }
    FileSource source{file,uint32_t(length)};
    auto content=std::make_unique<Content>();
    const auto start=Clock::now();
    const bool opened=content->open({&source,source.length,read_at,nullptr,progress});
    const double open_ms=elapsed(start);
    if(source.phase!=ResourceStage::Count)source.phases[unsigned(source.phase)].milliseconds+=elapsed(source.changed);
    std::printf("{\"operation\":\"open\",\"pack_bytes\":%u,\"success\":%s,"
                "\"milliseconds\":%.3f,",source.length,opened?"true":"false",open_ms);
    stats({},content->read_stats());std::puts("}");std::fflush(stdout);
    static const char*const names[]={"header","integrity","directory","images","tile_index","glyphs","strings","ready"};
    for(unsigned i=0;i<unsigned(ResourceStage::Count);++i){const auto&p=source.phases[i];
        std::printf("{\"operation\":\"open_phase\",\"phase\":\"%s\",\"milliseconds\":%.3f,\"reports\":%u,\"completed\":%u,\"total\":%u}\n",
            names[i],p.milliseconds,p.reports,p.completed,p.total);
    }
    std::fflush(stdout);
    if(!opened){std::fclose(file);return 1;}
    for(int i=2;i<argc;i++){
        const int id=int(std::strtoul(argv[i],nullptr,10));
        const auto before=content->read_stats();
        const bool cached=content->image_cached(id);const unsigned pages=content->image_cache_pages(id);
        const auto image_start=Clock::now();const Picture picture=content->image(id);
        const double image_ms=elapsed(image_start);
        const auto resident=content->image_cache_usage();
        std::printf("{\"operation\":\"image\",\"id\":%d,\"success\":%s,"
                    "\"width\":%u,\"height\":%u,\"format\":%u,\"milliseconds\":%.3f,"
                    "\"cache_hit_before\":%s,\"cache_pages\":%u,\"resident_entries\":%u,\"resident_pages\":%u,",
            id,picture.pixels?"true":"false",unsigned(picture.w),unsigned(picture.h),
            unsigned(picture.format),image_ms,cached?"true":"false",pages,resident.entries,resident.pages);
        stats(before,content->read_stats());std::puts("}");std::fflush(stdout);
        if(!picture.pixels||!content->healthy()){std::fclose(file);return 1;}
    }
    return std::fclose(file)==0?0:1;
}
