// The Chinese card-page working set exceeds 64 glyphs and eight strings, but
// their actual payloads occupy a small fraction of the former fixed slots.
// The full Chinese history page visits 1,249 glyph keys (62,257 bytes) and
// 932 string keys (10,452 bytes). Retain exact bytes in 64-byte runs and give
// the existing byte budget more keys instead of evicting these tiny records.
// Entries never move, so a view
// remains valid until that entry is evicted or its Content owner is reopened.
// The key index hashes only the requested offset/length; no payload is hashed.
template<unsigned ByteCount,unsigned Entries> class ContentTextCache {
    static constexpr unsigned PageBytes=64,Pages=ByteCount/PageBytes,Buckets=256;
    struct Entry {
        const Content* owner=nullptr;
        uint32_t offset=0,age=0;
        uint16_t length=0,first=0,next=0,pages=0;
    };
    uint8_t bytes_[ByteCount]{};
    Entry entries_[Entries]{};
    uint32_t used_[(Pages+31)/32]{};
    uint16_t buckets_[Buckets]{};
    uint32_t age_=0;
    unsigned next_entry_=0;
    static_assert(ByteCount%PageBytes==0&&Pages<=65535&&Entries<=65535,
                  "Text cache runs and keys must fit their bounded indexes");
    static unsigned bucket(uint32_t offset,unsigned length){
        return ((offset>>2)^(offset>>9)^(length<<3))&(Buckets-1);
    }
    uint32_t next_age(){
        if(age_==0xffffffffu){
            for(auto&entry:entries_)if(entry.owner)entry.age=(entry.age>>1)+1;
            age_=0x80000000u;
        }
        return ++age_;
    }
    void discard(unsigned id){
        auto&entry=entries_[id];
        if(entry.owner){
            auto*link=&buckets_[bucket(entry.offset,entry.length)];
            while(*link&&*link!=id+1)link=&entries_[*link-1].next;
            if(*link)*link=entry.next;
        }
        for(unsigned i=entry.first;i<unsigned(entry.first)+entry.pages;++i)
            used_[i/32]&=~(uint32_t(1)<<(i%32));
        entry={};
    }
    unsigned oldest()const{
        unsigned result=Entries;
        for(unsigned i=0;i<Entries;++i)if(entries_[i].owner&&
            (result==Entries||entries_[i].age<entries_[result].age))result=i;
        return result;
    }
public:
    using Loader=bool(*)(const void*,uint32_t,void*,unsigned);
    void clear(const Content* owner){
        for(unsigned i=0;i<Entries;++i)if(entries_[i].owner==owner)discard(i);
    }
    const uint8_t* cached(const Content* owner,uint32_t offset,unsigned length){
        if(!length||length>65535||length>ByteCount)return nullptr;
        const unsigned key=bucket(offset,length);
        for(unsigned link=buckets_[key];link;link=entries_[link-1].next){
            auto&entry=entries_[link-1];
            if(entry.owner==owner&&entry.offset==offset&&entry.length==length){
                entry.age=next_age();return bytes_+unsigned(entry.first)*PageBytes;
            }
        }
        return nullptr;
    }
    const uint8_t* read_with(const Content* owner,uint32_t offset,unsigned length,
                            const void*context,Loader load){
        if(!length||length>65535||length>ByteCount||!load)return nullptr;
        if(const auto*hit=cached(owner,offset,length))return hit;
        const unsigned key=bucket(offset,length);
        unsigned slot=Entries;
        for(unsigned i=0;i<Entries;++i){
            const unsigned at=(next_entry_+i)%Entries;
            if(!entries_[at].pages){slot=at;break;}
        }
        if(slot==Entries){slot=oldest();if(slot==Entries)return nullptr;discard(slot);}
        next_entry_=(slot+1)%Entries;
        const unsigned pages=(length+PageBytes-1)/PageBytes;
        for(unsigned attempt=0;attempt<=Entries;++attempt){
            unsigned run=0,first=Pages;
            for(unsigned i=0;i<Pages;++i){
                run=(used_[i/32]&(uint32_t(1)<<(i%32)))?0:run+1;
                if(run==pages){first=i+1-pages;break;}
            }
            if(first<Pages){
                auto&entry=entries_[slot];entry.first=uint16_t(first);entry.pages=uint16_t(pages);
                for(unsigned i=first;i<first+pages;++i)used_[i/32]|=uint32_t(1)<<(i%32);
                auto*output=bytes_+first*PageBytes;
                if(!load(context,offset,output,length)){discard(slot);return nullptr;}
                entry.owner=owner;entry.offset=offset;entry.length=uint16_t(length);entry.age=next_age();
                entry.next=buckets_[key];buckets_[key]=uint16_t(slot+1);return output;
            }
            const unsigned victim=oldest();if(victim==Entries)return nullptr;discard(victim);
        }
        return nullptr;
    }
    const uint8_t* read(const Content* owner,uint32_t offset,unsigned length,const ResourceReader&reader){
        return read_with(owner,offset,length,&reader,[](const void*context,uint32_t at,void*out,unsigned size){
            return static_cast<const ResourceReader*>(context)->read(at,out,size);
        });
    }
};
using GlyphByteCache=ContentTextCache<163u*1024u,1536>;
using StringByteCache=ContentTextCache<102u*1024u,1024>;
// Include all cache metadata in the old 64*3200 / 8*16384 payload budgets.
// These assertions hold on both 32-bit calculator and 64-bit native hosts.
static_assert(sizeof(GlyphByteCache)<=64u*(MaxGlyphBytes+sizeof(const Content*)+sizeof(uint32_t)),
              "Glyph byte cache must not exceed the previous fixed slots");
static_assert(sizeof(StringByteCache)<=8u*(MaxStringBytes+sizeof(const Content*)+2*sizeof(uint32_t)),
              "String byte cache must not exceed the previous fixed slots");
static GlyphByteCache glyph_cache;
static StringByteCache string_cache;
