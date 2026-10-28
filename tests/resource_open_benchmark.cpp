// Actual-pack host diagnostic, deliberately not a registered synthetic test.
// Timings include this host/build's instrumentation; they are not device FPS.
#include "app.hpp"
#include "resource_file_cursor.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

using namespace spire;
using Clock=std::chrono::steady_clock;
struct ReadCounts {
    uint64_t bytes=0;uint32_t calls=0,seeks=0,failures=0;
    void add(uint32_t count,bool seek,bool good){bytes+=count;++calls;seeks+=seek;failures+=!good;}
};
struct OpenObservation {
    struct Phase {double milliseconds=0;unsigned reports=0;uint32_t completed=0,total=0;ReadCounts reads;};
    Phase phases[unsigned(ResourceStage::Count)]{};
    ResourceStage phase=ResourceStage::Count;Clock::time_point changed{};
    ReadCounts reads,unphased;
};
struct FileSource {
    FILE* file=nullptr;uint32_t length=0;ResourceFileCursor cursor;OpenObservation* observation=nullptr;
    ~FileSource(){close();}
    bool close(){if(!file)return true;FILE* opened=file;file=nullptr;return std::fclose(opened)==0;}
    bool open(const char* path,OpenObservation& observed){
        file=std::fopen(path,"rb");observation=&observed;
        if(!file||std::fseek(file,0,SEEK_END)!=0)return false;
        const long size=std::ftell(file);
        if(size<=0||uint64_t(size)>MaxContentFile)return false;
        length=uint32_t(size);return true;
    }
};
static void progress(void* context,ResourceStage stage,uint32_t completed,uint32_t total){
    auto& source=*static_cast<FileSource*>(context)->observation;const auto now=Clock::now();
    if(stage!=source.phase){
        if(source.phase!=ResourceStage::Count)source.phases[unsigned(source.phase)].milliseconds+=
            std::chrono::duration<double,std::milli>(now-source.changed).count();
        source.phase=stage;source.changed=now;
    }
    auto& phase=source.phases[unsigned(stage)];++phase.reports;phase.completed=completed;phase.total=total;
}
static int file_seek(void* file,long offset,int origin){return std::fseek(static_cast<FILE*>(file),offset,origin);}
static unsigned file_read(void* output,unsigned size,unsigned count,void* file){return unsigned(std::fread(output,size,count,static_cast<FILE*>(file)));}
static bool read_at(void* context,uint32_t offset,void* output,uint32_t count){
    auto& source=*static_cast<FileSource*>(context);bool seek=false;
    const bool good=offset<=source.length&&count<=source.length-offset&&
        source.cursor.read(source.file,offset,output,count,file_seek,file_read,seek);
    auto& observation=*source.observation;observation.reads.add(count,seek,good);
    if(observation.phase==ResourceStage::Count)observation.unphased.add(count,seek,good);
    else observation.phases[unsigned(observation.phase)].reads.add(count,seek,good);
    return good;
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
static void file_stats(const ReadCounts& counts){
    std::printf("\"file_callbacks\":%u,\"file_requested_bytes\":%llu,\"file_seeks\":%u,\"file_failures\":%u",
        counts.calls,(unsigned long long)counts.bytes,counts.seeks,counts.failures);
}
int main(int argc,char** argv){
    const bool separated=argc>1&&!std::strcmp(argv[1],"--split");
    const int image_arg=separated?5:2;
    if(argc<image_arg||(separated&&std::strcmp(argv[4],"eng")&&std::strcmp(argv[4],"zhs"))){
        std::fprintf(stderr,"Usage: resource_open_benchmark REAL_PACK [IMAGE_ID ...]\n"
            "       resource_open_benchmark --split COMMON LOCALE eng|zhs [IMAGE_ID ...]\n");return 2;
    }
    for(int i=image_arg;i<argc;i++){
        char* end=nullptr;const unsigned long id=std::strtoul(argv[i],&end,10);
        if(!argv[i][0]||*end||id>65534){std::fprintf(stderr,"Invalid image ID\n");return 2;}
    }
    OpenObservation observation;FileSource source,common;SplitResourceSource split;
    if(!source.open(argv[separated?3:1],observation)||(separated&&!common.open(argv[2],observation))){
        std::perror("open resource file");return 2;
    }
    auto content=std::make_unique<Content>();
    const auto start=Clock::now();
    ResourceSource resource{&source,source.length,read_at,nullptr,progress};bool joined=true;
    if(separated){
        joined=split.open({&common,common.length,read_at,nullptr,progress},resource,
            !std::strcmp(argv[4],"eng")?ResourceLanguage::English:ResourceLanguage::Chinese);
        resource=split.source();
    }
    const bool opened=joined&&content->open(resource);
    const double open_ms=elapsed(start);
    if(observation.phase!=ResourceStage::Count)observation.phases[unsigned(observation.phase)].milliseconds+=elapsed(observation.changed);
    std::printf("{\"operation\":\"open\",\"pack_bytes\":%u,\"source_kind\":\"%s\",\"scope\":\"%s\",\"success\":%s,"
                "\"milliseconds\":%.3f,",resource.length,separated?"split":"full",
                separated?"split_open_and_content_open":"content_open",opened?"true":"false",open_ms);
    stats({},content->read_stats());std::putchar(',');file_stats(observation.reads);std::puts("}");std::fflush(stdout);
    static const char*const names[]={"header","integrity","directory","images","tile_index","glyphs","strings","ready"};
    for(unsigned i=0;i<unsigned(ResourceStage::Count);++i){const auto&p=observation.phases[i];
        std::printf("{\"operation\":\"open_phase\",\"phase\":\"%s\",\"milliseconds\":%.3f,\"reports\":%u,\"completed\":%u,\"total\":%u,",
            names[i],p.milliseconds,p.reports,p.completed,p.total);
        file_stats(p.reads);std::puts("}");
    }
    std::printf("{\"operation\":\"unphased_file_reads\",");file_stats(observation.unphased);std::puts("}");
    std::fflush(stdout);
    if(!opened)return 1;
    for(int i=image_arg;i<argc;i++){
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
        if(!picture.pixels||!content->healthy())return 1;
    }
    return source.close()&&common.close()?0:1;
}
