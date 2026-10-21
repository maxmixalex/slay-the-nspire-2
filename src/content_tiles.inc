// NSP2/15 sparse tiles. Included in content.cpp's private namespace so the
// existing encoded scratch and image tiers remain the only picture buffers.
constexpr unsigned TileSide=16,TileBytes=770,TilePageBytes=65536,TilePages=64;
constexpr unsigned MaxFrameTiles=300,MaxTileGroups=64;
// The offline access model uses the same contiguous first-fit allocation and
// oldest-unpinned LRU eviction. A frame's index group stays pinned throughout.
constexpr bool TileCacheLru=true;
struct TileGroup {
    const Content* owner{};
    uint32_t id{},size{},age{};
    uint16_t items{};
    uint8_t first{},pages{}; // first's high bit pins the entry; low six are a page.
};
static TileGroup tile_groups[MaxTileGroups];
alignas(4) static uint8_t tile_pages[TilePages*TilePageBytes];
static uint64_t tile_used=0;
static uint32_t tile_age=0;
static_assert(sizeof tile_groups+sizeof tile_used+sizeof tile_age<=2048,
              "Tile-cache bookkeeping must remain below 2 KiB");
static_assert(MaxEncodedImage==Lzma2BlockEncodedLimit+32,
              "All codecs must share the existing bounded encoded scratch");

static uint64_t tile_mask(unsigned first,unsigned count){
    return ((uint64_t(1)<<count)-1)<<first;
}
static uint8_t* tile_bytes(const TileGroup& group){
    return tile_pages+unsigned(group.first&63u)*TilePageBytes;
}
static void tile_discard(TileGroup& group){
    if(group.pages)tile_used&=~tile_mask(group.first&63u,group.pages);
    group={};
}
static void clear_tile_cache(const Content* owner){
    for(auto& group:tile_groups)if(group.owner==owner)tile_discard(group);
}
static uint32_t next_tile_age(){
    if(tile_age==0xffffffffu){
        // Preserve relative recency on the rare counter wrap, without a
        // second timestamp word in every cache entry.
        uint8_t order[MaxTileGroups];unsigned count=0;
        for(unsigned i=0;i<MaxTileGroups;++i)if(tile_groups[i].owner){
            unsigned at=count;
            while(at&&tile_groups[order[at-1]].age>tile_groups[i].age){order[at]=order[at-1];--at;}
            order[at]=uint8_t(i);++count;
        }
        for(unsigned i=0;i<count;++i)tile_groups[order[i]].age=i+1;
        tile_age=count;
    }
    return ++tile_age;
}
static TileGroup* find_tile_group(const Content* owner,uint32_t id){
    for(auto& group:tile_groups)if(group.owner==owner&&group.id==id)return &group;
    return nullptr;
}
static TileGroup* reserve_tile_group(unsigned bytes){
    const unsigned pages=(bytes+TilePageBytes-1)/TilePageBytes;
    for(;;){
        unsigned first=TilePages;
        for(unsigned i=0;i+pages<=TilePages;++i)if(!(tile_used&tile_mask(i,pages))){first=i;break;}
        if(first<TilePages){
            for(auto& group:tile_groups)if(!group.pages){
                // Ownership is published only after decoding and validation.
                group.first=uint8_t(first);group.pages=uint8_t(pages);
                tile_used|=tile_mask(first,pages);return &group;
            }
        }
        TileGroup* victim=nullptr;
        for(auto& group:tile_groups)if(group.pages&&!(group.first&128u)&&
            (!victim||group.age<victim->age))victim=&group;
        if(!victim)return nullptr;
        tile_discard(*victim);
    }
}

