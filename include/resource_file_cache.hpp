#pragma once
#include "resource_file_cursor.hpp"
#include "resource_source_base.hpp"

namespace spire {

// Immutable resource files can outnumber the OS's simultaneously open streams.
// Length probes close immediately; source reads share only Capacity handles.
// The selected locale and every mutable save stream remain outside this cache.
template<unsigned Files,unsigned Capacity>
class ResourceFileCache {
    static_assert(Files&&Capacity&&Capacity<=Files,"A resource cache needs bounded nonempty slots");
public:
    struct Host {
        void*context{};
        void*(*open)(void*,unsigned){};
        int(*close)(void*){};
        int(*seek)(void*,long,int){};
        long(*tell)(void*){};
        unsigned(*get)(void*,unsigned,unsigned,void*){};
        // Sampling may collect input; it must never dispatch/re-enter App.
        void(*poll)(void*){};
    };
    ResourceFileCache()=default;
    ResourceFileCache(const ResourceFileCache&)=delete;
    ResourceFileCache&operator=(const ResourceFileCache&)=delete;
    ~ResourceFileCache(){close();}
    bool open(Host host,uint32_t maximum){
        close();
        if(!host.open||!host.close||!host.seek||!host.tell||!host.get||!maximum)return false;
        host_=host;
        for(unsigned i=0;i<Files;++i){
            void*file=open_file(i);if(!file){close();return false;}
            const bool sought=host_.seek(file,0,2)==0;
            const long size=sought?host_.tell(file):-1;
            const bool closed=close_file(file);
            if(!closed||size<=0||static_cast<unsigned long>(size)>maximum){close();return false;}
            parts_[i]={this,uint32_t(size),i};
        }
        ready_=true;return true;
    }
    void close(){
        for(unsigned i=0;i<Capacity;++i)release(i);
        for(auto&part:parts_)part.length=0;
        ready_=false;host_={};
    }
    ResourceSource source(unsigned part,
        bool(*reader)(void*,uint32_t,void*,uint32_t)=read_at){
        return ready_&&part<Files?ResourceSource{&parts_[part],parts_[part].length,reader}:ResourceSource{};
    }
    // A host wrapper can account for the complete read, including cold opens,
    // without duplicating the cache's provider contexts or cursor ownership.
    static bool read_source(void*context,uint32_t at,void*out,uint32_t count,bool&did_seek){
        did_seek=false;if(!context)return false;
        auto&part=*static_cast<Part*>(context);
        return part.owner&&part.owner->read(part.index,at,out,count,did_seek);
    }
    bool read(unsigned part,uint32_t at,void*out,uint32_t count,bool&did_seek){
        did_seek=false;
        if(!ready_||part>=Files||at>parts_[part].length||count>parts_[part].length-at||
           (count&&!out))return false;
        if(!count)return true;
        unsigned slot=Capacity;
        for(unsigned i=0;i<Capacity;++i)if(slots_[i].file&&slots_[i].part==part){slot=i;break;}
        if(slot==Capacity){
            for(unsigned i=0;i<Capacity;++i)if(!slots_[i].file){slot=i;break;}
            if(slot==Capacity){
                slot=0;for(unsigned i=1;i<Capacity;++i)if(slots_[i].rank>slots_[slot].rank)slot=i;
                // Evict before opening: never transiently exceed Capacity.
                if(!release(slot))return false;
            }
            auto&s=slots_[slot];s.file=open_file(part);
            if(!s.file)return false; // A subsequent request retries normally.
            s.part=part;s.rank=0;for(unsigned i=0;i<Capacity;++i)if(i!=slot&&slots_[i].file)++s.rank;
            s.cursor={}; // Even a reused FILE address starts at an unknown position.
        }
        touch(slot);auto&s=slots_[slot];
        const bool good=s.cursor.read(s.file,at,out,count,host_.seek,host_.get,did_seek);
        if(!good)release(slot); // Do not retain a failed/poisoned stream.
        return good;
    }
private:
    struct Part {ResourceFileCache*owner{};uint32_t length{};unsigned index{};};
    struct Slot {void*file{};ResourceFileCursor cursor{};unsigned part{},rank{};};
    Host host_{};
    Part parts_[Files]{};
    Slot slots_[Capacity]{};
    bool ready_=false;
    void poll(){if(host_.poll)host_.poll(host_.context);}
    void*open_file(unsigned part){poll();void*file=host_.open(host_.context,part);poll();return file;}
    bool close_file(void*file){poll();const bool good=host_.close(file)==0;poll();return good;}
    bool release(unsigned slot){
        auto&s=slots_[slot];
        bool good=true;
        if(s.file){
            good=close_file(s.file);
            for(unsigned i=0;i<Capacity;++i)if(slots_[i].file&&slots_[i].rank>s.rank)--slots_[i].rank;
        }
        s={};return good;
    }
    void touch(unsigned slot){
        const unsigned rank=slots_[slot].rank;
        for(unsigned i=0;i<Capacity;++i)if(i!=slot&&slots_[i].file&&slots_[i].rank<rank)++slots_[i].rank;
        slots_[slot].rank=0;
    }
    static bool read_at(void*context,uint32_t at,void*out,uint32_t count){
        bool did_seek=false;return read_source(context,at,out,count,did_seek);
    }
};
}
