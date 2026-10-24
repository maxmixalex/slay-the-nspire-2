// Build this same driver against the unchanged baseline and candidate, then
// compare its complete binary transcripts. It observes map and RNG fields,
// never the private workspace representation or object padding.
#include "reference_act.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace spire::reference;
namespace {
static MapWorkspace workspace;
static ReferenceMap map;
static std::vector<unsigned char> transcript;
static void word(uint64_t value,unsigned count=4){
    for(unsigned i=0;i<count;++i)transcript.push_back(static_cast<unsigned char>(value>>(i*8)));
}
static void snapshot(bool ok,QuotaTrace quota){
    word(ok,1);word(unsigned(quota.error),1);word(quota.rest_calls);word(quota.unknown_calls);
    word(map.count,1);word(map.root,1);word(map.boss,1);word(map.second_boss,1);word(unsigned(map.error),1);
    const auto&c=map.config;
    word(c.rows,1);word(c.rests,1);word(c.shops,1);word(c.elites,1);word(c.unknowns,1);
    word(c.replace_treasure,1);word(c.second_boss,1);word(c.prune,1);word(c.variant,1);
    for(auto stage:map.stage_calls)word(stage);
    for(auto state:map.final_rng.words)word(state,8);
    word(map.final_rng.calls);word(map.final_rng.good,1);
    for(const auto&column:map.grid)for(auto point:column)word(point,1);
    for(unsigned id=0;id<map.count;++id){const auto&p=map.points[id];
        word(p.col,1);word(p.row,1);word(unsigned(p.type),1);word(p.active,1);word(p.modifiable,1);
        word(p.parents.used,1);for(auto point:p.parents.slots)word(point,1);
        word(p.children.used,1);for(auto point:p.children.slots)word(point,1);
    }
}
}
int main(int argc,char**argv){
    if(argc!=2){std::fprintf(stderr,"Usage: map_storage_benchmark TRANSCRIPT\n");return 2;}
    Random seeds;seeds.reset(UINT64_C(0xf306b58d7192a4ce));
    unsigned success=0,failed=0,cases=0;
    const auto began=std::chrono::steady_clock::now();
    for(unsigned index=0;index<768;++index){
        ActSettings settings;settings.act=ActKind(index%4);settings.variant=uint8_t(index/4%3);
        settings.increased_elites=bool(index&16);settings.replace_treasure=bool(index&32);
        settings.second_boss=bool(index&64);QuotaTrace quota;
        const auto seed=seeds.next();word(seed,8);
        const bool ok=generate_act_map(map,seed,settings,workspace,quota);
        if(ok&&!map.valid())return 3;
        success+=ok;failed+=!ok;++cases;snapshot(ok,quota);
    }
    // Exercise the full supported row range and the no-pruning constructor,
    // beyond the usual region heights. Reuse one workspace across all cases.
    for(unsigned index=0;index<256;++index){
        MapConfig config;config.rows=uint8_t(10+index%11);config.prune=bool(index&1);
        config.second_boss=bool(index&2);config.replace_treasure=bool(index&4);
        const auto seed=seeds.next();word(seed,8);Random rng;rng.reset(seed);
        const bool ok=generate_map(map,rng,config,workspace);
        if(ok&&!map.valid())return 3;
        success+=ok;failed+=!ok;++cases;snapshot(ok,{});
    }
    const double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();
    FILE*file=std::fopen(argv[1],"wb");if(!file)return 4;
    const bool wrote=std::fwrite(transcript.data(),1,transcript.size(),file)==transcript.size();
    if(std::fclose(file)||!wrote)return 4;
    std::printf("{\"cases\":%u,\"success\":%u,\"explicit_failures\":%u,\"milliseconds\":%.3f,\"workspace_bytes\":%zu,\"segment_bytes\":%zu,\"transcript_bytes\":%zu}\n",
        cases,success,failed,elapsed,sizeof workspace,sizeof(MapWorkspace::Segment),transcript.size());
}
