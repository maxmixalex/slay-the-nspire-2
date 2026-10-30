// NSP2/15 sparse tiles. Included in content.cpp's private namespace so the
// existing encoded scratch and image tiers remain the only picture buffers.
// The ordinary 6 MiB group tier shares the bounded 13 MiB picture arena.
// Merchant frames use a1 MiB group tier and retain the finished pictures;
// even that minimum fits a maximum index group plus a maximum tile group.
constexpr unsigned TileSide=16,TileBytes=770,TilePageBytes=131072,TilePages=48;
constexpr unsigned RetainedPageBytes=TilePageBytes/4;
constexpr unsigned MaxFrameTiles=300,MaxTileGroups=64;
// The offline access model uses the same contiguous first-fit allocation and
// oldest-unpinned LRU eviction. A frame's index group stays pinned throughout.
constexpr bool TileCacheLru=true;
struct TileGroup {
    const Content* owner{};
    uint32_t id{},size{},age{};
    uint16_t items{};
    // first: bit7 pins an index, bit6 selects retained32 KiB quarters,
    // bits0..5 locate the parent128 KiB page. Compact retained groups do not
    // have tile items: items bits0..1 hold the first quarter, bits2..3 the
    // quarter count minus one. The pixel arena and metadata stay unchanged.
    uint8_t first{},pages{};
};
static TileGroup tile_groups[MaxTileGroups];
static uint32_t retained_decodes=0;
static uint8_t* tile_pages=picture_pixels+ImagePages*ImagePageBytes;
static unsigned tile_page_limit=TilePages;
static uint64_t tile_used=0;
static uint32_t tile_age=0;
static_assert(TilePages<=64&&Lzma2BlockRawLimit/TilePageBytes<64,
              "Tile pages must fit the bitmap and packed first-page index");
static_assert(PictureBufferBudget-RoomImagePages*ImagePageBytes>=2u*Lzma2BlockRawLimit,
              "The smallest tier must hold a pinned index and one full tile group");
static_assert(sizeof tile_groups+sizeof tile_used+sizeof tile_age<=2048,
              "Tile-cache bookkeeping must remain below 2 KiB");
static_assert(MaxEncodedImage==Lzma2BlockEncodedLimit+32,
              "All codecs must share the existing bounded encoded scratch");

