// Actual-pack host diagnostic, deliberately not a registered synthetic test.
// Timings include this host/build's instrumentation; they are not device FPS.
#include "app.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace spire;
using Clock=std::chrono::steady_clock;
struct FileSource {FILE* file;uint32_t length;};
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
    const bool opened=content->open({&source,source.length,read_at,nullptr});
    const double open_ms=elapsed(start);
    std::printf("{\"operation\":\"open\",\"pack_bytes\":%u,\"success\":%s,"
                "\"milliseconds\":%.3f,",source.length,opened?"true":"false",open_ms);
    stats({},content->read_stats());std::puts("}");std::fflush(stdout);
    if(!opened){std::fclose(file);return 1;}
    for(int i=2;i<argc;i++){
        const int id=int(std::strtoul(argv[i],nullptr,10));
        const auto before=content->read_stats();
        const auto image_start=Clock::now();const Picture picture=content->image(id);
        const double image_ms=elapsed(image_start);
        std::printf("{\"operation\":\"image\",\"id\":%d,\"success\":%s,"
                    "\"width\":%u,\"height\":%u,\"format\":%u,\"milliseconds\":%.3f,",
            id,picture.pixels?"true":"false",unsigned(picture.w),unsigned(picture.h),
            unsigned(picture.format),image_ms);
        stats(before,content->read_stats());std::puts("}");std::fflush(stdout);
        if(!picture.pixels||!content->healthy()){std::fclose(file);return 1;}
    }
    return std::fclose(file)==0?0:1;
}
