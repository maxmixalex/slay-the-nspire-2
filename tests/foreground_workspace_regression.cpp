#include "resource_transfer.hpp"
#include "reference_act.hpp"
#include "lzma2_block.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace spire;
using namespace spire::reference;
using Bytes=std::vector<uint8_t>;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"foreground workspace line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static MapWorkspace independent;
static State original,restored;
static SaveData canonical,encoded,again;

static void word(Bytes&out,uint64_t value,unsigned bytes=4){
    for(unsigned i=0;i<bytes;++i)out.push_back(uint8_t(value>>(8*i)));
}
static Bytes snapshot(const ReferenceMap&m,const QuotaTrace&q){
    Bytes out;
    word(out,m.count);word(out,m.root);word(out,m.boss);word(out,m.second_boss);word(out,unsigned(m.error));
    const auto&c=m.config;
    for(unsigned value:{unsigned(c.rows),unsigned(c.rests),unsigned(c.shops),unsigned(c.elites),unsigned(c.unknowns),
        unsigned(c.replace_treasure),unsigned(c.second_boss),unsigned(c.prune),unsigned(c.variant),unsigned(c.ignore_elite_rules)})word(out,value);
    for(auto value:m.stage_calls)word(out,value);
    for(auto value:m.final_rng.words)word(out,value,8);
    word(out,m.final_rng.calls);word(out,m.final_rng.good);
    word(out,unsigned(q.error));word(out,q.rest_calls);word(out,q.unknown_calls);
    for(const auto&column:m.grid)for(auto id:column)word(out,id,1);
    for(unsigned i=0;i<m.count;++i){const auto&p=m.points[i];
        word(out,p.col,1);word(out,p.row,1);word(out,unsigned(p.type),1);word(out,p.active,1);word(out,p.modifiable,1);
        word(out,p.parents.used,1);for(auto id:p.parents.slots)word(out,id,1);
        word(out,p.children.used,1);for(auto id:p.children.slots)word(out,id,1);
    }
    return out;
}
static Bytes workspace_bytes(){
    const auto*bytes=reinterpret_cast<const uint8_t*>(&foreground_workspace);
    return {bytes,bytes+sizeof foreground_workspace};
}
static void same_workspace(const Bytes&before){
    CHECK(before.size()==sizeof foreground_workspace);
    CHECK(!std::memcmp(before.data(),&foreground_workspace,sizeof foreground_workspace));
}
static void map_rejected(){
    ReferenceMap out;QuotaTrace trace;const Bytes before=workspace_bytes();
    CHECK(!generate_act_map(out,731,ActSettings{},trace));
    CHECK(out.error==MapError::Capacity);same_workspace(before);
}
static void lease_conflicts(){
    new_run(original,0x717331,true);new_run(restored,17,true,Character::Defect);
    CHECK(save(original,canonical));
    for(auto use:{ResourceTransferUse::Bytes,ResourceTransferUse::Save,ResourceTransferUse::Codec,ResourceTransferUse::Map}){
        CHECK(!resource_transfer_uses);
        // Every member activation is trivial: it begins a lifetime without a
        // hidden aggregate constructor or a full-buffer zero initialization.
        std::memset(&foreground_workspace,0xa7,sizeof foreground_workspace);
        const Bytes inactive=workspace_bytes();
        {ResourceTransferLease lease(use);CHECK(lease);same_workspace(inactive);
            CHECK(resource_transfer_uses==unsigned(use));
            CHECK(bool(lease.bytes())==(use==ResourceTransferUse::Bytes));
            CHECK(bool(lease.save())==(use==ResourceTransferUse::Save));
            CHECK(bool(lease.state_storage())==(use==ResourceTransferUse::Codec));
            CHECK(bool(lease.map_workspace())==(use==ResourceTransferUse::Map));
            map_rejected();
            for(auto other:{ResourceTransferUse::Bytes,ResourceTransferUse::Save,ResourceTransferUse::Codec,ResourceTransferUse::Map}){
                const Bytes before=workspace_bytes();
                {ResourceTransferLease nested(other);
                    const bool allowed=use==ResourceTransferUse::Save&&other==ResourceTransferUse::Codec;
                    CHECK(bool(nested)==allowed);same_workspace(before);
                }
                CHECK(resource_transfer_uses==unsigned(use));same_workspace(before);
            }
            if(use==ResourceTransferUse::Save){
                auto&buffer=*lease.save();std::memset(&buffer,0x6d,sizeof buffer);
                CHECK(save(original,buffer));CHECK(buffer.size==canonical.size);
                CHECK(!std::memcmp(buffer.bytes,canonical.bytes,canonical.size));
                CHECK(load(restored,buffer.bytes,buffer.size));
                CHECK(!std::memcmp(buffer.bytes,canonical.bytes,canonical.size));
                map_rejected();
            }else{
                const Bytes before=workspace_bytes();
                std::memset(&encoded,0x3c,sizeof encoded);const auto*target=reinterpret_cast<const uint8_t*>(&restored);
                const Bytes saved_target(target,target+sizeof restored);
                CHECK(!save(original,encoded));CHECK(!load(restored,canonical.bytes,canonical.size));
                for(auto byte:encoded.bytes)CHECK(byte==0x3c);
                CHECK(!std::memcmp(saved_target.data(),&restored,sizeof restored));same_workspace(before);
            }
        }
        CHECK(!resource_transfer_uses);
    }
}
static void image_transfer(unsigned salt){
    Bytes expected(Lzma2BlockRawLimit),decoded(expected.size());
    for(unsigned i=0;i<expected.size();++i)expected[i]=uint8_t((i*31u+i/97u)^salt);
    {ResourceTransferLease transfer(ResourceTransferUse::Bytes);CHECK(transfer);
        unsigned at=0;
        for(unsigned start=0;start<expected.size();start+=65536){
            transfer.bytes()[at++]=start?2:1;transfer.bytes()[at++]=255;transfer.bytes()[at++]=255;
            std::memcpy(transfer.bytes()+at,expected.data()+start,65536);at+=65536;
        }
        transfer.bytes()[at++]=0;CHECK(at<=ResourceTransferCapacity);
        map_rejected();CHECK(decode_lzma2_block(decoded.data(),unsigned(decoded.size()),unsigned(expected.size()),transfer.bytes(),at,14));
        CHECK(decoded==expected);
    }
    CHECK(!resource_transfer_uses);
}
static void alternating_maps_and_serialization(){
    for(unsigned index=0;index<48;++index){
        ActSettings settings;settings.act=ActKind(index%4);settings.variant=uint8_t(index/4%3);
        settings.increased_elites=bool(index&4);settings.replace_treasure=bool(index&8);
        settings.second_boss=bool(index&16);settings.big_game_hunter=bool(index&32);
        const uint64_t seed=UINT64_C(0xc1703b19)+UINT64_C(0x9e3779b97f4a7c15)*index;
        ReferenceMap expected,actual;QuotaTrace first,second;
        std::memset(&independent,0x51,sizeof independent);
        const bool good=generate_act_map(expected,seed,settings,independent,first);
        CHECK(good&&expected.valid());
        const Bytes oracle=snapshot(expected,first);
        std::memset(&foreground_workspace,0x9d,sizeof foreground_workspace);
        CHECK(generate_act_map(actual,seed,settings,second));CHECK(actual.valid());
        CHECK(snapshot(actual,second)==oracle);CHECK(!resource_transfer_uses);
        const Bytes retained=snapshot(actual,second);
        image_transfer(index);
        CHECK(snapshot(actual,second)==retained); // No pointer into the reused bytes.
        new_run(original,0x717331u+index,true,Character(index%CharacterKinds));CHECK(save(original,canonical));
        {ResourceTransferLease transfer(ResourceTransferUse::Save);CHECK(transfer);
            CHECK(save(original,*transfer.save()));CHECK(transfer.save()->size==canonical.size);
            CHECK(!std::memcmp(transfer.save()->bytes,canonical.bytes,canonical.size));
            CHECK(load(restored,transfer.save()->bytes,transfer.save()->size));map_rejected();
        }
        CHECK(save(restored,again));CHECK(again.size==canonical.size&&!std::memcmp(again.bytes,canonical.bytes,canonical.size));
        CHECK(generate_act_map(actual,seed,settings,second));CHECK(snapshot(actual,second)==oracle);
        CHECK(!resource_transfer_uses);
    }
}
int main(){
    CHECK(sizeof(ForegroundWorkspace)<sizeof(ResourceTransferWorkspace)+sizeof(MapWorkspace));
    lease_conflicts();alternating_maps_and_serialization();
    std::printf("foreground workspace: %u checks; %zu bytes replace %zu; saved %zu; dirty map/decode/save cycles and lease conflicts exact\n",
        checks,sizeof(ForegroundWorkspace),sizeof(ResourceTransferWorkspace)+sizeof(MapWorkspace),
        sizeof(ResourceTransferWorkspace)+sizeof(MapWorkspace)-sizeof(ForegroundWorkspace));
}