static bool tile_header_bounds(const uint8_t* h,uint32_t base,uint32_t length,
    uint32_t images,uint32_t directory_capacity,uint32_t& prefix){
    if(u32(h)!=0x314c4954u||u32(h+4)!=1||u32(h+8)!=TileSide||
       u32(h+12)!=Lzma2DictionaryProperty||u32(h+40)!=TileBytes||
       u32(h+48)!=Lzma2BlockRawLimit||u32(h+52)!=MaxEncodedImage||
       u32(h+56)||u32(h+60))return false;
    const uint32_t tiles=u32(h+16),groups=u32(h+20),indexes=u32(h+24);
    const uint32_t frames=u32(h+28),descriptors=u32(h+32),per_group=u32(h+44);
    if(!tiles||tiles>65535u*MaxFrameTiles||!per_group||per_group>680||
       groups!=(tiles+per_group-1)/per_group||!frames||frames>images||
       !indexes||indexes>frames||base>length||length-base<64||
       base>directory_capacity||directory_capacity-base<64)return false;
    const uint32_t table=base+64;
    if(u32(h+36)!=table||groups>(directory_capacity-table)/8||
       indexes>(directory_capacity-table)/8-groups)return false;
    prefix=table+(groups+indexes)*8;
    return prefix<=length&&descriptors>=prefix&&!(descriptors&1u)&&
        descriptors<=length&&frames<=(length-descriptors)/16;
}
static bool valid_tile_directory(const uint8_t* h,uint32_t length){
    const uint32_t count=u32(h+20)+u32(h+24);
    uint32_t end=u32(h+32)+16*u32(h+28);
    for(uint32_t i=0;i<count;++i){
        const uint8_t* row=h+64+i*8;
        const uint32_t offset=u32(row),bytes=u32(row+4);
        if((offset&1u)||offset<end||offset>length||bytes<33||
           bytes>MaxEncodedImage||bytes>length-offset)return false;
        end=offset+bytes;
    }
    return true;
}
static bool valid_tile_records(const uint8_t* bytes,unsigned items){
    for(unsigned i=0;i<items;++i){
        const uint8_t* p=bytes+i*TileBytes;
        const unsigned w=p[0],h=p[1];if(!w||w>TileSide||!h||h>TileSide)return false;
        for(unsigned y=0;y<TileSide;++y)for(unsigned x=y<h?w:0;x<TileSide;++x){
            const unsigned at=y*TileSide+x;
            if(p[2+at*2]||p[3+at*2]||p[514+at])return false;
        }
    }
    return true;
}
static bool valid_tile_geometry(const uint8_t* geometry,unsigned length,unsigned output,
    unsigned width=320,unsigned height=0){
    unsigned at=0,out=0,rows=0;
    while(at<length){
        if(rows>=240||(height&&rows>=height)||length-at<2||output-out<2)return false;
        const unsigned spans=u16(geometry+at);at+=2;out+=2;++rows;
        if(spans>width)return false;
        unsigned last=0;
        for(unsigned i=0;i<spans;++i){
            if(length-at<5||output-out<5)return false;
            const unsigned x=u16(geometry+at),count=u16(geometry+at+2),mode=geometry[at+4];
            if(!count||mode>1||x<last||x+count>width)return false;
            last=x+count;at+=5;out+=5;
            const unsigned bytes=count*(mode?3u:2u);if(bytes>output-out)return false;
            out+=bytes;
        }
    }
    return rows&&(!height||rows==height)&&out==output;
}
static bool valid_tile_indexes(const uint8_t* bytes,unsigned size,unsigned items,unsigned tiles){
    unsigned at=0;
    for(unsigned i=0;i<items;++i){
        if(size-at<12)return false;
        const unsigned output=u32(bytes+at),index=u32(bytes+at+4),geometry=u32(bytes+at+8);
        if(!output||output>MaxImageBytes||!index||index>MaxFrameTiles*4||
           index%4||geometry<2||index>size-at-12||geometry>size-at-12-index)return false;
        for(unsigned j=0;j<index;j+=4)if(u32(bytes+at+12+j)>=tiles)return false;
        if(!valid_tile_geometry(bytes+at+12+index,geometry,output))return false;
        at+=12+index+geometry;
    }
    return at==size;
}
static bool valid_tile_group_header(const uint8_t* h,uint32_t id,const uint8_t* p,unsigned length){
    const uint32_t tiles=u32(h+16),groups=u32(h+20);
    const unsigned size=u32(p+12),encoded=u32(p+16),items=u32(p+24);
    const bool is_tile=id<groups;
    if(u32(p)!=0x31524754u||u32(p+4)!=(is_tile?1u:2u)||u32(p+8)!=id||
       !size||size>Lzma2BlockRawLimit||encoded!=length-32||u32(p+28))return false;
    if(is_tile){
        const unsigned per_group=u32(h+44),left=tiles-id*per_group;
        const unsigned expected=left<per_group?left:per_group;
        return items==expected&&size==items*TileBytes;
    }
    return items&&items<=64;
}
static TileGroup* load_tile_group(const Content* owner,const ResourceReader& reader,
    const uint8_t* h,uint32_t id){
    if(TileGroup* hit=find_tile_group(owner,id)){
        if(TileCacheLru)hit->age=next_tile_age();
        return hit;
    }
    const uint32_t tile_group_count=u32(h+20);
    if(id>=tile_group_count+u32(h+24))return nullptr;
    const uint8_t* row=h+64+id*8;
    const uint32_t offset=u32(row),length=u32(row+4);
    if(length<33||length>sizeof encoded_pixels||!reader.read(offset,encoded_pixels,length))return nullptr;
    const uint8_t* p=encoded_pixels;
    const unsigned size=u32(p+12),encoded=u32(p+16),items=u32(p+24);
    const bool is_tile=id<tile_group_count;
    if(!valid_tile_group_header(h,id,p,length))return nullptr;
    TileGroup* group=reserve_tile_group(size);if(!group)return nullptr;
    uint8_t* output=tile_bytes(*group);
    if(!decode_lzma2_block(output,group->pages*TilePageBytes,size,p+32,encoded,
            u32(h+12),u32(p+20))||
       !(is_tile?valid_tile_records(output,items):valid_tile_indexes(output,size,items,u32(h+16)))){
        tile_discard(*group);return nullptr;
    }
    group->id=id;group->size=size;group->items=uint16_t(items);group->age=next_tile_age();
    group->owner=owner;return group;
}

