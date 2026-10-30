#include "app.hpp"
#include "lz4_block.hpp"
#include "lzma2_block.hpp"
#include "inline_icons.hpp"
#include "unicode_text.hpp"
#include "resource_transfer.hpp"
#include "tile_filter.hpp"
#include "doom_assets.hpp"
#include "vantom_ui15_resources.hpp"
#include "vantom_vfx_ui15.hpp"
extern "C" void* memcpy(void*,const void*,size_t);
namespace spire {
static uint16_t u16(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t u32(const uint8_t*p){return u16(p)|(uint32_t)u16(p+2)<<16;}
namespace {
// A view lives until its entry is evicted or its owner is reopened/failed.
// Entries never move: unrelated allocations and group-cache reuse preserve
// every surviving view. All sizes and owners share the bounded image pool;
// callers must finish using a view before unbounded further image requests.
constexpr unsigned MaxImageBytes=320u*240u*8u+2u*240u;
constexpr unsigned MaxEncodedImage=MaxImageBytes+MaxImageBytes/255u+32u;
constexpr unsigned MaxGlyphBytes=40u*40u*2u,MaxStringBytes=16384;
// Keep the total foreground allocation below 24 MiB. Two additional MiB
// retain complete multi-enemy/card animation working sets after warmup.
// Ordinary scenes use6.75 MiB of decoded images and6 MiB of tile groups;
// merchant scenes use11.75 MiB of decoded images and a1 MiB decoder tier.
// The256 KiB reduction covers the bounded exact random-water lookup without
// increasing the allocation required from the ordinary OS's contiguous heap.
// The split changes only at a frame boundary. Picture addresses below the
// new boundary remain valid; the relocated tile tier is always discarded.
constexpr unsigned ImagePageBytes=4096,ImagePages=1728,RoomImagePages=3008,ImageEntries=1151;
constexpr unsigned PictureBufferBudget=13u*1024u*1024u-256u*1024u;
alignas(4) static uint8_t picture_pixels[PictureBufferBudget];
static uint8_t*const image_pixels=picture_pixels;
static unsigned image_page_limit=ImagePages;
static_assert(MaxEncodedImage==ResourceTransferCapacity,"Shared transfer capacity must preserve the original decoding bound");
static void cancel_content_preload(const Content*owner=nullptr);
#include "content_tiles.inc"
// Keep a directory index in the existing key word. The immutable, resident
// descriptor supplies offset, geometry, stored format and encoded length
// without copying the descriptor or adding range reads during lookup.
struct ImageCache {
    const Content* owner{};uint32_t image{},age{};
    uint16_t first{};uint8_t pages{},packed_format{};
    // Low16 bits retain the image id, followed by ten chain-link bits and
    // the six-bit bucket. The format's unused high bit holds link bit10,
    // retaining64 buckets and the same16-byte ARM entry. The complete closed
    // merchant, unfocused opening clip and stock need about1050 keys, while
    // their decoded bytes already fit the existing image arena.
    unsigned id()const{return image&65535u;}
    unsigned next()const{return ((image>>16)&1023u)|(unsigned(packed_format&128u)<<3);}
    unsigned bucket()const{return image>>26;}
    unsigned format()const{return packed_format&7u;}
    void link(unsigned next){image=(image&~(1023u<<16))|((next&1023u)<<16);
        packed_format=uint8_t((packed_format&127u)|((next>>3)&128u));}
};
static ImageCache image_cache[ImageEntries];
constexpr unsigned ImageBuckets=64;
static uint16_t image_buckets[ImageBuckets]{};
static uint32_t image_used[(RoomImagePages+31)/32]{},image_age=0;
static_assert(ImagePageBytes==Content::image_page_bytes()&&ImagePages==Content::image_capacity_pages()&&
              ImageEntries==Content::image_capacity_entries(),"Public preparation budget must match the cache");
static_assert(RoomImagePages<65536&&(MaxImageBytes+ImagePageBytes-1)/ImagePageBytes<=255,
              "Image page indexes and lengths must fit the bounded entry");
static_assert(ImageEntries<2048&&ImageBuckets==64,"Image ids and links must fit their packed key/format fields");
static_assert(sizeof picture_pixels==13u*1024u*1024u-256u*1024u,
              "Picture tiers reserve256 KiB for the exact water lookup");
static_assert(sizeof image_cache+sizeof image_buckets+sizeof image_used+sizeof image_age<=28u*1024u,
              "Image-cache bookkeeping must remain below28 KiB on the host");
static uint8_t* image_buffer(const ImageCache& cache){return image_pixels+unsigned(cache.first)*ImagePageBytes;}
static unsigned image_bucket(const ImageDescriptor& descriptor){
    return ((descriptor.offset>>1)^(descriptor.offset>>9)^(descriptor.shape>>5)^descriptor.shape^descriptor.length)&(ImageBuckets-1);
}
static ImageCache* find_image(const Content* owner,ImageDirectoryView directory,unsigned id){
    const auto descriptor=directory[id];
    for(unsigned link=image_buckets[image_bucket(descriptor)];link;){
        auto& cache=image_cache[link-1];link=cache.next();if(cache.owner!=owner)continue;
        if(directory.equal(cache.id(),id))return &cache;
    }
    return nullptr;
}
static unsigned image_page_bound(const ImageDescriptor& descriptor){
    const unsigned format=descriptor.format,w=descriptor.width(),h=descriptor.height();
    if(!w||w>320||!h||h>240||format>7||descriptor.length>MaxEncodedImage)return 0;
    const unsigned pixels=w*h,spans=pixels*8u+2u*h,palette=512u+2u*pixels;
    const unsigned bytes=format<5?descriptor.length:spans>palette?spans:palette;
    return (bytes+ImagePageBytes-1)/ImagePageBytes;
}
static void discard_image(ImageCache& cache){
    if(cache.owner){
        unsigned previous=0,link=image_buckets[cache.bucket()],id=unsigned(&cache-image_cache)+1;
        while(link&&link!=id){previous=link;link=image_cache[link-1].next();}
        if(link){if(previous)image_cache[previous-1].link(cache.next());else image_buckets[cache.bucket()]=uint16_t(cache.next());}
    }
    for(unsigned i=cache.first;i<unsigned(cache.first)+cache.pages;++i)image_used[i/32]&=~(uint32_t(1)<<(i%32));
    cache={};
}
static uint32_t next_image_age(){
    if(image_age==0xffffffffu){
        // Preserve LRU order across the bounded counter's wrap. Empty and
        // unpublished reservations have no age and cannot become victims.
        uint16_t order[ImageEntries];unsigned count=0;
        for(unsigned i=0;i<ImageEntries;++i)if(image_cache[i].owner){
            unsigned at=count;
            while(at&&image_cache[order[at-1]].age>image_cache[i].age){order[at]=order[at-1];--at;}
            order[at]=uint16_t(i);++count;
        }
        for(unsigned i=0;i<count;++i)image_cache[order[i]].age=i+1;
        image_age=count;
    }
    return ++image_age;
}
static ImageCache* reserve_image(unsigned size){
    if(!size||size>MaxImageBytes)return nullptr;
    const unsigned pages=(size+ImagePageBytes-1)/ImagePageBytes;
    // At most all published entries can be evicted. No compaction or
    // allocation moves another entry while a caller still holds its view.
    for(unsigned attempt=0;attempt<=ImageEntries;++attempt){
        unsigned run=0,first=image_page_limit;
        for(unsigned i=0;i<image_page_limit;++i){
            run=(image_used[i/32]&(uint32_t(1)<<(i%32)))?0:run+1;
            if(run==pages){first=i+1-pages;break;}
        }
        if(first<image_page_limit)for(auto& cache:image_cache)if(!cache.pages){
            cache.first=uint16_t(first);cache.pages=uint8_t(pages);
            for(unsigned i=first;i<first+pages;++i)image_used[i/32]|=uint32_t(1)<<(i%32);
            return &cache;
        }
        ImageCache* victim=nullptr;
        for(auto& cache:image_cache)if(cache.owner&&(!victim||cache.age<victim->age))victim=&cache;
        if(!victim)return nullptr;
        discard_image(*victim);
    }
    return nullptr;
}
struct ImageReservation {
    ImageCache* cache;
    explicit ImageReservation(unsigned size):cache(reserve_image(size)){}
    ImageReservation(const ImageReservation&)=delete;
    ImageReservation& operator=(const ImageReservation&)=delete;
    ~ImageReservation(){if(cache&&!cache->owner)discard_image(*cache);}
    uint8_t* bytes()const{return cache?image_buffer(*cache):nullptr;}
    unsigned publish(const Content* owner,ImageDirectoryView directory,uint32_t image,unsigned format,bool alpha_checked=false){
        const auto descriptor=directory[image];
        if(!alpha_checked&&(format==1||format==3)){
            // Several full-screen originals store an entirely opaque alpha
            // plane. Prove that once when publishing the decoded image, not
            // in every draw, and use the equivalent opaque sampler thereafter.
            const unsigned count=descriptor.width()*descriptor.height();
            const uint8_t*alpha=bytes()+(format==1?count*2:512+count);
            unsigned i=0;while(i<count&&alpha[i]==255)++i;
            if(i==count)--format;
        }
        const unsigned bucket=image_bucket(descriptor);
        cache->image=image|(bucket<<26);cache->packed_format=uint8_t(format);
        cache->link(image_buckets[bucket]);cache->age=next_image_age();cache->owner=owner;
        image_buckets[bucket]=uint16_t(cache-image_cache+1);return format;
    }
};
#include "content_text_cache.inc"
// Font layout and eight-pass outlines repeatedly ask for the same immutable
// directory records before requesting their alpha bytes. Keep a small exact-key
// memo beside the existing byte cache, rather than repeat a catalog binary
// search for each measurement/pass. Entries contain only directory indexes;
// payload views, decoding, fallback order and provider reads stay unchanged.
struct GlyphRecordCache {
    static constexpr unsigned Count=256;
    const Content* owner=nullptr;
    uint32_t keys[Count]{},indexes[Count]{};
    static unsigned bucket(uint32_t key){return (key^(key>>9)^(key>>17)^(key>>24))&(Count-1);}
    void use(const Content* current){
        if(owner==current)return;
        for(auto&key:keys)key=0;
        owner=current;
    }
    bool lookup(const Content* current,uint32_t key,const uint8_t*base,const uint8_t*&result){
        use(current);const unsigned slot=bucket(key);
        if(keys[slot]!=key)return false;
        result=indexes[slot]?base+(indexes[slot]-1)*16:nullptr;return true;
    }
    const uint8_t* remember(uint32_t key,const uint8_t*base,const uint8_t*result){
        const unsigned slot=bucket(key);keys[slot]=key;
        indexes[slot]=result?uint32_t((result-base)/16)+1:0;return result;
    }
    void clear(const Content*current){if(owner==current)owner=nullptr;}
};
static GlyphRecordCache glyph_records;
static_assert(sizeof glyph_records<=2048u+sizeof(const Content*),
              "Glyph record memo must retain its 2 KiB directory-index budget");

#include "content_grouped_glyphs.inc"
#include "content_grouped_strings.inc"
static void clear_caches(const Content* owner){
    cancel_content_preload(owner);
    clear_tile_cache(owner);
    for(auto&v:image_cache)if(v.owner==owner)discard_image(v);
    glyph_cache.clear(owner);string_cache.clear(owner);glyph_records.clear(owner);
}
bool valid_image(const uint8_t* p,unsigned len,unsigned w,unsigned h,unsigned fmt){
    if(fmt<4){unsigned expected=fmt<2?w*h*(fmt?3:2):512+w*h*(fmt==3?2:1);return len==expected;}
    unsigned at=0;
    for(unsigned y=0;y<h;y++){
        if(len-at<2)return false;unsigned spans=u16(p+at);at+=2;if(spans>w)return false;unsigned last=0;
        for(unsigned j=0;j<spans;j++){
            if(len-at<5)return false;unsigned x=u16(p+at),count=u16(p+at+2),mode=p[at+4];at+=5;
            if(!count||mode>1||x<last||x+count>w||count*(mode?3u:2u)>len-at)return false;
            last=x+count;at+=count*(mode?3u:2u);
        }
    }
    return at==len;
}
#include "content_preload.inc"
static bool compact_directory(const ResourceReader&reader,uint8_t*output,unsigned capacity,
        unsigned input_size,unsigned images){
    const unsigned image_end=40+images*16,reduction=images*4;
    if(input_size<image_end||input_size-reduction>capacity)return false;
    // Match the NSL2 decoder's natural 64 KiB boundaries. Starting each read
    // at image offset 40 would decode every boundary block twice. A sixteen-
    // byte carry holds the one descriptor crossing a boundary instead.
    ForegroundScratch<uint8_t,65536> scratch;if(!scratch)return false;
    uint8_t record[16];unsigned partial=0,rows=0;
    for(unsigned offset=0;offset<input_size;){
        const unsigned count=input_size-offset<65536?input_size-offset:65536;
        auto*chunk=scratch.array();if(!reader.read(offset,chunk,count))return false;
        unsigned at=0;
        while(at<count){
            const unsigned absolute=offset+at;
            if(absolute<40){
                const unsigned n=count-at<40-absolute?count-at:40-absolute;
                memcpy(output+absolute,chunk+at,n);at+=n;
            }else if(absolute<image_end){
                const unsigned n=count-at<16-partial?count-at:16-partial;
                memcpy(record+partial,chunk+at,n);partial+=n;at+=n;
                if(partial==16){ImageDirectoryView::compact(output+40+rows*12,record);++rows;partial=0;}
            }else{
                memcpy(output+absolute-reduction,chunk+at,count-at);at=count;
            }
        }
        offset+=count;
    }
    return !partial&&rows==images;
}
}
const uint8_t* Content::payload(uint32_t offset,uint32_t count,uint8_t* scratch,uint32_t capacity)const{
    if(!io_good_||!reader_.contains(offset,count)){io_good_=false;return nullptr;}
    if(data_)return data_+offset;
    if(!scratch||count>capacity||!reader_.read(offset,scratch,count)){io_good_=false;return nullptr;}
    return scratch;
}
bool Content::open(const uint8_t*p,size_t n){return open(ResourceSource::memory(p,n));}
bool Content::open(ResourceSource source){
    ++generation_;
    data_=metadata_=tile_metadata_=nullptr;directory_bytes_=0;image_pages_=nullptr;
    image_directory_={};directory_reduction_=0;
    version_=cards_=0;ui_offset_=ui_count_=0;language_id_=2;io_good_=true;clear_caches(this);
    if(!reader_.reset(source)||source.length<40||source.length>MaxContentFile)return false;
    reader_.progress(ResourceStage::Header,0,1);
    uint8_t h[40];if(!reader_.read(0,h,sizeof h))return false;const uint32_t n=source.length;
    if(u32(h)!=0x3250534E||u32(h+8)!=0x41CEF1EA||u32(h+12)!=n)return false;
    const uint32_t version=u32(h+4),cards=u32(h+32);
    if(version<3||version>MaximumContentVersion||(version==3&&cards)||(version==4&&cards!=24)||(version==5&&cards!=26)||(version==6&&cards!=51)||(version==7&&cards!=58)||(version>=8&&version<=10&&cards!=68)||(version==11&&cards!=114)||(version==12&&cards!=228)||(version==13&&cards!=497)||(version>=14&&cards!=CardKinds))return false;
    auto ni=u32(h+16),ng=u32(h+20),ns=u32(h+24),nc=u32(h+28);
    if(ni<A_Count||ni>(version>=14?65535u:8192u)||ng>(version>=14?scene_font_glyph_limit(version):version==13?16384u:8192u)||ns<66||ns>(version>=14?16384u:2048u)||nc!=(version>=BestiaryKaiserContentVersion?BestiaryKaiserAnimationKinds:version>=CharacterDeathContentVersion?CharacterDeathAnimationKinds:version>=PhobiaContentVersion?PhobiaAnimationKinds:version>=Ui7ContentVersion?Ui7AnimationKinds:version>=MerchantEffectsContentVersion?MerchantEffectsAnimationKinds:version==FeedbackContentVersion?FeedbackAnimationKinds:version>=14?AnimationKinds:version==13?38:version>=6?17:11))return false;
    if((version==11&&ns!=407)||(version==12&&ns!=749)||(version==13&&ns!=1556)||
       (version>=14&&version<LocaleContentVersion&&ns!=ContentStrings&&!(version==NativeUiContentVersion&&ns==NativeUiStringCount))||
       (version>=LocaleContentVersion&&ns!=(version>=Ui7ContentVersion?Ui7StringCount:LocaleStringCount)))return false;
    unsigned auxiliary=version>=GameOverTransitionContentVersion?unsigned(GameOverTransitionAuxiliaryCount):version>=HistoryJournalContentVersion?unsigned(HistoryJournalAuxiliaryCount):version>=CharacterDeathContentVersion?unsigned(CharacterDeathAuxiliaryCount):version>=HistoryUi8ContentVersion?unsigned(HistoryUi8AuxiliaryCount):version>=HudUi7ContentVersion?unsigned(HudUi7AuxiliaryCount):version>=ProfileContentVersion?unsigned(ProfileAuxiliaryCount):version>=PhobiaContentVersion?unsigned(PhobiaAuxiliaryCount):version>=Ui7ContentVersion?unsigned(Ui7AuxiliaryCount):version>=Ui6ContentVersion?unsigned(Ui6AuxiliaryCount):version>=FrontendContentVersion?unsigned(FrontendAuxiliaryCount):version>=TitleEffectsContentVersion?unsigned(TitleEffectsAuxiliaryCount):version>=MerchantEffectsContentVersion?unsigned(MerchantEffectsAuxiliaryCount):version==FeedbackContentVersion?unsigned(FeedbackAuxiliaryCount):version==LocaleContentVersion?unsigned(LocaleAuxiliaryCount):version==NativeUiContentVersion?unsigned(NativeUiAuxiliaryCount):version>=14?unsigned(U_Count):version==13?339:version==12?233:version==11?185:version==10?105:version==9?95:version==8?90:version==7?83:version==6?74:version==5?56:0;
    uint32_t base=40+ni*16+ng*16+ns*8+nc*16+cards*16+auxiliary*4;
    if(version>=CardResourceContentVersion){
        uint32_t next=0;if(!reader_.u32(base,next))return false;
        if(next!=0x314c4954u){
            if(next>=ni||!reader_.u32(base+4,next))return false;
            if(next==0x314c4954u){auxiliary=CardResourceAuxiliaryCount;base+=4;}
            else{
                // The optional tutorial catalog follows the card overlay.
                // Probe only its six remaining image IDs and TIL1, never
                // scan arbitrary payload or decode a startup image/string.
                if(version<FtueContentVersion||next>=ni)return false;
                uint8_t suffix[28];if(!reader_.read(base+8,suffix,sizeof suffix))return false;
                for(unsigned i=0;i<6;++i)if(u32(suffix+i*4)>=ni)return false;
                auxiliary=FtueAuxiliaryCount;base+=32;
                if(u32(suffix+24)!=0x314c4954u){
                    // The optional preview-arrow/SDF pair is one exact
                    // extension, not an unbounded search for a tile header.
                    if(u32(suffix+24)>=ni||!reader_.u32(base+4,next)||next>=ni)return false;
                    if(!reader_.u32(base+8,next))return false;
                    auxiliary=SelectionAuxiliaryCount;base+=8;
                    if(next!=0x314c4954u){
                        if(next>=ni||!reader_.u32(base+4,next))return false;
                        auxiliary=InspectionAuxiliaryCount;base+=4;
                        if(next!=0x314c4954u){
                            // v37 adds speech plus five source-sized popups.
                            // Earlier catalogs still stop at their own TIL1.
                            if(version<SpeechBubbleContentVersion||next>=ni)return false;
                            uint8_t room_suffix[24];if(!reader_.read(base+4,room_suffix,sizeof room_suffix))return false;
                            for(unsigned i=0;i<5;++i)if(u32(room_suffix+i*4)>=ni)return false;
                            auxiliary=SpeechBubbleAuxiliaryCount;base+=24;
                            if(u32(room_suffix+20)!=0x314c4954u){
                                // One optional source History reminder icon.
                                // This remains a bounded one-ID probe.
                                if(version<HistoryHeaderContentVersion||u32(room_suffix+20)>=ni||
                                   !reader_.u32(base+4,next))return false;
                                auxiliary=HistoryHeaderAuxiliaryCount;base+=4;
                                if(next!=0x314c4954u){
                                    if(version<HistoryBodyContentVersion)return false;
                                    // v39 appends a fixed History/Settings image catalog.
                                    // Do not scan an unbounded payload for a TIL1 marker.
                                    for(unsigned i=HistoryHeaderAuxiliaryCount;i<SettingsUi10AuxiliaryCount;++i){
                                        if(next>=ni||!reader_.u32(base+4,next))return false;base+=4;
                                    }
                                    if(version>=Ui12ContentVersion){
                                        for(unsigned i=SettingsUi10AuxiliaryCount;i<Ui12AuxiliaryCount;++i){
                                            if(next>=ni||!reader_.u32(base+4,next))return false;base+=4;
                                        }
                                    }
                                    auxiliary=version>=Ui12ContentVersion?Ui12AuxiliaryCount:SettingsUi10AuxiliaryCount;
                                    if(version>=Ui12ContentVersion&&next!=0x314c4954u){
                                        // The first-launch logo is exactly37 optional IDs.
                                        // Older UI40 directories end before this catalog.
                                        for(unsigned i=0;i<IntroLogoFrames;++i){
                                            if(next>=ni||!reader_.u32(base+4,next))return false;base+=4;
                                        }
                                        auxiliary=IntroLogoAuxiliaryCount;
                                        // One optional original Entomancer particle. Keep
                                        // the old2161/2198 endpoints and reject any further IDs.
                                        if(next!=0x314c4954u){
                                            if(next>=ni||!reader_.u32(base+4,next))return false;base+=4;
                                            auxiliary=PhobiaSwarmAuxiliaryCount;
                                            // Two localized sleeping particles form one
                                            // complete append; a lone ID is not a valid catalog.
                                            if(next!=0x314c4954u){
                                                for(unsigned i=0;i<2;++i){
                                                    if(next>=ni||!reader_.u32(base+4,next))return false;base+=4;
                                                }
                                                auxiliary=SleepingVfxAuxiliaryCount;
                                                // UI13 adds complete source-art catalogs. Each
                                                // known endpoint may end at TIL1; incomplete
                                                // catalogs fail without scanning payload data.
                                                constexpr unsigned endpoints[]={FrontendUi13AuxiliaryCount,
                                                    RoomsUi13AuxiliaryCount,ArchitectUi13AuxiliaryCount,
                                                    ArchitectAttackUi13AuxiliaryCount,DoomUi13AuxiliaryCount,
                                                    RandomCharacterUi13AuxiliaryCount,FakeMerchantBaseUi13AuxiliaryCount,
                                                    FakeMerchantUi13AuxiliaryCount,CombatUi14AuxiliaryCount,
                                                    MerchantUi14AuxiliaryCount,RoomsUi15AuxiliaryCount,
                                                    CombatUi15AuxiliaryCount,VantomUi15AuxiliaryCount,
                                                    vantom_vfx::AuxiliaryCount};
                                                for(unsigned end:endpoints){
                                                    if(next==0x314c4954u)break;
                                                    for(unsigned i=auxiliary;i<end;++i){
                                                        if(next>=ni||!reader_.u32(base+4,next))return false;base+=4;
                                                    }
                                                    auxiliary=end;
                                                }
                                            }
                                        }
                                    }
                                    if(next!=0x314c4954u)return false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(base>n||base>DirectoryInputCapacity)return false;
    reader_.progress(ResourceStage::Header,1,1);
    uint32_t start=base;
    if(version>=TileContentVersion){
        uint8_t header[64];
        if(!reader_.read(base,header,sizeof header)||
           !tile_header_bounds(header,base,n,ni,DirectoryInputCapacity,version>=RetainedContentVersion,start))return false;
    }
    // NSL2 only compresses metadata that stays in this existing directory
    // cache. Frame-descriptor reads may already hold an image-transfer lease.
    if(source.required_cached_prefix>start)return false;
    reader_.progress(ResourceStage::Directory,0,start);
    if(source.direct){data_=source.direct;metadata_=source.direct;}
    else {
        if(!compact_directory(reader_,directory_,DirectoryCapacity,start,ni))return false;
        metadata_=directory_;directory_reduction_=ni*4;
    }
    reader_.progress(ResourceStage::Directory,start,start);
    const uint8_t*p=metadata_;length_=n;images_=ni;glyphs_=ng;strings_=ns;clips_=nc;version_=version;
    image_directory_={p+40,data_?16u:12u};
    directory_bytes_=start-directory_reduction_;tile_metadata_=version>=TileContentVersion?p+(base-directory_reduction_):nullptr;
    if(directory_reduction_&&tile_metadata_){
        // These two words address resident tables, not virtual payload bytes.
        // Descriptor and group payload offsets remain their original values.
        // The virtual base can exceed the smaller resident array. Subtract
        // as an integer before forming a pointer into the owned array.
        auto*tile=directory_+(base-directory_reduction_);
        ImageDirectoryView::word(tile+36,u32(tile+36)-directory_reduction_);
        if(version>=RetainedContentVersion)ImageDirectoryView::word(tile+56,u32(tile+56)-directory_reduction_);
    }
    auto reject=[&](){data_=metadata_=tile_metadata_=nullptr;directory_bytes_=0;image_pages_=nullptr;
        image_directory_={};directory_reduction_=0;version_=cards_=ui_offset_=ui_count_=0;language_id_=2;clear_caches(this);return false;};
    // Opening needs only the directory. Pixel groups, glyphs and strings are
    // read on demand; legacy checksums and unrequested records are ignored.
    const unsigned available=data_?DirectoryCapacity:DirectoryCapacity-directory_bytes_;
    if(!data_&&version>=RetainedContentVersion){
        // The prepared costs already occupy owned, mutable directory storage.
        // Learning an exact decoded cost can update that row in place instead
        // of reserving another 58,060 bytes for the current artwork catalog.
        image_pages_=directory_+u32(tile_metadata_+56);
    }else if(images_<=available){
        image_pages_=directory_+DirectoryCapacity-images_;
        const uint8_t* costs=version>=RetainedContentVersion?p+u32(tile_metadata_+56):nullptr;
        for(unsigned i=0;i<images_;++i)image_pages_[i]=costs?costs[i]:0;
    }
    if(version>=LocaleContentVersion){
        const uint8_t* row=p+image_directory_end()+ng*16+LocaleMarkerString*8;uint8_t marker[4];
        if(u32(row+4)!=3||!reader_.read(u32(row),marker,4)||marker[3]||
           !locale_language_id(reinterpret_cast<const char*>(marker)))return reject();
        language_id_=uint8_t(locale_language_id(reinterpret_cast<const char*>(marker)));
    }
    if(version>=5){
        ui_count_=auxiliary;ui_offset_=image_directory_end()+ng*16+ns*8+nc*16+cards*16;
    }
    cards_=version>=4?cards:14;reader_.progress(ResourceStage::Ready,1,1);return true;
}
bool Content::image_cached(int id)const{
    if(!healthy()||id<0||unsigned(id)>=images_)return false;
    const auto descriptor=image_directory_[unsigned(id)];
    return (data_&&descriptor.format<5)||find_image(this,image_directory_,unsigned(id));
}
void Content::set_image_cache_profile(bool room_frames)const{
    const unsigned pages=room_frames?RoomImagePages:ImagePages;
    if(image_page_limit==pages)return;
    cancel_content_preload();
    // App calls this before requesting frame pictures. Only the tile tier
    // moves: a larger image tier can keep every published image at its exact
    // address. On shrink, discard just the entries crossing its new bound.
    // Clearing the unaffected title/hud pictures here caused cold reloads
    // during Continue's800ms wipe and wasted its bounded stock preparation.
    for(auto&group:tile_groups)tile_discard(group);
    for(auto&entry:image_cache)if(unsigned(entry.first)+entry.pages>pages)discard_image(entry);
    tile_age=0;
    image_page_limit=pages;
    tile_pages=picture_pixels+pages*ImagePageBytes;
    tile_page_limit=(PictureBufferBudget-pages*ImagePageBytes)/TilePageBytes;
}
bool Content::image_alias(int first,int second)const{
    if(!healthy()||first<0||second<0||unsigned(first)>=images_||unsigned(second)>=images_)return false;
    return image_directory_.equal(unsigned(first),unsigned(second));
}
unsigned Content::image_cache_pages(int id)const{
    if(!healthy()||id<0||unsigned(id)>=images_)return 0;
    const auto descriptor=image_directory_[unsigned(id)];
    if(data_&&descriptor.format<5)return 0;
    if(image_pages_&&image_pages_[id])return image_pages_[id];
    if(const auto* cache=find_image(this,image_directory_,unsigned(id)))return cache->pages;
    if(version_>=RetainedContentVersion){
        const unsigned pages=metadata_[u32(tile_metadata_+56)+unsigned(id)];if(pages)return pages;
    }
    return image_page_bound(descriptor);
}
ImageCacheUsage Content::image_cache_usage()const{
    ImageCacheUsage used;
    if(healthy())for(const auto& cache:image_cache)if(cache.owner==this){++used.entries;used.pages+=cache.pages;}
    return used;
}
Picture Content::image(int id)const{
    return image_payload(id,nullptr,0);
}
void Content::cancel_preload()const{cancel_content_preload(this);}
PreparationResult Content::preload_image_step(int id)const{
    if(!healthy()||id<0||unsigned(id)>=images_)return PreparationResult::Failed;
    if(resource_transfer_uses)return PreparationResult::Pending;
    auto&p=content_preload;
    if(image_cached(id)){
        if(p.owner==this&&!p.is_glyph&&p.key==unsigned(id))cancel_content_preload(this);
        return PreparationResult::Ready;
    }
    const auto r=image_directory_[unsigned(id)];const unsigned format=r.format,length=r.length;
    const unsigned w=r.width(),h=r.height(),offset=r.offset;
    // Tiled animation reconstruction stays with its explicit animation
    // preparation path; it is never a static merchant inventory request.
    if(format==6)return PreparationResult::Failed;
    const auto fail=[&](){io_good_=false;clear_caches(this);return PreparationResult::Failed;};
    if(!w||w>320||!h||h>240||format>7||!length||length>MaxEncodedImage||
       (format==5&&version_<5)||(format==7&&(version_<RetainedContentVersion||length!=16))||
       !reader_.contains(offset,length))return fail();
    if(p.owner!=this||p.is_glyph||p.key!=unsigned(id)){
        cancel_content_preload();p.owner=this;p.reader=&reader_;p.directory=image_directory_;p.key=unsigned(id);
        p.descriptor=offset;p.stored_size=length;p.stored_format=p.format=uint8_t(format);
        p.w=uint16_t(w);p.h=uint16_t(h);p.size=length;
        p.phase=format==7?PreloadPhase::Descriptor:format==5?PreloadPhase::Lz4Header:PreloadPhase::Literal;
    }
    const auto result=step_content_preload();
    if(result==PreparationResult::Failed)return fail();
    if(result==PreparationResult::Ready&&image_pages_){
        if(const auto*entry=find_image(this,image_directory_,unsigned(id)))image_pages_[id]=entry->pages;
    }
    return result;
}
ImageDimensions Content::image_dimensions(int id)const{
    if(!healthy()||id<0||unsigned(id)>=images_)return {};
    const auto r=image_directory_[unsigned(id)];const unsigned w=r.width(),h=r.height();
    if(!w||w>320||!h||h>240)return {};
    return {uint16_t(w),uint16_t(h)};
}
void Content::prepare_images(const int* ids,unsigned count)const{
    // Called only for the pictures already known to be visible this frame.
    // This is synchronous work in the measured render, not a loading phase or
    // speculative neighbouring-page prefetch. Reuse the existing encoded
    // workspace and normal decoded-image pool; never allocate another pool.
    if(!healthy()||data_||!ids)return;
    struct Request {unsigned id;uint32_t offset,length;unsigned pages;};
    Request requests[32];unsigned n=0;
    for(unsigned i=0;i<count&&i<32;++i){
        const int id=ids[i];if(id<0||unsigned(id)>=images_||image_cached(id))continue;
        const auto r=image_directory_[unsigned(id)];
        const uint32_t offset=r.offset,length=r.length,format=r.format;
        const unsigned pages=image_page_bound(r);
        if(format>5||(format==5&&(version_<5||length<6))||!length||!pages||
           !reader_.contains(offset,length))continue;
        bool duplicate=false;
        for(unsigned j=0;j<n;++j)if(image_alias(int(requests[j].id),id)){duplicate=true;break;}
        if(duplicate)continue;
        unsigned at=n;
        while(at&&requests[at-1].offset>offset){requests[at]=requests[at-1];--at;}
        requests[at]={unsigned(id),offset,length,pages};++n;
    }
    for(unsigned first=0;first<n;){
        const uint32_t start=requests[first].offset;
        uint32_t end=start+requests[first].length;unsigned pages=requests[first].pages,last=first+1;
        while(last<n){
            const auto&next=requests[last];
            // Coalesce only tiny alignment gaps, and never cross the bounded
            // encoded workspace or decoded-page budget. Sparse distant cards
            // retain the regular single-image path and its eviction behavior.
            if(next.offset<end||next.offset-end>16||next.length>MaxEncodedImage||
               next.offset-start>MaxEncodedImage-next.length||
               next.pages>image_page_limit-pages)break;
            end=next.offset+next.length;pages+=next.pages;++last;
        }
        if(last-first>1){
            ResourceTransferLease transfer(ResourceTransferUse::Bytes);if(!transfer)return;
            uint8_t*encoded_pixels=transfer.bytes();
            if(!reader_.read(start,encoded_pixels,end-start)){io_good_=false;clear_caches(this);return;}
            for(unsigned i=first;i<last;++i){
                const auto&r=requests[i];
                if(!image_payload(int(r.id),encoded_pixels+r.offset-start,r.length).pixels)return;
            }
        }
        first=last;
    }
}
Picture Content::image_payload(int id,const uint8_t* supplied,uint32_t supplied_size)const{
    const Picture empty{nullptr,nullptr,nullptr,0,0,0};
    if(!metadata_||!io_good_||id<0||(unsigned)id>=images_)return empty;
    const auto fail=[&](){io_good_=false;clear_caches(this);return empty;};
    const auto r=image_directory_[unsigned(id)];const uint32_t off=r.offset,length=r.length;
    const uint32_t stored_format=r.format;
    uint16_t w=uint16_t(r.width()),h=uint16_t(r.height());unsigned fmt=stored_format,size=length;
    if(!w||w>320||!h||h>240||fmt>7||(fmt==5&&version_<5)||
       (fmt==6&&(!tile_metadata_||length!=16))||length>MaxEncodedImage||
       (fmt==7&&(version_<RetainedContentVersion||length!=16))||
       !reader_.contains(off,length))return fail();
    const uint8_t*p=nullptr;
    if(auto* cache=find_image(this,image_directory_,unsigned(id))){
        p=image_buffer(*cache);fmt=cache->format();size=unsigned(cache->pages)*ImagePageBytes;cache->age=next_image_age();
    }
    if(!p){
        // Coalesced literal requests already hold their encoded lease. Every
        // other cold image owns it until publication, including descriptor
        // callbacks and all groups of a tiled frame. A reentrant save/load
        // cannot replace bytes being decoded; a busy resource is not corrupt.
        const bool provided=supplied&&supplied_size==length&&fmt<=5;
        ResourceTransferLease transfer(provided?ResourceTransferUse::None:ResourceTransferUse::Bytes);
        if(!provided&&!transfer)return empty;
        // A different visible frame must not repeatedly discard the stock
        // dictionary being prepared between frames. Its raw/input groups and
        // unpublished image/glyph entries already hold separate reservations.
        // The same image still preempts preparation and publishes synchronously.
        if(content_preload.owner==this&&!content_preload.is_glyph&&
           image_alias(id,int(content_preload.key)))cancel_content_preload(this);
        uint8_t*encoded_pixels=transfer.bytes();
        if(fmt==7){
            uint8_t row[16];if(!reader_.read(off,row,sizeof row))return fail();
            // Relative group positions keep common artwork identical across
            // language files, whose resident directories have different sizes.
            const int64_t position=int64_t(off)+int32_t(u32(row));
            const uint32_t group_length=u32(row+4),raw_offset=u32(row+8),format_size=u32(row+12);
            fmt=format_size>>24;size=format_size&0xffffffu;
            if(position<0||position>MaxContentFile||fmt>4||!size||size>MaxImageBytes)return fail();
            TileGroup* group=load_retained_group(this,reader_,uint32_t(position),group_length,encoded_pixels);
            if(!group||raw_offset>group->size||size>group->size-raw_offset)return fail();
            ImageReservation reservation(size);uint8_t* output=reservation.bytes();if(!output)return fail();
            const uint8_t* raw=tile_bytes(*group)+raw_offset;
            for(unsigned i=0;i<size;++i)output[i]=raw[i];
            p=output;if(!valid_image(p,size,w,h,fmt))return fail();
            fmt=reservation.publish(this,image_directory_,uint32_t(id),fmt);
        }else if(fmt==6){
            TileFrame frame;
            if(!tile_metadata_||length!=16||!frame.open(this,reader_,tile_metadata_,
                    uint32_t(tile_metadata_-metadata_)+directory_reduction_,off,w,h,encoded_pixels))return fail();
            size=frame.output_size;fmt=4;
            ImageReservation reservation(size);uint8_t* output=reservation.bytes();if(!output)return fail();
            if(!rebuild_tile_frame(this,reader_,tile_metadata_,frame,w,h,output,encoded_pixels))return fail();
            p=output;fmt=reservation.publish(this,image_directory_,uint32_t(id),fmt);
        }else{
            if(fmt==5&&length<6)return fail();
            const uint8_t* source=provided?supplied:payload(off,length,encoded_pixels,ResourceTransferCapacity);if(!source)return fail();
            // The descriptor already gives the stored byte count. Read it
            // once and parse its five-byte LZ4 header in the same buffer; a
            // separate header read used to fetch an extra 4 KiB cache page.
            if(fmt==5){fmt=source[0];size=u32(source+1);}
            if(fmt>4||!size||size>MaxImageBytes)return fail();
            if(stored_format!=5&&data_){p=source;if(!valid_image(p,size,w,h,fmt))return fail();}
            else {
                ImageReservation reservation(size);uint8_t*output=reservation.bytes();if(!output)return fail();
                if(stored_format==5){if(!decode_lz4(source+5,length-5,output,size))return fail();}
                else for(unsigned i=0;i<size;i++)output[i]=source[i];
                p=output;if(!valid_image(p,size,w,h,fmt))return fail();fmt=reservation.publish(this,image_directory_,uint32_t(id),fmt);
            }
        }
    }
    if(image_pages_&&!(data_&&stored_format<5))image_pages_[id]=uint8_t((size+ImagePageBytes-1)/ImagePageBytes);
    if(fmt==4)return {p,nullptr,nullptr,w,h,4};
    if(fmt>=2)return {p+512,fmt==3?p+512+w*h:nullptr,p,w,h,(uint8_t)fmt};
    return {p,fmt?p+w*h*2:nullptr,nullptr,w,h,(uint8_t)fmt};
}
int Content::font_size(int preferred,unsigned style,bool at_most,bool sparse_scene_sizes)const{
    if(!metadata_||!io_good_||!glyphs_)return 8;
    if(font_size_generation_!=generation_){font_size_generation_=generation_;font_sizes_known_=0;}
    unsigned face=version_>=HistoryUi8ContentVersion?(style&7u):version_>=Ui7ContentVersion?(style&3u):0u;
    const auto sizes=[&](unsigned selected){
        if(!(font_sizes_known_&(1u<<selected))){
            uint32_t mask=0;const unsigned band=((selected&3u)<<6)|((selected&4u)<<3);
            for(unsigned px=1;px<=(version_>=HistoryUi8ContentVersion?31u:32u);++px){
                const unsigned encoded=band|px;unsigned lo=0,hi=glyphs_;
                while(lo<hi){const unsigned mid=lo+(hi-lo)/2;const auto*r=metadata_+image_directory_end()+mid*16;
                    if(u16(r+8)<encoded)lo=mid+1;else hi=mid;}
                if(lo<glyphs_){const auto*r=metadata_+image_directory_end()+lo*16;
                    if(u16(r+8)==encoded&&!kerning_codepoint(u32(r)))mask|=1u<<(px-1);}
            }
            font_sizes_[selected]=mask;font_sizes_known_|=uint8_t(1u<<selected);
        }return font_sizes_[selected];
    };
    uint32_t mask=sizes(face);if(face&4u)mask|=sizes(face&3u);if(face)mask|=sizes(0);
    // Seven-pixel relic titles and nine-pixel setup headings have sparse
    // scene alphabets. Only those labels opt in: automatic shrink steps keep
    // their complete existing6/8px alphabets and identical old metrics.
    if(!sparse_scene_sizes)mask&=~((1u<<6)|(1u<<8));
    if(!mask)return 8;preferred=preferred<1?1:preferred>32?32:preferred;
    if(at_most){for(int px=preferred;px>0;--px)if(mask&(1u<<(px-1)))return px;
        for(int px=preferred+1;px<=32;++px)if(mask&(1u<<(px-1)))return px;}
    for(int distance=0;distance<32;++distance){const int lower=preferred-distance,upper=preferred+distance;
        if(lower>=1&&(mask&(1u<<(lower-1))))return lower;
        if(upper<=32&&(mask&(1u<<(upper-1))))return upper;}
    return 8;
}
const uint8_t* Content::glyph_record(uint32_t cp,int size,unsigned style)const{
    if(!metadata_||!io_good_||size<=0||size>32||cp>0x10ffff)return nullptr;
    // UI8 uses the previously unused size bit5 for a scene-specific family
    // (Spectral / Kreon bold / Bitter italic). Existing UI7 sizes and faces
    // retain their exact keys. Every missing face has a regular fallback.
    unsigned face=version_>=HistoryUi8ContentVersion?(style&7u):version_>=Ui7ContentVersion?(style&3u):0u;
    unsigned band=((face&3u)<<6)|((face&4u)<<3);
    // A locale may contain only Regular, or no combined BoldItalic band.
    // Sorted resident metadata already proves that absence: skip a whole
    // failed binary search without scanning fonts or adding cache state.
    if(face&&glyphs_&&u16(metadata_+image_directory_end()+(glyphs_-1)*16+8)<band){face=band=0;}
    const uint32_t key=((uint32_t(size)|band)<<24)|cp;
    const uint8_t*base=metadata_+image_directory_end(),*cached=nullptr;
    if(glyph_records.lookup(this,key,base,cached))return cached;
    int lo=0,hi=(int)glyphs_;
    while(lo<hi){int mid=(lo+hi)/2;const uint8_t*r=base+mid*16;uint32_t k=(uint32_t)u16(r+8)<<24|u32(r);if(k<key)lo=mid+1;else hi=mid;}
    if((unsigned)lo<glyphs_){const uint8_t*r=base+lo*16;if(((uint32_t)u16(r+8)<<24|u32(r))==key){
        const unsigned w=r[10],h=r[11],stored=u16(r+14),encoded=stored&~GroupedGlyphFlag;
        const unsigned sized=u16(r+8),pixel_size=sized&(version_>=HistoryUi8ContentVersion?31u:63u);
        if(!w||w>40||!h||h>40||!pixel_size||pixel_size>32||sized>255||(version_<Ui7ContentVersion&&sized>32)||!valid_glyph_advance(cp,u16(r+12),version_)||
           ((stored&GroupedGlyphFlag)&&version_<GroupedGlyphContentVersion)||
           (encoded&&(version_<5||encoded%2||encoded>2*w*h))){io_good_=false;return nullptr;}
        return glyph_records.remember(key,base,r);
    }}
    const uint8_t*fallback=face?glyph_record(cp,size,(face&4u)?face&3u:0):cp!='?'?glyph_record('?',size,0):nullptr;
    // Recursive fallback may find a malformed record. It must poison this
    // owner exactly as before and must never publish a successful memo entry.
    return io_good_?glyph_records.remember(key,base,fallback):nullptr;
}
bool Content::has_exact_glyph(uint32_t cp,int size,unsigned style)const{
    if(style>7||size<=0||size>31||text_format_control(cp)||kerning_codepoint(cp)||inline_icon(cp))return false;
    const uint8_t* row=glyph_record(cp,size,style);
    const unsigned band=unsigned(size)|((style&3u)<<6)|((style&4u)<<3);
    return row&&u32(row)==cp&&u16(row+8)==band;
}
Glyph Content::glyph_metrics(uint32_t cp,int size,unsigned style)const{
    if(!metadata_||!io_good_||text_format_control(cp)||kerning_codepoint(cp))return {nullptr,0,0,0};
    if(inline_icon(cp)&&size>0&&size<=255)return {nullptr,uint8_t(size),uint8_t(size),uint16_t(size)};
    const uint8_t*r=glyph_record(cp,size,style);
    return r?Glyph{nullptr,r[10],r[11],uint16_t(text_pixel(glyph_advance64(u16(r+12)))),uint16_t(u16(r+14)&~GroupedGlyphFlag),uint16_t(glyph_advance64(u16(r+12))),int8_t(glyph_bearing(u16(r+12)))}:Glyph{nullptr,0,0,uint16_t(size/2)};
}
int Content::glyph_kerning64(uint32_t left,uint32_t right,int size,unsigned style)const{
    if(version_<Ui12ContentVersion||left<32||left>126||right<32||right>126)return 0;
    unsigned face=style&7u;
    if(face){
        // A scene face may fall back to Regular at a missing raster size.
        // Use the face which actually supplied both letters. Metadata reads
        // share the glyph-record memo; no raster/stream access is needed.
        const auto*l=glyph_record(left,size,face),*r=glyph_record(right,size,face);
        if(!l||!r||u32(l)!=left||u32(r)!=right)return 0;
        const unsigned lb=u16(l+8),rb=u16(r+8);
        const unsigned lf=((lb>>6)&3u)|((lb>>3)&4u),rf=((rb>>6)&3u)|((rb>>3)&4u);
        if(lf!=rf)return 0;face=lf;
    }
    face=kerning_font_style(face);
    const unsigned band=KerningMetadataSize|((face&3u)<<6)|((face&4u)<<3);
    const uint32_t cp=kerning_codepoint(left,right);const auto*r=glyph_record(cp,KerningMetadataSize,face);
    // An absent pair means zero for this face. glyph_record's normal regular
    // fallback is useful for letters but must never import another font's pair.
    if(!r||u32(r)!=cp||u16(r+8)!=band||(u16(r+12)&0xf000u)!=KerningGlyphAdvance)return 0;
    const int value=(int(u16(r+12)&4095u)-2048)*size;
    return value<0?-((-value+16)/32):(value+16)/32;
}
uint32_t Content::retained_decode_serial()const{return retained_decodes;}
bool Content::glyph_cached(uint32_t cp,int size,unsigned style)const{
    if(!metadata_||!io_good_||text_format_control(cp)||kerning_codepoint(cp)||inline_icon(cp))return true;
    const uint8_t*r=glyph_record(cp,size,style);if(!r)return true;
    const unsigned stored=u16(r+14),encoded=stored&~GroupedGlyphFlag;
    if(data_&&!(stored&GroupedGlyphFlag))return true;
    const unsigned length=encoded?encoded:unsigned(r[10])*r[11];
    return glyph_cache.cached(this,u32(r+4),length)!=nullptr;
}
PreparationResult Content::preload_glyph_step(uint32_t cp,int size,unsigned style)const{
    if(!healthy())return PreparationResult::Failed;
    if(resource_transfer_uses)return PreparationResult::Pending;
    if(text_format_control(cp)||kerning_codepoint(cp)||inline_icon(cp))return PreparationResult::Ready;
    const uint8_t*r=glyph_record(cp,size,style);
    if(!r)return healthy()?PreparationResult::Ready:PreparationResult::Failed;
    const unsigned stored=u16(r+14),encoded=stored&~GroupedGlyphFlag;
    const unsigned length=encoded?encoded:unsigned(r[10])*r[11];const uint32_t offset=u32(r+4);
    const bool grouped=stored&GroupedGlyphFlag;
    auto&p=content_preload;
    if((data_&&!grouped)||glyph_cache.cached(this,offset,length)){
        if(p.owner==this&&p.is_glyph&&p.descriptor==offset)cancel_content_preload(this);
        return PreparationResult::Ready;
    }
    const auto fail=[&](){io_good_=false;clear_caches(this);return PreparationResult::Failed;};
    if(!reader_.contains(offset,grouped?GroupedGlyphDescriptorBytes:length))return fail();
    if(p.owner!=this||!p.is_glyph||p.descriptor!=offset){
        cancel_content_preload();p.owner=this;p.reader=&reader_;p.is_glyph=true;p.descriptor=offset;p.size=length;
        p.phase=grouped?PreloadPhase::Descriptor:PreloadPhase::Literal;
    }
    const auto result=step_content_preload();return result==PreparationResult::Failed?fail():result;
}
Glyph Content::glyph(uint32_t cp,int size,unsigned style)const{
    if(!metadata_||!io_good_||text_format_control(cp)||kerning_codepoint(cp))return {nullptr,0,0,0};
    if(inline_icon(cp))return glyph_metrics(cp,size);
    if(const uint8_t*r=glyph_record(cp,size,style)){
        const unsigned stored=u16(r+14),encoded=stored&~GroupedGlyphFlag;
        const uint32_t off=u32(r+4),len=encoded?encoded:unsigned(r[10])*r[11];const uint8_t*bytes=nullptr;
        const bool grouped=stored&GroupedGlyphFlag;
        if(!reader_.contains(off,grouped?GroupedGlyphDescriptorBytes:len)){io_good_=false;return {nullptr,0,0,0};}
        if(grouped){
            bytes=glyph_cache.cached(this,off,len);
            if(!bytes){
                ResourceTransferLease transfer(ResourceTransferUse::Bytes);
                if(!transfer)return {nullptr,0,0,0}; // A reentrant miss must not corrupt an active save/image transfer.
                if(content_preload.owner==this&&content_preload.is_glyph&&
                   content_preload.descriptor==off)cancel_content_preload(this);
                const GroupedGlyphRead read{this,&reader_,transfer.bytes()};
                bytes=glyph_cache.read_with(this,off,len,&read,read_grouped_glyph);
                if(!bytes)io_good_=false;
            }
        }else if(data_)bytes=data_+off;
        else {if(!glyph_cache.cached(this,off,len)&&!resource_transfer_uses&&
                 content_preload.owner==this&&content_preload.is_glyph&&content_preload.descriptor==off)cancel_content_preload(this);
            bytes=glyph_cache.read(this,off,len,reader_);if(!bytes)io_good_=false;}
        if(!bytes)return {nullptr,0,0,0};return {bytes,r[10],r[11],uint16_t(text_pixel(glyph_advance64(u16(r+12)))),uint16_t(encoded),uint16_t(glyph_advance64(u16(r+12))),int8_t(glyph_bearing(u16(r+12)))};
    }
    return {nullptr,0,0,(uint16_t)(size/2)};
}
const char* Content::string(int id)const{
    if(!metadata_||!io_good_||id<0||(unsigned)id>=strings_)return "";
    const uint8_t*r=metadata_+image_directory_end()+glyphs_*16+id*8;
    const uint32_t off=u32(r),stored=u32(r+4),size=stored&~GroupedStringFlag;
    const bool grouped=bool(stored&GroupedStringFlag);
    if(size>=MaxStringBytes||(grouped&&version_<GroupedStringContentVersion)||
       !reader_.contains(off,grouped?GroupedStringDescriptorBytes:size+1)){io_good_=false;return "";}
    const uint8_t*bytes=nullptr;
    if(grouped){
        bytes=string_cache.cached(this,off,size+1);
        if(!bytes){
            ResourceTransferLease transfer(ResourceTransferUse::Bytes);
            // A reentrant cache miss may not modify an active save/image
            // workspace or evict text; it remains retryable after the lease.
            if(!transfer)return "";
            const GroupedStringRead read{this,&reader_,transfer.bytes()};
            bytes=string_cache.read_with(this,off,size+1,&read,read_grouped_string);
        }
    }else if(data_)bytes=data_+off;
    else bytes=string_cache.read(this,off,size+1,reader_);
    if(!bytes||bytes[size]){io_good_=false;return "";}
    return reinterpret_cast<const char*>(bytes);
}
Clip Content::clip(int id)const{
    if(!metadata_||id<0||(unsigned)id>=clips_)return {0,0,1,0,0};
    const uint8_t*r=metadata_+image_directory_end()+glyphs_*16+strings_*8+id*16;
    const uint32_t first=u32(r);const uint16_t count=u16(r+4),ms=u16(r+6);
    if(first>=images_||!count||count>images_-first||!ms)return {0,0,1,0,0};
    const uint32_t units=version_>=CharacterDeathContentVersion?u32(r+12):0;
    if(units>=1000000)return {0,0,1,0,0};
    return {first,count,ms,(int16_t)u16(r+8),(int16_t)u16(r+10),units};
}
int Content::card_image(int kind)const{
    if(!metadata_||kind<0||(unsigned)kind>=cards_)return -1;
    if(version_==3)return A_Card0+kind;
    const uint8_t*r=metadata_+image_directory_end()+glyphs_*16+strings_*8+clips_*16+kind*16;return u32(r)<images_?(int)u32(r):-1;
}
const char* Content::card_string(int kind,int variant)const{
    if(!metadata_||kind<0||(unsigned)kind>=cards_||variant<0||variant>2)return "";
    if(version_==3)return string(kind*3+variant);
    const uint8_t*r=metadata_+image_directory_end()+glyphs_*16+strings_*8+clips_*16+kind*16;return string((int)u32(r+4+variant*4));
}
int Content::ui_image(int key)const{
    if(!metadata_||!ui_offset_||key<0||(unsigned)key>=ui_count_)return -1;
    const uint32_t image=u32(metadata_+ui_offset_+key*4);return image<images_?(int)image:-1;
}
int Content::relic_image(int index)const{
    if(index<0||index>=WorldRelicKinds)return -1;
    if(version_>=14)return world_image("relics",world_relic_key(RelicId(index)));
    if(index>=RelicKinds)return -1;
    if(index>=30)return ui_image(U_CrackedCore+index-30);
    return index>=25?ui_image(U_Kunai+index-25):index>=17?ui_image(U_HornCleat+index-17):index==16?ui_image(U_SnakeRing):index<10?relic_definitions[index].asset:ui_image(U_Strawberry+index-10);
}
int Content::frame_image(Card card)const{
    if(card.kind==Soul)return ui_image(U_SoulFrame);
    if(card.kind==SovereignBlade)return ui_image(U_BladeFrame);
    if(card.kind>=CardKinds)return -1;
    const int type=(int)definitions[card.kind].type;
    // The reference has skill-shaped status/curse frames; never index past
    // an attack/skill/power triplet with the appended non-playable types.
    if(type==(int)CardType::Curse)return ui_image(U_CurseFrame);
    if(type==(int)CardType::Status)return ui_image(U_ColorlessSkill);
    if(type==(int)CardType::Quest)return version_>=14?world_image("cards","QUEST_FRAME"):ui_image(U_ColorlessSkill);
    const CardColor color=card_color(card.kind);
    int base=color==CardColor::Silent?U_GreenAttack:color==CardColor::Defect?U_BlueAttack:
        color==CardColor::Regent?U_OrangeAttack:color==CardColor::Necrobinder?U_PinkAttack:
        (color==CardColor::Colorless||color==CardColor::Token)?U_ColorlessAttack:U_RedAttack;
    const int image=ui_image(base+type);
    return image>=0?image:ui_image(U_BigAttack+type);
}
int Content::energy_image(Card card,bool unknown)const{
    if(card.kind>=CardKinds||(!unknown&&cost(card)==-2))return -1;
    if(card.kind==Soul)return ui_image(U_SoulEnergy);
    if(card.kind==SovereignBlade)return ui_image(U_BladeEnergy);
    const CardColor color=card_color(card.kind);
    return color==CardColor::Silent?ui_image(U_SilentEnergy):color==CardColor::Defect?ui_image(U_DefectEnergy):
        color==CardColor::Regent?ui_image(U_RegentEnergy):color==CardColor::Necrobinder?ui_image(U_NecrobinderEnergy):
        (color==CardColor::Colorless||color==CardColor::Token)?ui_image(U_ColorlessEnergy):A_Energy;
}
int Content::potion_image(Potion p)const{
    unsigned i=(unsigned)p;if(!i||i>PotionKinds)return -1;
    return i>16?ui_image(U_ExtPotionStart+(int)i-17):i<=3?A_FirePotion+(int)i-1:ui_image(U_EnergyPotion+(int)i-4);
}
#include "world_content.inc"
#include "locale_content.inc"
} // namespace spire