static uint64_t tile_mask(unsigned first,unsigned count){
    return ((uint64_t(1)<<count)-1)<<first;
}
static unsigned retained_quarters(unsigned page){
    unsigned used=0;
    for(const auto& group:tile_groups)
        if(group.pages&&(group.first&64u)&&(group.first&63u)==page)
            used|=((1u<<((group.items>>2)+1u))-1u)<<(group.items&3u);
    return used;
}
static uint8_t* tile_bytes(const TileGroup& group){
    return tile_pages+unsigned(group.first&63u)*TilePageBytes+
        ((group.first&64u)?unsigned(group.items&3u)*RetainedPageBytes:0);
}
static unsigned tile_capacity(const TileGroup& group){
    return (group.first&64u)?((group.items>>2)+1u)*RetainedPageBytes:group.pages*TilePageBytes;
}
static void tile_discard(TileGroup& group){
    if(group.pages){
        const unsigned first=group.first&63u;
        if(group.first&64u){
            group={};
            if(!retained_quarters(first))tile_used&=~tile_mask(first,1);
            return;
        }
        tile_used&=~tile_mask(first,group.pages);
    }
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
static TileGroup* reserve_tile_group(unsigned bytes,bool retained=false){
    if(!bytes||bytes>Lzma2BlockRawLimit)return nullptr;
    const unsigned quarters=(bytes+RetainedPageBytes-1u)/RetainedPageBytes;
    const bool compact=retained&&quarters<4u;
    const unsigned pages=(bytes+TilePageBytes-1)/TilePageBytes;
    // In the1 MiB tier, keep every allocation within one512 KiB bank.
    // Otherwise a pinned four-page index at pages2..5 could strand both
    // ends and prevent the next four-page tile from fitting. Power-of-two
    // alignment guarantees that the other bank is reclaimable; no copying
    // or movement of an existing/pinned group is needed.
    const unsigned step=tile_page_limit==8?(pages>2?4:pages==2?2:1):1;
    for(;;){
        unsigned first=tile_page_limit,quarter=0;
        if(compact){
            // Derive the few occupied parent pages from the existing entries.
            // There is no extra bitmap/arena, and full-size tile/index groups
            // still see these pages as occupied in the ordinary allocator.
            uint64_t parents=0;
            for(const auto& group:tile_groups)
                if(group.pages&&(group.first&64u))parents|=tile_mask(group.first&63u,1);
            for(unsigned i=0;i<tile_page_limit;++i)if(parents&tile_mask(i,1)){
                const unsigned used=retained_quarters(i);
                for(unsigned q=0;q+quarters<=4u;++q)
                    if(!(used&(((1u<<quarters)-1u)<<q))){first=i;quarter=q;break;}
                if(first<tile_page_limit)break;
            }
        }
        if(first==tile_page_limit)
            for(unsigned i=0;i+pages<=tile_page_limit;i+=step)
                if(!(tile_used&tile_mask(i,pages))){first=i;break;}
        if(first<tile_page_limit){
            for(auto& group:tile_groups)if(!group.pages){
                // Ownership is published only after a complete decode.
                group.first=uint8_t(first|(compact?64u:0u));
                group.pages=uint8_t(pages);group.items=uint16_t(compact?quarter|((quarters-1u)<<2):0);
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
    uint32_t images,uint32_t directory_capacity,bool prepared_costs,uint32_t& prefix){
    if(u32(h)!=0x314c4954u||u32(h+4)!=1||u32(h+8)!=TileSide||
       u32(h+12)!=Lzma2DictionaryProperty||u32(h+40)!=TileBytes||
       u32(h+48)!=Lzma2BlockRawLimit||u32(h+52)!=MaxEncodedImage)return false;
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
    if(prepared_costs){
        if(u32(h+56)!=prefix||u32(h+60)!=images||images>directory_capacity-prefix)return false;
        prefix+=(images+1u)&~1u;
    }
    return prefix<=length&&prefix<=directory_capacity&&descriptors>=prefix&&!(descriptors&1u)&&
        descriptors<=length&&frames<=(length-descriptors)/16;
}
static bool valid_tile_group_header(const uint8_t* h,uint32_t id,const uint8_t* p,unsigned length,unsigned version){
    const uint32_t tiles=u32(h+16),groups=u32(h+20);
    const unsigned size=u32(p+12),encoded=u32(p+16),items=u32(p+24);
    const bool is_tile=id<groups;
    const bool filtered=u32(p)==FilteredTileGroupMagic;
    const bool predicted=u32(p)==PredictiveIndexGroupMagic;
    // TGR1 never used its checksum/reserved words at runtime. Keep that legacy
    // behavior; only the distinct TGR2 magic gives word 7 functional meaning.
    if((predicted?(version<PredictiveIndexContentVersion||is_tile||u32(p+28)!=1u):
        filtered?(version<TileFilterContentVersion||!is_tile||u32(p+28)<1||u32(p+28)>2):
        (u32(p)!=0x31524754u))||u32(p+4)!=(is_tile?1u:2u)||u32(p+8)!=id||
       !size||size>Lzma2BlockRawLimit||encoded!=length-32)return false;
    if(is_tile){
        const unsigned per_group=u32(h+44),left=tiles-id*per_group;
        const unsigned expected=left<per_group?left:per_group;
        return items==expected&&size==items*TileBytes;
    }
    return true;
}
static TileGroup* load_tile_group(const Content* owner,const ResourceReader& reader,
    const uint8_t* h,uint32_t id,uint8_t*encoded_pixels){
    if(TileGroup* hit=find_tile_group(owner,id)){
        if(TileCacheLru)hit->age=next_tile_age();
        return hit;
    }
    if(id>=u32(h+20)+u32(h+24))return nullptr;
    const uint8_t* row=h+64+id*8;
    const uint32_t offset=u32(row),length=u32(row+4);
    if(!encoded_pixels||length<33||length>ResourceTransferCapacity||!reader.read(offset,encoded_pixels,length))return nullptr;
    const uint8_t* p=encoded_pixels;
    const unsigned size=u32(p+12),encoded=u32(p+16),items=u32(p+24);
    if(!valid_tile_group_header(h,id,p,length,owner->version()))return nullptr;
    const unsigned filter=u32(p)==FilteredTileGroupMagic?u32(p+28):0;
    const bool predicted=u32(p)==PredictiveIndexGroupMagic;
    TileGroup* group=reserve_tile_group(size);
    if(!group){cancel_content_preload();group=reserve_tile_group(size);}
    if(!group)return nullptr;
    if(!decode_lzma2_block(tile_bytes(*group),tile_capacity(*group),size,p+32,encoded,u32(h+12))){
        tile_discard(*group);return nullptr;
    }
    if(filter){
        // LZMA2 has consumed its input. Reuse the same transfer lease for the
        // reversible field shuffle; cache size and decoded group IDs stay exact.
        const uint8_t* decoded=tile_bytes(*group);
        for(unsigned i=0;i<size;++i)encoded_pixels[i]=decoded[i];
        if(!restore_tile_filter(tile_bytes(*group),encoded_pixels,size,items,filter)){
            tile_discard(*group);return nullptr;
        }
    }
    if(predicted&&!restore_index_prediction(tile_bytes(*group),size,items,u32(h+16))){
        tile_discard(*group);return nullptr;
    }
    group->id=id;group->size=size;group->items=uint16_t(items);group->age=next_tile_age();
    group->owner=owner;return group;
}

// Retained pictures share the existing animation-group pool and scratch.
// File offsets occupy a separate key namespace from the small tile-group ids.
static TileGroup* load_retained_group(const Content* owner,const ResourceReader& reader,
    uint32_t offset,uint32_t length,uint8_t*encoded_pixels){
    const uint32_t id=0x80000000u|offset;
    if(TileGroup* hit=find_tile_group(owner,id)){
        hit->age=next_tile_age();return hit;
    }
    if(!encoded_pixels||length<33||length>ResourceTransferCapacity||!reader.read(offset,encoded_pixels,length))return nullptr;
    const uint8_t* p=encoded_pixels;
    const unsigned size=u32(p+4),encoded=u32(p+8),property=u32(p+12);
    if(!size||size>Lzma2BlockRawLimit||property!=Lzma2DictionaryProperty)return nullptr;
    unsigned start=32,count=0,mode=0;
    if(u32(p)==FilteredRetainedGroupMagic){
        count=u32(p+16);mode=u32(p+20);const unsigned table_bytes=u32(p+24);
        if(owner->version()<RetainedFilterContentVersion||mode<1||mode>2||u32(p+28)||
           !count||count>(length-32)/8||table_bytes!=count*8||
           !valid_retained_slices(p+32,count,size))return nullptr;
        start+=table_bytes;
    }else if(u32(p)!=0x31524752u)return nullptr;
    if(!encoded||encoded!=length-start)return nullptr;
    TileGroup* group=reserve_tile_group(size,true);
    // The one-MiB room tier may need a whole512KiB bank. Visible demand may
    // reclaim a background reservation only when it actually prevents that
    // allocation, never merely because the actor advanced to another frame.
    if(!group){cancel_content_preload();group=reserve_tile_group(size,true);}
    if(!group)return nullptr;
    if(!decode_lzma2_block(tile_bytes(*group),tile_capacity(*group),size,p+start,encoded,property)||
       (mode&&!restore_retained_group(tile_bytes(*group),size,p+32,count,mode))){
        tile_discard(*group);return nullptr;
    }
    group->id=id;group->size=size;group->age=next_tile_age();
    group->owner=owner;++retained_decodes;return group;
}

static const uint8_t* tile_record_at(const TileGroup& group,unsigned offset,unsigned length){
    if(offset>group.size||length<12||length>group.size-offset)return nullptr;
    const uint8_t* record=tile_bytes(group)+offset;
    const unsigned output=u32(record),index=u32(record+4),geometry=u32(record+8);
    if(!output||output>MaxImageBytes||!index||index>MaxFrameTiles*4||index%4||
       geometry<2||index>length-12||geometry!=length-12-index)return nullptr;
    return record;
}
static bool tile_descriptor_slot(const uint8_t* h,uint32_t descriptor,unsigned& slot){
    const uint32_t first=u32(h+32),frames=u32(h+28);
    if(descriptor<first||descriptor-first>=frames*16||(descriptor-first)%16)return false;
    slot=(descriptor-first)/16;return true;
}
static bool valid_tile_descriptor(const uint8_t* h,uint32_t header_offset,const uint8_t* row){
    if(u32(row)!=header_offset||u32(row+4)>=u32(h+24))return false;
    const unsigned offset=u32(row+8),length=u32(row+12);
    return offset<=Lzma2BlockRawLimit&&length>=18&&length<=Lzma2BlockRawLimit-offset;
}
static bool read_tile_descriptor(const ResourceReader& reader,const uint8_t* h,
    uint32_t header_offset,uint32_t descriptor,uint8_t* row){
    return reader.read(descriptor,row,16)&&valid_tile_descriptor(h,header_offset,row);
}
struct TileFrame {
    TileGroup* pinned=nullptr;
    const uint8_t* ids=nullptr;
    const uint8_t* geometry=nullptr;
    unsigned output_size=0,geometry_size=0,count=0,columns=0;
    TileFrame()=default;
    TileFrame(const TileFrame&)=delete;
    TileFrame& operator=(const TileFrame&)=delete;
    ~TileFrame(){if(pinned)pinned->first&=127u;}
    bool open(const Content* owner,const ResourceReader& reader,const uint8_t* h,
        uint32_t header_offset,uint32_t descriptor,unsigned w,unsigned height,uint8_t*encoded_pixels){
        unsigned slot=0;if(!tile_descriptor_slot(h,descriptor,slot))return false;
        uint8_t row[16];if(!read_tile_descriptor(reader,h,header_offset,descriptor,row))return false;
        pinned=load_tile_group(owner,reader,h,u32(h+20)+u32(row+4),encoded_pixels);if(!pinned)return false;
        pinned->first|=128u;
        const uint8_t* record=tile_record_at(*pinned,u32(row+8),u32(row+12));if(!record)return false;
        columns=(w+TileSide-1)/TileSide;count=columns*((height+TileSide-1)/TileSide);
        if(u32(record+4)!=count*4)return false;
        output_size=u32(record);geometry_size=u32(record+8);
        ids=record+12;geometry=ids+count*4;
        return true;
    }
};

static bool rebuild_tile_frame(const Content* owner,const ResourceReader& reader,
    const uint8_t* h,const TileFrame& frame,unsigned w,unsigned height,
    uint8_t* output,uint8_t*encoded_pixels){
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
    for(unsigned i=0;i<frame.count;++i)if(active[i]){
        const unsigned tile=u32(frame.ids+i*4);if(tile>=u32(h+16))return false;
        const unsigned id=tile/per_group;unsigned position=0;
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
        TileGroup* group=load_tile_group(owner,reader,h,needed[g],encoded_pixels);if(!group)return false;
        const uint8_t* records=tile_bytes(*group);
        for(unsigned i=0;i<frame.count;++i)if(active[i]&&u32(frame.ids+i*4)/per_group==needed[g]){
            const unsigned id=u32(frame.ids+i*4)%per_group;
            if(id>=group->items)return false;
            const uint8_t* tile=records+id*TileBytes;
            const unsigned tx=(i%frame.columns)*TileSide,ty=(i/frame.columns)*TileSide;
            const unsigned tw=w-tx<TileSide?w-tx:TileSide,th=height-ty<TileSide?height-ty:TileSide;
            for(unsigned y=ty;y<ty+th;++y){
                unsigned ga=row_geometry[y],oa=row_output[y];const unsigned spans=u16(frame.geometry+ga);ga+=2;oa+=2;
                for(unsigned j=0;j<spans;++j){
                    const unsigned x=u16(frame.geometry+ga),count=u16(frame.geometry+ga+2),mode=frame.geometry[ga+4];
                    const unsigned stride=mode?3:2;ga+=5;oa+=5;
                    const unsigned begin=x>tx?x:tx,end=x+count<tx+tw?x+count:tx+tw;
                    for(unsigned pixel=begin;pixel<end;++pixel){
                        const unsigned pos=(y-ty)*TileSide+pixel-tx,destination=oa+(pixel-x)*stride;
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