static const uint8_t* tile_record_at(const TileGroup& group,unsigned offset,unsigned length){
    if(offset>group.size||length>group.size-offset)return nullptr;
    const uint8_t* bytes=tile_bytes(group);unsigned at=0;
    for(unsigned i=0;i<group.items&&at<offset;++i)
        at+=12+u32(bytes+at+4)+u32(bytes+at+8);
    if(at!=offset||group.size-at<12||length!=12+u32(bytes+at+4)+u32(bytes+at+8))return nullptr;
    return bytes+at;
}
static bool tile_descriptor_slot(const uint8_t* h,uint32_t descriptor,unsigned& slot){
    const uint32_t first=u32(h+32),frames=u32(h+28);
    if(descriptor<first||descriptor-first>=frames*16||(descriptor-first)%16)return false;
    slot=(descriptor-first)/16;return true;
}
static bool read_tile_descriptor(const ResourceReader& reader,const uint8_t* h,
    uint32_t header_offset,uint32_t descriptor,uint8_t* row){
    if(!reader.read(descriptor,row,16)||u32(row)!=header_offset||u32(row+4)>=u32(h+24))return false;
    const unsigned offset=u32(row+8),length=u32(row+12);
    return offset<=Lzma2BlockRawLimit&&length>=18&&length<=Lzma2BlockRawLimit-offset;
}
static bool valid_tile_shape(const uint8_t* record,unsigned width,unsigned height){
    const unsigned index=u32(record+4);
    return index==4*((width+TileSide-1)/TileSide)*((height+TileSide-1)/TileSide)&&
        valid_tile_geometry(record+12+index,u32(record+8),u32(record),width,height);
}

struct TileFrame {
    TileGroup* pinned=nullptr;
    const uint8_t* ids=nullptr;
    const uint8_t* geometry=nullptr;
    unsigned output_size=0,geometry_size=0,count=0,columns=0;
    TileFrame()=default;
    TileFrame(const TileFrame&)=delete;
    TileFrame& operator=(const TileFrame&)=delete;
    ~TileFrame(){if(pinned)pinned->first&=63u;}
    bool open(const Content* owner,const ResourceReader& reader,const uint8_t* h,
        uint32_t header_offset,uint32_t descriptor,unsigned w,unsigned height){
        unsigned slot=0;if(!tile_descriptor_slot(h,descriptor,slot))return false;
        uint8_t row[16];if(!read_tile_descriptor(reader,h,header_offset,descriptor,row))return false;
        pinned=load_tile_group(owner,reader,h,u32(h+20)+u32(row+4));if(!pinned)return false;
        pinned->first|=128u;
        const uint8_t* record=tile_record_at(*pinned,u32(row+8),u32(row+12));if(!record)return false;
        columns=(w+TileSide-1)/TileSide;count=columns*((height+TileSide-1)/TileSide);
        if(u32(record+4)!=count*4)return false;
        output_size=u32(record);geometry_size=u32(record+8);
        ids=record+12;geometry=ids+count*4;
        for(unsigned i=0;i<count;++i)if(u32(ids+i*4)>=u32(h+16))return false;
        return true;
    }
};

