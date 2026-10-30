#pragma once
#include "resource_file_cursor.hpp"
#include "resource_source_base.hpp"

namespace spire {

// Immutable resource files can outnumber the OS's simultaneously open streams.
// Length/header probes close immediately; source reads share only Capacity handles.
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
    // Optional observation of the complete probe, including open/close. Reads
    // and seeks describe actual host calls, not the later use of saved headers.
    struct Probe {
        void*context{};
        void(*begin)(void*){};
        void(*end)(void*,uint32_t bytes,unsigned reads,unsigned seeks,bool good){};
    };
    struct ReadStats {uint32_t calls=0,bytes=0,seeks=0;};
    using Metadata=bool(*)(void*,unsigned,uint32_t&);
    using HeaderCheck=bool(*)(const uint8_t*,uint32_t,unsigned);
    ResourceFileCache()=default;
    ResourceFileCache(const ResourceFileCache&)=delete;
    ResourceFileCache&operator=(const ResourceFileCache&)=delete;
    ~ResourceFileCache(){close();}
    bool open(Host host,uint32_t maximum,uint8_t*headers=nullptr,unsigned header_bytes=0,Probe probe={}){
        close();
        if(!host.open||!host.close||!host.seek||!host.tell||!host.get||!maximum||
           bool(headers)!=bool(header_bytes)||header_bytes>maximum||header_bytes>UINT32_MAX/Files||
           bool(probe.begin)!=bool(probe.end))return false;
        host_=host;
        for(unsigned i=0;i<Files;++i){
            if(probe.begin)probe.begin(probe.context);
            unsigned reads=0,seeks=0;uint32_t bytes=0;long size=-1;bool good=false;
            void*file=open_file(i);
            if(file){
                ++seeks;const bool sought=host_.seek(file,0,2)==0;
                size=sought?host_.tell(file):-1;
                good=size>0&&static_cast<unsigned long>(size)<=maximum&&header_bytes<=static_cast<unsigned long>(size);
                if(good&&header_bytes){
                    ++seeks;good=host_.seek(file,0,0)==0;
                    if(good){
                        bytes=header_bytes;++reads;
                        good=host_.get(headers+i*header_bytes,1,header_bytes,file)==header_bytes;
                    }
                }
                if(!close_file(file))good=false;
            }
            if(probe.end)probe.end(probe.context,bytes,reads,seeks,good);
            if(!good){close();return false;}
            parts_[i]={this,uint32_t(size),i,true};
        }
        ready_=true;return true;
    }
    // Immutable-file metadata proves presence and bounds without opening each
    // payload. Its 32-byte header is checked before that part yields any data.
    // This check survives LRU eviction and is reset by close/reinitialization.
    bool open_deferred(Host host,uint32_t maximum,Metadata metadata,HeaderCheck check,Probe probe={}){
        close();
        if(!host.open||!host.close||!host.seek||!host.get||!metadata||!check||maximum<32||
           bool(probe.begin)!=bool(probe.end))return false;
        host_=host;check_=check;
        for(unsigned i=0;i<Files;++i){
            if(probe.begin)probe.begin(probe.context);
            uint32_t size=0;poll();const bool found=metadata(host_.context,i,size);poll();
            const bool good=found&&size>=32&&size<=maximum;
            if(probe.end)probe.end(probe.context,0,0,0,good);
            if(!good){close();return false;}
            parts_[i]={this,size,i,false};
        }
        ready_=true;return true;
    }
    void close(){
        for(unsigned i=0;i<Capacity;++i)release(i);
        for(auto&part:parts_)part.length=0;
        ready_=false;host_={};check_=nullptr;
    }
    ResourceSource source(unsigned part,
        bool(*reader)(void*,uint32_t,void*,uint32_t)=read_at){
        return ready_&&part<Files?ResourceSource{&parts_[part],parts_[part].length,reader}:ResourceSource{};
    }
    // A host wrapper can account for the complete read, including cold opens,
    // without duplicating the cache's provider contexts or cursor ownership.
    static bool read_source(void*context,uint32_t at,void*out,uint32_t count,bool&did_seek,ReadStats*stats=nullptr){
        did_seek=false;if(stats)*stats={};if(!context)return false;
        auto&part=*static_cast<Part*>(context);
        return part.owner&&part.owner->read(part.index,at,out,count,did_seek,stats);
    }
    bool read(unsigned part,uint32_t at,void*out,uint32_t count,bool&did_seek,ReadStats*stats=nullptr){
        did_seek=false;if(stats)*stats={};
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
        if(!parts_[part].validated){
            uint8_t header[32];bool sought=false,read=false;
            const bool fetched=s.cursor.read(s.file,0,header,sizeof header,host_.seek,host_.get,sought,&read);
            did_seek|=sought;
            if(stats){stats->seeks+=sought;stats->calls+=read;stats->bytes+=read?sizeof header:0;}
            if(!fetched||!check_||!check_(header,parts_[part].length,part)){release(slot);return false;}
            parts_[part].validated=true;
        }
        bool sought=false,read=false;
        const bool good=s.cursor.read(s.file,at,out,count,host_.seek,host_.get,sought,&read);
        did_seek|=sought;
        if(stats){stats->seeks+=sought;stats->calls+=read;stats->bytes+=read?count:0;}
        if(!good){parts_[part].validated=!check_;release(slot);} // Retry checks a fresh failed stream.
        return good;
    }
private:
    struct Part {ResourceFileCache*owner{};uint32_t length{};unsigned index{};bool validated=false;};
    struct Slot {void*file{};ResourceFileCursor cursor{};unsigned part{},rank{};};
    Host host_{};
    HeaderCheck check_=nullptr;
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
