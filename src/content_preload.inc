// One foreground preparation task shared by static images and glyphs. The
// dictionary, probability table and input page live in the existing tile pool;
// unpublished image/glyph reservations use their existing final caches.
constexpr unsigned PreloadOutputBytes=512,PreloadInputBytes=4096;
constexpr unsigned TilePreloadOutputBytes=4096,TilePreloadInputBytes=32768;
static_assert(Lzma2ProbabilityBytes+PreloadInputBytes<=RetainedPageBytes,
              "Incremental LZMA2 workspace must fit one existing 32 KiB quarter");
enum class PreloadPhase:uint8_t {Descriptor,Header,Decode,FilterSlice,FilterColors,PlaneCopy,PlaneRestore,
    ReserveCopy,Copy,Literal,Lz4Header,Lz4,Verify,
    TileDescriptor,TileGroup,TileDecode,TilePredict,TileFilter,TileSparseScan,TileSparseRestore,TileReady,
    TilePlan,TileGeometry,TileNeeds,TileSelect,TileCopy,TileDenseClear,TileDenseOutput};
struct ContentPreload {
    const Content*owner=nullptr;
    const ResourceReader*reader=nullptr;
    ImageDirectoryView directory{};
    TileGroup*raw=nullptr,*workspace=nullptr;
    TileGroup*index=nullptr,*restored=nullptr;
    const uint8_t*tile_header=nullptr;
    ImageCache*image=nullptr;
    GlyphByteCache::Reservation glyph{};
    Lzma2BlockStream stream{};
    RetainedPlanesCursor planes{};
    uint32_t key=0,descriptor=0,stored_size=0,group_offset=0,group_length=0;
    uint32_t raw_offset=0,size=0,position=0,source_offset=0,source_size=0,loaded=0;
    uint32_t filter_count=0,filter_index=0,filter_end=0,slice_offset=0,slice_size=0;
    uint32_t scan=0,colors=0,spans=0,rows=0,previous=0;
    uint32_t lz4_length=0,lz4_offset=0;
    uint32_t tile_header_offset=0,tile_record_offset=0,tile_record_length=0,tile_id=0,tile_copied=0,tile_geometry=0;
    uint16_t tile_columns=0,tile_count=0,tile_needed=0,tile_next=0,tile_slot=0;
    uint8_t tile_pass=0,tile_geometry_mode=0;
    uint16_t input_at=0,input_size=0,w=0,h=0;
    uint8_t format=0,stored_format=0,filter_mode=0,slice_format=0,stride=0,lz4_phase=0,lz4_token=0;
    PreloadPhase phase=PreloadPhase::Descriptor;
    bool is_glyph=false,opaque=true;
};
static ContentPreload content_preload;
static_assert(sizeof(ContentPreload)<=704,"Preparation must retain only small task metadata");
static void cancel_content_preload(const Content*owner){
    auto&p=content_preload;if(!p.owner||(owner&&p.owner!=owner))return;
    if(p.image&&!p.image->owner)discard_image(*p.image);
    glyph_cache.cancel(p.glyph);
    if(p.raw){if(p.raw->owner)p.raw->first&=127u;else tile_discard(*p.raw);}
    if(p.index){if(p.index->owner)p.index->first&=127u;else tile_discard(*p.index);}
    if(p.restored)tile_discard(*p.restored);
    if(p.workspace)tile_discard(*p.workspace);
    if(implicit_dense_owner==&p)release_implicit_scratch(&p);
    p={};
}
static uint8_t* preload_output(ContentPreload&p){return p.is_glyph?p.glyph.data:image_buffer(*p.image);}
static uint8_t* preload_input(ContentPreload&p){return tile_bytes(*p.workspace)+Lzma2ProbabilityBytes;}
static bool preload_workspace(ContentPreload&p){
    if(p.workspace)return true;
    // Anchor probabilities at the first page in the large-tile room tier.
    // The other three edge pages must stay contiguous for the alpha plane.
    p.workspace=reserve_tile_group(RetainedPageBytes*(p.stored_format==6?2u:1u),true,true);if(!p.workspace)return false;
    p.workspace->first|=128u;return true;
}
static bool preload_reserve_output(ContentPreload&p){
    if(!p.size||p.size>(p.is_glyph?MaxGlyphBytes:MaxImageBytes))return false;
    if(p.is_glyph){p.glyph=glyph_cache.reserve(p.size);return p.glyph.data!=nullptr;}
    if(p.format<4){
        const unsigned pixels=unsigned(p.w)*p.h;
        const unsigned expected=p.format<2?pixels*(p.format?3u:2u):512+pixels*(p.format==3?2u:1u);
        if(p.size!=expected)return false;
    }
    p.image=reserve_image(p.size);return p.image!=nullptr;
}
static bool preload_fill(ContentPreload&p){
    if(p.input_at!=p.input_size||p.loaded>=p.source_size)return false;
    const unsigned capacity=p.stored_format==6?TilePreloadInputBytes:PreloadInputBytes;
    const unsigned left=p.source_size-p.loaded,n=left<capacity?left:capacity;
    if(p.stored_format==6?!p.reader->read(p.source_offset+p.loaded,preload_input(p),n):
       !p.reader->read_bounded(p.source_offset+p.loaded,preload_input(p),n))return false;
    p.loaded+=n;p.input_at=0;p.input_size=uint16_t(n);return true;
}
static void preload_group_ready(ContentPreload&p){
    p.raw->id=0x80000000u|p.group_offset;p.raw->age=next_tile_age();p.raw->owner=p.owner;
    ++retained_decodes;p.phase=PreloadPhase::ReserveCopy;
    if(p.workspace){tile_discard(*p.workspace);p.workspace=nullptr;}
}
static bool preload_filter_colors(ContentPreload&p){
    auto*base=tile_bytes(*p.raw)+p.slice_offset;unsigned budget=PreloadOutputBytes;
    const auto restore=[&](uint8_t*data,unsigned count,unsigned stride){
        if(p.filter_mode==2||p.filter_mode==10){if(stride==3)retained_filter_detail::colors<2,3>(data,count);
            else retained_filter_detail::colors<2,2>(data,count);}
        else {if(stride==3)retained_filter_detail::colors<1,3>(data,count);
            else retained_filter_detail::colors<1,2>(data,count);}
    };
    while(budget>=5){
        if(p.colors){
            const unsigned count=p.colors<budget/2?p.colors:budget/2;
            restore(base+p.scan,count,p.stride);p.scan+=count*p.stride;p.colors-=count;budget-=count*2;
            if(p.colors)return true;
        }
        if(p.slice_format<4){p.scan=p.slice_size;break;}
        if(p.spans){
            if(p.slice_size-p.scan<5)return false;
            const unsigned x=u16(base+p.scan),count=u16(base+p.scan+2),alpha=base[p.scan+4];
            if(!count||alpha>1||x<p.previous||x>320||count>320-x)return false;
            p.previous=x+count;p.scan+=5;budget-=5;p.stride=uint8_t(2+alpha);
            if(count>(p.slice_size-p.scan)/p.stride)return false;
            p.colors=count;--p.spans;continue;
        }
        if(p.scan==p.slice_size)break;
        if(p.slice_size-p.scan<2||p.rows>=240)return false;
        p.spans=u16(base+p.scan);if(p.spans>320)return false;
        p.scan+=2;budget-=2;++p.rows;p.previous=0;
    }
    if(p.scan==p.slice_size&&!p.colors&&!p.spans){
        p.filter_end=p.slice_offset+p.slice_size;++p.filter_index;p.phase=PreloadPhase::FilterSlice;
    }
    return true;
}
static PreparationResult preload_lz4(ContentPreload&p){
    unsigned budget=PreloadOutputBytes;auto*out=preload_output(p);const auto*input=preload_input(p);
    const auto byte=[&](unsigned&value){
        if(p.input_at==p.input_size)return false;
        value=input[p.input_at++];return true;
    };
    // Header and match-extension reads are bounded by the one input page.
    while(budget){
        unsigned value=0;
        switch(p.lz4_phase){
        case 0:
            if(!byte(value))return p.loaded==p.source_size?PreparationResult::Failed:PreparationResult::Pending;
            p.lz4_token=uint8_t(value);p.lz4_length=value>>4;p.lz4_phase=p.lz4_length==15?1:2;break;
        case 1:
            if(!byte(value))return p.loaded==p.source_size?PreparationResult::Failed:PreparationResult::Pending;
            if(p.lz4_length>p.size||value>p.size-p.lz4_length)return PreparationResult::Failed;
            p.lz4_length+=value;if(value!=255)p.lz4_phase=2;break;
        case 2:{
            if(p.lz4_length>p.size-p.position)return PreparationResult::Failed;
            unsigned n=p.lz4_length<budget?p.lz4_length:budget;
            const unsigned available=p.input_size-p.input_at;if(n>available)n=available;
            for(unsigned i=0;i<n;++i)out[p.position++]=input[p.input_at++];
            p.lz4_length-=n;budget-=n;
            if(p.lz4_length)return p.input_at==p.input_size&&p.loaded==p.source_size?PreparationResult::Failed:PreparationResult::Pending;
            if(p.input_at==p.input_size&&p.loaded==p.source_size)
                return p.position==p.size?PreparationResult::Ready:PreparationResult::Failed;
            p.lz4_phase=3;break;
        }
        case 3:
            if(!byte(value))return p.loaded==p.source_size?PreparationResult::Failed:PreparationResult::Pending;
            p.lz4_offset=value;p.lz4_phase=4;break;
        case 4:
            if(!byte(value))return p.loaded==p.source_size?PreparationResult::Failed:PreparationResult::Pending;
            p.lz4_offset|=value<<8;
            if(!p.lz4_offset||p.lz4_offset>p.position)return PreparationResult::Failed;
            p.lz4_length=(p.lz4_token&15u)+4;p.lz4_phase=(p.lz4_token&15u)==15u?5:6;break;
        case 5:
            if(!byte(value))return p.loaded==p.source_size?PreparationResult::Failed:PreparationResult::Pending;
            if(p.lz4_length>p.size||value>p.size-p.lz4_length)return PreparationResult::Failed;
            p.lz4_length+=value;if(value!=255)p.lz4_phase=6;break;
        case 6:{
            if(p.lz4_length>p.size-p.position)return PreparationResult::Failed;
            const unsigned n=p.lz4_length<budget?p.lz4_length:budget;
            for(unsigned i=0;i<n;++i){out[p.position]=out[p.position-p.lz4_offset];++p.position;}
            p.lz4_length-=n;budget-=n;if(!p.lz4_length)p.lz4_phase=0;break;
        }
        default:return PreparationResult::Failed;
        }
    }
    return PreparationResult::Pending;
}
static PreparationResult preload_verify(ContentPreload&p){
    if(p.is_glyph){
        if(!glyph_cache.publish(p.glyph,p.owner,p.descriptor,p.size))return PreparationResult::Failed;
        cancel_content_preload(nullptr);return PreparationResult::Ready;
    }
    const auto*bytes=preload_output(p);
    if(p.format==4){
        unsigned budget=PreloadOutputBytes;
        while(budget>=5&&(p.rows<p.h||p.spans)){
            if(p.spans){
                if(p.size-p.scan<5)return PreparationResult::Failed;
                const unsigned x=u16(bytes+p.scan),n=u16(bytes+p.scan+2),mode=bytes[p.scan+4];
                p.scan+=5;budget-=5;
                if(!n||mode>1||x<p.previous||x>p.w||n>p.w-x||n*(2+mode)>p.size-p.scan)return PreparationResult::Failed;
                p.previous=x+n;p.scan+=n*(2+mode);--p.spans;
            }else{
                if(p.size-p.scan<2)return PreparationResult::Failed;
                p.spans=u16(bytes+p.scan);if(p.spans>p.w)return PreparationResult::Failed;
                p.scan+=2;budget-=2;++p.rows;p.previous=0;
            }
        }
        if(p.rows<p.h||p.spans)return PreparationResult::Pending;
        if(p.scan!=p.size)return PreparationResult::Failed;
    }else if(p.format==1||p.format==3){
        const unsigned count=unsigned(p.w)*p.h;
        const auto*alpha=bytes+(p.format==1?count*2:512+count);
        const unsigned left=count-p.scan,n=left<PreloadOutputBytes?left:PreloadOutputBytes;
        for(unsigned i=0;i<n;++i)if(alpha[p.scan+i]!=255)p.opaque=false;
        p.scan+=n;if(p.scan<count)return PreparationResult::Pending;
        if(p.opaque)--p.format;
    }
    ImageReservation reservation(0);reservation.cache=p.image;
    reservation.publish(p.owner,p.directory,p.key,p.format,true);p.image=nullptr;
    cancel_content_preload(nullptr);return PreparationResult::Ready;
}
#include "content_preload_tiles.inc"
static PreparationResult step_content_preload(){
    auto&p=content_preload;
    // A lease exists only during this call, excluding reentrant cold readers.
    // No input or probability bytes point into this foreground workspace.
    ResourceTransferLease transfer(ResourceTransferUse::Bytes);
    if(!transfer)return PreparationResult::Pending;
    if(p.stored_format==6&&p.phase!=PreloadPhase::Verify)return preload_tile_step(p);
    switch(p.phase){
    case PreloadPhase::Descriptor:{
        uint8_t row[16];if(!p.reader->read_bounded(p.descriptor,row,sizeof row))return PreparationResult::Failed;
        const int64_t at=int64_t(p.descriptor)+int32_t(u32(row));
        p.group_length=u32(row+4);p.raw_offset=u32(row+8);const unsigned shape=u32(row+12);
        if(p.is_glyph){if(shape!=p.size)return PreparationResult::Failed;}
        else {p.format=uint8_t(shape>>24);p.size=shape&0xffffffu;if(p.format>4||!p.size||p.size>MaxImageBytes)return PreparationResult::Failed;}
        if(at<0||at>MaxContentFile||p.group_length<33||p.group_length>ResourceTransferCapacity||
           !p.reader->contains(uint32_t(at),p.group_length)||
           !(uint64_t(at)+p.group_length<=p.descriptor||uint64_t(p.descriptor)+16<=uint64_t(at)))return PreparationResult::Failed;
        p.group_offset=uint32_t(at);
        if(auto*hit=find_tile_group(p.owner,0x80000000u|p.group_offset)){
            if(p.is_glyph&&hit->size>GroupedGlyphRawLimit)return PreparationResult::Failed;
            p.raw=hit;p.raw->first|=128u;p.raw->age=next_tile_age();p.phase=PreloadPhase::ReserveCopy;
        }else p.phase=PreloadPhase::Header;
        return PreparationResult::Pending;
    }
    case PreloadPhase::Header:{
        if(!preload_workspace(p))return PreparationResult::Failed;
        uint8_t header[32];if(!p.reader->read_bounded(p.group_offset,header,sizeof header))return PreparationResult::Failed;
        const unsigned size=u32(header+4),encoded=u32(header+8),property=u32(header+12);
        if(!size||size>Lzma2BlockRawLimit||(p.is_glyph&&size>GroupedGlyphRawLimit)||
           property!=Lzma2DictionaryProperty)return PreparationResult::Failed;
        unsigned start=32;
        const bool planar=u32(header)==PlanarRetainedGroupMagic;
        if(u32(header)==FilteredRetainedGroupMagic||planar){
            p.filter_count=u32(header+16);const unsigned mode=u32(header+20),table=u32(header+24);
            if(p.owner->version()<(planar?RetainedPlanesContentVersion:RetainedFilterContentVersion)||
               mode>2||(!planar&&!mode)||u32(header+28)||
               !p.filter_count||p.filter_count>size/2||p.filter_count>(p.group_length-32)/8||table!=p.filter_count*8)return PreparationResult::Failed;
            p.filter_mode=uint8_t(mode+(planar?8u:0u));start+=table;
        }else if(u32(header)!=0x31524752u)return PreparationResult::Failed;
        if(!encoded||encoded!=p.group_length-start||p.raw_offset>size||p.size>size-p.raw_offset)return PreparationResult::Failed;
        p.raw=reserve_tile_group(size,true);if(!p.raw)return PreparationResult::Failed;
        p.raw->first|=128u;p.raw->size=size;
        if(!begin_lzma2_stream(p.stream,tile_bytes(*p.raw),tile_capacity(*p.raw),size,encoded,property,
                              tile_bytes(*p.workspace),Lzma2ProbabilityBytes))return PreparationResult::Failed;
        p.source_offset=p.group_offset+start;p.source_size=encoded;p.phase=PreloadPhase::Decode;
        return PreparationResult::Pending;
    }
    case PreloadPhase::Decode:{
        if(p.input_at==p.input_size&&p.loaded<p.source_size)
            return preload_fill(p)?PreparationResult::Pending:PreparationResult::Failed;
        uint32_t consumed=0;
        const auto result=step_lzma2_stream(p.stream,preload_input(p)+p.input_at,p.input_size-p.input_at,consumed,PreloadOutputBytes);
        p.input_at=uint16_t(p.input_at+consumed);
        if(result==Lzma2StepResult::Failed)return PreparationResult::Failed;
        if(result==Lzma2StepResult::Ready){
            // The LZMA stream is finished. Reuse its existing pool reservation
            // for one source slice rather than allocating another resident arena.
            if(p.filter_mode>=8){tile_discard(*p.workspace);p.workspace=nullptr;}
            if(p.filter_mode)p.phase=PreloadPhase::FilterSlice;else preload_group_ready(p);
        }
        return PreparationResult::Pending;
    }
    case PreloadPhase::FilterSlice:{
        if(p.filter_index==p.filter_count){
            if(p.filter_end!=p.raw->size)return PreparationResult::Failed;
            preload_group_ready(p);return PreparationResult::Pending;
        }
        uint8_t row[8];if(!p.reader->read_bounded(p.group_offset+32+p.filter_index*8,row,sizeof row))return PreparationResult::Failed;
        p.slice_offset=u32(row);const unsigned shape=u32(row+4);p.slice_size=shape&0xffffffu;p.slice_format=uint8_t(shape>>24);
        if(p.slice_offset!=p.filter_end||!p.slice_size||p.slice_format>4||p.slice_size>p.raw->size-p.filter_end)return PreparationResult::Failed;
        p.scan=p.colors=p.spans=p.rows=p.previous=0;p.stride=2;
        if(p.filter_mode>=8&&p.slice_format==4){
            if(p.workspace&&tile_capacity(*p.workspace)<p.slice_size){tile_discard(*p.workspace);p.workspace=nullptr;}
            if(!p.workspace){p.workspace=reserve_tile_group(p.slice_size,true);if(!p.workspace)return PreparationResult::Failed;p.workspace->first|=128u;}
            p.position=0;p.planes={};p.phase=PreloadPhase::PlaneCopy;return PreparationResult::Pending;
        }
        if(p.filter_mode==8){p.filter_end=p.slice_offset+p.slice_size;++p.filter_index;return PreparationResult::Pending;}
        if(p.slice_format<2){const unsigned stride=p.slice_format?3:2;
            if(p.slice_size%stride)return PreparationResult::Failed;p.colors=p.slice_size/stride;
        }else if(p.slice_format<4){
            if(p.slice_size<=512||(p.slice_format==3&&((p.slice_size-512)&1u)))return PreparationResult::Failed;
            p.colors=256;
        }
        p.phase=PreloadPhase::FilterColors;return PreparationResult::Pending;
    }
    case PreloadPhase::FilterColors:
        return preload_filter_colors(p)?PreparationResult::Pending:PreparationResult::Failed;
    case PreloadPhase::PlaneCopy:{
        const unsigned left=p.slice_size-p.position,n=left<PreloadOutputBytes?left:PreloadOutputBytes;
        auto*out=tile_bytes(*p.workspace);const auto*in=tile_bytes(*p.raw)+p.slice_offset;
        for(unsigned i=0;i<n;++i)out[p.position+i]=in[p.position+i];p.position+=n;
        if(p.position==p.slice_size)p.phase=PreloadPhase::PlaneRestore;
        return PreparationResult::Pending;
    }
    case PreloadPhase::PlaneRestore:{
        const auto result=step_retained_planes(p.planes,tile_bytes(*p.raw)+p.slice_offset,
            tile_bytes(*p.workspace),p.slice_size,p.filter_mode-8,PreloadOutputBytes);
        if(result==RetainedPlanesResult::Failed)return PreparationResult::Failed;
        if(result==RetainedPlanesResult::Ready){p.filter_end=p.slice_offset+p.slice_size;++p.filter_index;p.phase=PreloadPhase::FilterSlice;}
        return PreparationResult::Pending;
    }
    case PreloadPhase::ReserveCopy:
        if(p.raw_offset>p.raw->size||p.size>p.raw->size-p.raw_offset||!preload_reserve_output(p))return PreparationResult::Failed;
        p.position=0;p.phase=PreloadPhase::Copy;return PreparationResult::Pending;
    case PreloadPhase::Copy:{
        const unsigned left=p.size-p.position,n=left<PreloadOutputBytes?left:PreloadOutputBytes;
        auto*out=preload_output(p);const auto*in=tile_bytes(*p.raw)+p.raw_offset;
        for(unsigned i=0;i<n;++i)out[p.position+i]=in[p.position+i];p.position+=n;
        if(p.position==p.size){p.scan=p.spans=p.rows=p.previous=0;p.phase=PreloadPhase::Verify;}
        return PreparationResult::Pending;
    }
    case PreloadPhase::Literal:{
        if(!p.image&&!p.glyph.data&&!preload_reserve_output(p))return PreparationResult::Failed;
        const unsigned left=p.size-p.position,n=left<PreloadOutputBytes?left:PreloadOutputBytes;
        if(!p.reader->read_bounded(p.descriptor+p.position,preload_output(p)+p.position,n))return PreparationResult::Failed;
        p.position+=n;if(p.position==p.size)p.phase=PreloadPhase::Verify;
        return PreparationResult::Pending;
    }
    case PreloadPhase::Lz4Header:{
        if(!preload_workspace(p)||p.stored_size<6)return PreparationResult::Failed;
        uint8_t header[5];if(!p.reader->read_bounded(p.descriptor,header,sizeof header))return PreparationResult::Failed;
        p.format=header[0];p.size=u32(header+1);
        if(p.format>4||!preload_reserve_output(p))return PreparationResult::Failed;
        p.source_offset=p.descriptor+5;p.source_size=p.stored_size-5;p.phase=PreloadPhase::Lz4;
        return PreparationResult::Pending;
    }
    case PreloadPhase::Lz4:{
        if(p.input_at==p.input_size&&p.loaded<p.source_size)
            return preload_fill(p)?PreparationResult::Pending:PreparationResult::Failed;
        const auto result=preload_lz4(p);
        if(result==PreparationResult::Ready){tile_discard(*p.workspace);p.workspace=nullptr;p.phase=PreloadPhase::Verify;return PreparationResult::Pending;}
        return result;
    }
    case PreloadPhase::Verify:return preload_verify(p);
    default:return PreparationResult::Failed;
    }
    return PreparationResult::Failed;
}