static void tile_put32(uint8_t* out,uint32_t value){
    for(unsigned i=0;i<4;++i)out[i]=uint8_t(value>>(8*i));
}
// Startup validates every index stream and descriptor, but leaves expensive
// tile pixel streams for the first image that uses them. Borrow the unused
// resident-directory tail for group lists; neither a second index cache nor
// a permanent per-descriptor allocation is needed.
static bool validate_tile_catalog(const Content* owner,const ResourceReader& reader,
    const uint8_t* h,uint32_t header_offset,const uint8_t* images,unsigned image_count,
    uint8_t* workspace,unsigned capacity){
    const unsigned descriptors=u32(h+28),indexes=u32(h+24),tile_count=u32(h+20);
    const unsigned required=4*(2*descriptors+indexes);
    bool grouped=capacity>=required;
    uint8_t* shapes=workspace;
    uint8_t* next=grouped?shapes+descriptors*4:nullptr;
    uint8_t* heads=grouped?next+descriptors*4:nullptr;
    if(grouped){
        for(unsigned i=0;i<descriptors*4;++i)shapes[i]=0;
        for(unsigned i=0;i<indexes;++i)tile_put32(heads+i*4,0xffffffffu);
    }
    for(unsigned i=0;i<image_count;++i){
        const uint8_t* image=images+i*16;if(u32(image+8)!=6)continue;
        unsigned slot=0;if(!tile_descriptor_slot(h,u32(image),slot))return false;
        if(grouped){
            const unsigned shape=u32(image+4),old=u32(shapes+slot*4);
            // Conflicting image shapes require independent structural checks;
            // do not let one alias authorize another image's later tile edges.
            if(old&&old!=shape)grouped=false;
            else tile_put32(shapes+slot*4,shape);
        }
    }
    for(unsigned i=0;i<descriptors;++i){
        uint8_t row[16];if(!read_tile_descriptor(reader,h,header_offset,u32(h+32)+i*16,row))return false;
        if(grouped){
            const unsigned group=u32(row+4);tile_put32(next+i*4,u32(heads+group*4));tile_put32(heads+group*4,i);
        }
    }
    for(unsigned id=0;id<tile_count+indexes;++id){
        if(id<tile_count){
            const uint8_t* lookup=h+64+id*8;uint8_t row[32];
            if(!reader.read(u32(lookup),row,sizeof row)||!valid_tile_group_header(h,id,row,u32(lookup+4)))return false;
            continue;
        }
        TileFrame pin;pin.pinned=load_tile_group(owner,reader,h,id);if(!pin.pinned)return false;
        pin.pinned->first|=128u;
        if(grouped)for(unsigned descriptor=u32(heads+(id-tile_count)*4);descriptor!=0xffffffffu;descriptor=u32(next+descriptor*4)){
            uint8_t row[16];
            if(!read_tile_descriptor(reader,h,header_offset,u32(h+32)+descriptor*16,row))return false;
            const uint8_t* record=tile_record_at(*pin.pinned,u32(row+8),u32(row+12));if(!record)return false;
            const unsigned shape=u32(shapes+descriptor*4);
            if(shape&&!valid_tile_shape(record,shape&65535u,shape>>16))return false;
        }
    }
    if(grouped)return true;
    // A legal directory may consume the entire 2 MiB workspace. Preserve
    // acceptance and bounded memory with sequential checks in that case;
    // eviction can cause additional index decoding, but no tile pixel reads.
    for(unsigned i=0;i<descriptors;++i){
        uint8_t row[16];if(!read_tile_descriptor(reader,h,header_offset,u32(h+32)+i*16,row))return false;
        TileGroup* group=load_tile_group(owner,reader,h,tile_count+u32(row+4));
        if(!group||!tile_record_at(*group,u32(row+8),u32(row+12)))return false;
    }
    for(unsigned i=0;i<image_count;++i){
        const uint8_t* image=images+i*16;if(u32(image+8)!=6)continue;
        TileFrame frame;
        if(!frame.open(owner,reader,h,header_offset,u32(image),u16(image+4),u16(image+6))||
           !valid_tile_geometry(frame.geometry,frame.geometry_size,frame.output_size,u16(image+4),u16(image+6)))return false;
    }
    return true;
}

static bool rebuild_tile_frame(const Content* owner,const ResourceReader& reader,
    const uint8_t* h,const TileFrame& frame,unsigned w,unsigned height,
    bool validate_all,uint8_t* output){
    unsigned row_geometry[240],row_output[240];
    uint8_t active[MaxFrameTiles]{};
    unsigned at=0,out=0;
    for(unsigned y=0;y<height;++y){
        if(frame.geometry_size-at<2||frame.output_size-out<2)return false;
        row_geometry[y]=at;row_output[y]=out;
        const unsigned spans=u16(frame.geometry+at);if(spans>w)return false;
        output[out++]=frame.geometry[at++];output[out++]=frame.geometry[at++];unsigned last=0;
        for(unsigned j=0;j<spans;++j){
            if(frame.geometry_size-at<5||frame.output_size-out<5)return false;
            const unsigned x=u16(frame.geometry+at),count=u16(frame.geometry+at+2),mode=frame.geometry[at+4];
            if(!count||mode>1||x<last||x+count>w)return false;
            last=x+count;
            for(unsigned i=0;i<5;++i)output[out++]=frame.geometry[at++];
            const unsigned bytes=count*(mode?3u:2u);if(bytes>frame.output_size-out)return false;
            out+=bytes;
            for(unsigned tx=x/TileSide;tx<=(last-1)/TileSide;++tx)
                active[(y/TileSide)*frame.columns+tx]=1;
        }
    }
    if(at!=frame.geometry_size||out!=frame.output_size)return false;
    unsigned needed[MaxFrameTiles],needed_count=0;uint8_t cached[MaxFrameTiles];
    const unsigned per_group=u32(h+44);
    for(unsigned i=0;i<frame.count;++i)if(validate_all||active[i]){
        const unsigned id=u32(frame.ids+i*4)/per_group;unsigned position=0;
        while(position<needed_count&&needed[position]<id)++position;
        if(position<needed_count&&needed[position]==id)continue;
        for(unsigned j=needed_count;j>position;--j)needed[j]=needed[j-1];
        needed[position]=id;++needed_count;
    }
    // Snapshot hits after pinning the index. Consume those first so loading a
    // missing group cannot evict another group still needed by this frame.
    for(unsigned i=0;i<needed_count;++i)cached[i]=find_tile_group(owner,needed[i])?1:0;
    unsigned copied=0;
    for(unsigned pass=0;pass<2;++pass)for(unsigned g=0;g<needed_count;++g){
        if(cached[g]!=(pass==0?1:0))continue;
        TileGroup* group=load_tile_group(owner,reader,h,needed[g]);if(!group)return false;
        const uint8_t* records=tile_bytes(*group);
        for(unsigned i=0;i<frame.count;++i)if((validate_all||active[i])&&u32(frame.ids+i*4)/per_group==needed[g]){
            const unsigned id=u32(frame.ids+i*4)%per_group;
            if(id>=group->items)return false;
            const uint8_t* tile=records+id*TileBytes;
            const unsigned tx=(i%frame.columns)*TileSide,ty=(i/frame.columns)*TileSide;
            const unsigned tw=w-tx<TileSide?w-tx:TileSide,th=height-ty<TileSide?height-ty:TileSide;
            if(tile[0]!=tw||tile[1]!=th)return false;
            if(!active[i])continue;
            for(unsigned y=ty;y<ty+th;++y){
                unsigned ga=row_geometry[y],oa=row_output[y];const unsigned spans=u16(frame.geometry+ga);ga+=2;oa+=2;
                for(unsigned j=0;j<spans;++j){
                    const unsigned x=u16(frame.geometry+ga),count=u16(frame.geometry+ga+2),mode=frame.geometry[ga+4];
                    const unsigned stride=mode?3:2;ga+=5;oa+=5;
                    const unsigned begin=x>tx?x:tx,end=x+count<tx+tw?x+count:tx+tw;
                    for(unsigned pixel=begin;pixel<end;++pixel){
                        const unsigned pos=(y-ty)*TileSide+pixel-tx,destination=oa+(pixel-x)*stride;
                        if(!mode&&tile[514+pos]!=255)return false;
                        output[destination]=tile[2+pos*2];output[destination+1]=tile[3+pos*2];
                        if(mode)output[destination+2]=tile[514+pos];copied+=stride;
                    }
                    oa+=count*stride;
                }
            }
        }
    }
    return copied+frame.geometry_size==frame.output_size;
}
