// Incremental tiled pictures retain all unpublished bytes in the existing
// picture arenas. No view points into the shared per-call transfer lease.
// A 64 KiB retained reservation contains the LZMA probabilities, one bounded
// input chunk and geometry bookkeeping; the completed cache layout is unchanged.
struct TilePreloadPlan {
    uint32_t ids[MaxFrameTiles],row_output[240],needed[MaxFrameTiles];
    uint8_t active[MaxFrameTiles],cached[MaxFrameTiles];
};
constexpr unsigned TilePreloadPlanOffset=Lzma2ProbabilityBytes+TilePreloadInputBytes;
static_assert(TilePreloadPlanOffset+sizeof(TilePreloadPlan)<=2u*RetainedPageBytes,
              "Tiled preparation metadata must use its existing arena reservation");
static TilePreloadPlan& preload_tile_plan(ContentPreload&p){
    return *reinterpret_cast<TilePreloadPlan*>(tile_bytes(*p.workspace)+TilePreloadPlanOffset);
}
static const uint8_t* preload_tile_record(const ContentPreload&p){
    return tile_record_at(*p.index,p.tile_record_offset,p.tile_record_length);
}
static PreparationResult preload_tile_predict(ContentPreload&p){
    using index_prediction_detail::word;
    auto*data=tile_bytes(*p.raw);const unsigned bytes=p.raw->size,tiles=u32(p.tile_header+16);
    if(!p.filter_count||p.filter_count>64u)return PreparationResult::Failed;
    if(p.filter_index==p.filter_count){
        if(p.scan!=bytes)return PreparationResult::Failed;
        p.phase=PreloadPhase::TileReady;return PreparationResult::Pending;
    }
    if(p.scan>bytes||bytes-p.scan<12u)return PreparationResult::Failed;
    auto*record=data+p.scan;
    const unsigned decoded=word(record),index_bytes=word(record+4),geometry=word(record+8);
    if(!decoded||decoded>MaxImageBytes||!index_bytes||index_bytes>MaxFrameTiles*4u||(index_bytes&3u)||
       !geometry||geometry>decoded||index_bytes>bytes-p.scan-12u||
       geometry>bytes-p.scan-12u-index_bytes)return PreparationResult::Failed;
    auto*indices=record+12;const unsigned count=index_bytes/4u;uint32_t spatial=0;
    for(unsigned i=0;i<count;++i){
        auto*at=indices+i*4u;const uint32_t encoded=word(at),zigzag=encoded>>1;
        if(zigzag>=tiles*2u)return PreparationResult::Failed;
        const uint32_t base=(encoded&1u)?(p.previous==count?word(data+p.slice_offset+i*4u):0u):spatial;
        const uint32_t delta=(zigzag>>1)^(0u-(zigzag&1u)),id=base+delta;
        if(id>=tiles)return PreparationResult::Failed;
        word(at,id);spatial=id;
    }
    p.slice_offset=p.scan+12;p.previous=count;p.scan+=12+index_bytes+geometry;++p.filter_index;
    return PreparationResult::Pending;
}
static PreparationResult preload_tile_filter(ContentPreload&p){
    if(!p.restored){
        const unsigned planes=tile_filter_plane_bytes(p.filter_count);
        p.restored=reserve_tile_group(planes,true);if(!p.restored)return PreparationResult::Failed;
        p.restored->first|=128u;p.restored->size=planes;p.position=0;
    }
    if(p.position<p.restored->size){
        const auto*input=tile_bytes(*p.raw);auto*output=tile_bytes(*p.restored);
        const unsigned headers=p.filter_count*2u,left=p.restored->size-p.position;
        const unsigned count=left<TilePreloadOutputBytes?left:TilePreloadOutputBytes;
        for(unsigned i=0;i<count;++i){const unsigned at=p.position+i;
            output[at]=input[at<headers?at:p.filter_count*514u+at-headers];}
        p.position+=count;return PreparationResult::Pending;
    }
    if(p.filter_index==p.filter_count){
        tile_discard(*p.restored);p.restored=nullptr;
        p.phase=PreloadPhase::TileReady;return PreparationResult::Pending;
    }
    if(!restore_tile_filter_in_place_step(tile_bytes(*p.raw),tile_bytes(*p.restored),
        p.filter_count,p.filter_mode,p.filter_index++))return PreparationResult::Failed;
    return PreparationResult::Pending;
}
static PreparationResult preload_tile_step(ContentPreload&p){
    const auto*h=p.tile_header;if(!h)return PreparationResult::Failed;
    switch(p.phase){
    case PreloadPhase::TileDescriptor:{
        unsigned slot=0;uint8_t row[16];
        if(!tile_descriptor_slot(h,p.descriptor,slot)||
           !p.reader->read_bounded(p.descriptor,row,sizeof row)||
           !valid_tile_descriptor(h,p.tile_header_offset,row))return PreparationResult::Failed;
        if(!preload_workspace(p))return PreparationResult::Failed;
        ::new(detail::InPlaceAddress{tile_bytes(*p.workspace)+TilePreloadPlanOffset}) TilePreloadPlan;
        p.tile_record_offset=u32(row+8);p.tile_record_length=u32(row+12);
        p.tile_id=u32(h+20)+u32(row+4);p.phase=PreloadPhase::TileGroup;return PreparationResult::Pending;
    }
    case PreloadPhase::TileGroup:{
        if(auto*hit=find_tile_group(p.owner,p.tile_id)){
            if(TileCacheLru)hit->age=next_tile_age();hit->first|=128u;
            if(p.tile_id>=u32(h+20)){p.index=hit;p.phase=PreloadPhase::TilePlan;}
            else {p.raw=hit;p.phase=PreloadPhase::TileCopy;}
            return PreparationResult::Pending;
        }
        if(p.tile_id>=u32(h+20)+u32(h+24))return PreparationResult::Failed;
        const auto*row=h+64+p.tile_id*8u;
        p.group_offset=u32(row);p.group_length=u32(row+4);
        if(p.group_length<33||p.group_length>ResourceTransferCapacity)return PreparationResult::Failed;
        const unsigned first=p.group_length<TilePreloadInputBytes?p.group_length:TilePreloadInputBytes;
        auto*input=preload_input(p);
        // The first requested chunk includes the header, avoiding a separate
        // cold 32-byte read followed by rereading the same first input page.
        if(!p.reader->read(p.group_offset,input,first)||
           !valid_tile_group_header(h,p.tile_id,input,p.group_length,p.owner->version()))return PreparationResult::Failed;
        const unsigned size=u32(input+12),items=u32(input+24);
        p.raw=reserve_tile_group(size);if(!p.raw)return PreparationResult::Failed;
        p.raw->first|=128u;p.raw->size=size;p.raw->items=uint16_t(items);
        p.filter_mode=u32(input)==PredictiveIndexGroupMagic?3u:u32(input)==FilteredTileGroupMagic?uint8_t(u32(input+28)):0u;
        p.filter_count=items;p.filter_index=p.scan=p.previous=p.slice_offset=0;
        p.source_offset=p.group_offset+32;p.source_size=p.group_length-32;p.loaded=first-32;
        p.input_at=32;p.input_size=uint16_t(first);
        if(!begin_lzma2_stream(p.stream,tile_bytes(*p.raw),tile_capacity(*p.raw),size,p.source_size,u32(h+12),
                              tile_bytes(*p.workspace),Lzma2ProbabilityBytes))return PreparationResult::Failed;
        p.phase=PreloadPhase::TileDecode;return PreparationResult::Pending;
    }
    case PreloadPhase::TileDecode:{
        if(p.input_at==p.input_size&&p.loaded<p.source_size)
            return preload_fill(p)?PreparationResult::Pending:PreparationResult::Failed;
        uint32_t consumed=0;
        const auto result=step_lzma2_stream(p.stream,preload_input(p)+p.input_at,p.input_size-p.input_at,consumed,TilePreloadOutputBytes);
        p.input_at=uint16_t(p.input_at+consumed);
        if(result==Lzma2StepResult::Failed)return PreparationResult::Failed;
        if(result==Lzma2StepResult::Ready)p.phase=p.filter_mode==3?PreloadPhase::TilePredict:
            p.filter_mode?PreloadPhase::TileFilter:PreloadPhase::TileReady;
        return PreparationResult::Pending;
    }
    case PreloadPhase::TilePredict:return preload_tile_predict(p);
    case PreloadPhase::TileFilter:return preload_tile_filter(p);
    case PreloadPhase::TileReady:
        p.raw->id=p.tile_id;p.raw->age=next_tile_age();p.raw->owner=p.owner;
        if(p.tile_id>=u32(h+20)){p.index=p.raw;p.raw=nullptr;p.phase=PreloadPhase::TilePlan;}
        else p.phase=PreloadPhase::TileCopy;
        return PreparationResult::Pending;
    case PreloadPhase::TilePlan:{
        const auto*record=preload_tile_record(p);if(!record)return PreparationResult::Failed;
        p.tile_columns=uint16_t((p.w+TileSide-1)/TileSide);
        p.tile_count=uint16_t(p.tile_columns*((p.h+TileSide-1)/TileSide));
        if(p.tile_count>MaxFrameTiles||u32(record+4)!=p.tile_count*4u)return PreparationResult::Failed;
        p.size=u32(record);p.tile_geometry=u32(record+8);p.format=4;
        if(!preload_reserve_output(p))return PreparationResult::Failed;
        auto&plan=preload_tile_plan(p);for(unsigned i=0;i<MaxFrameTiles;++i){
            plan.active[i]=plan.cached[i]=0;plan.ids[i]=i<p.tile_count?u32(record+12+i*4u):0u;}
        p.scan=p.position=p.rows=p.tile_copied=0;p.phase=PreloadPhase::TileGeometry;return PreparationResult::Pending;
    }
    case PreloadPhase::TileGeometry:{
        if(p.rows==p.h){
            if(p.scan!=p.tile_geometry||p.position!=p.size)return PreparationResult::Failed;
            // IDs are in the bounded plan; all geometry is already at its
            // final output positions. No later stage needs the source index.
            p.index->first&=127u;p.index=nullptr;
            p.tile_slot=p.tile_needed=0;p.phase=PreloadPhase::TileNeeds;return PreparationResult::Pending;
        }
        const auto*record=preload_tile_record(p);if(!record)return PreparationResult::Failed;
        const auto*geometry=record+12+p.tile_count*4u;auto*out=preload_output(p);auto&plan=preload_tile_plan(p);
        const unsigned y=p.rows++;plan.row_output[y]=p.position;
        if(p.tile_geometry-p.scan<2||p.size-p.position<2)return PreparationResult::Failed;
        const unsigned spans=u16(geometry+p.scan);if(spans>p.w)return PreparationResult::Failed;
        out[p.position++]=geometry[p.scan++];out[p.position++]=geometry[p.scan++];unsigned last=0;
        for(unsigned j=0;j<spans;++j){
            if(p.tile_geometry-p.scan<5||p.size-p.position<5)return PreparationResult::Failed;
            const unsigned x=u16(geometry+p.scan),count=u16(geometry+p.scan+2),mode=geometry[p.scan+4];
            if(!count||mode>1||x<last||x+count>p.w)return PreparationResult::Failed;
            last=x+count;for(unsigned k=0;k<5;++k)out[p.position++]=geometry[p.scan++];
            const unsigned bytes=count*(mode?3u:2u);if(bytes>p.size-p.position)return PreparationResult::Failed;
            p.position+=bytes;for(unsigned tx=x/TileSide;tx<=(last-1)/TileSide;++tx)plan.active[(y/TileSide)*p.tile_columns+tx]=1;
        }
        return PreparationResult::Pending;
    }
    case PreloadPhase::TileNeeds:{
        auto&plan=preload_tile_plan(p);const unsigned stop=p.tile_slot+32<p.tile_count?p.tile_slot+32:p.tile_count;
        for(;p.tile_slot<stop;++p.tile_slot)if(plan.active[p.tile_slot]){
            const unsigned tile=plan.ids[p.tile_slot];if(tile>=u32(h+16))return PreparationResult::Failed;
            const unsigned id=tile/u32(h+44);unsigned at=0;
            while(at<p.tile_needed&&plan.needed[at]<id)++at;
            if(at<p.tile_needed&&plan.needed[at]==id)continue;
            for(unsigned j=p.tile_needed;j>at;--j)plan.needed[j]=plan.needed[j-1];
            plan.needed[at]=id;++p.tile_needed;
        }
        if(p.tile_slot==p.tile_count){
            for(unsigned i=0;i<p.tile_needed;++i)plan.cached[i]=find_tile_group(p.owner,plan.needed[i])?1:0;
            p.tile_next=p.tile_slot=p.tile_pass=0;p.phase=PreloadPhase::TileSelect;
        }
        return PreparationResult::Pending;
    }
    case PreloadPhase::TileSelect:{
        auto&plan=preload_tile_plan(p);
        for(;;){
            while(p.tile_next<p.tile_needed&&plan.cached[p.tile_next]!=(p.tile_pass?0:1))++p.tile_next;
            if(p.tile_next<p.tile_needed){p.tile_id=plan.needed[p.tile_next];p.tile_slot=0;p.phase=PreloadPhase::TileGroup;break;}
            if(!p.tile_pass){p.tile_pass=1;p.tile_next=0;continue;}
            if(p.tile_copied+p.tile_geometry!=p.size)return PreparationResult::Failed;
            p.scan=p.spans=p.rows=p.previous=0;p.phase=PreloadPhase::Verify;break;
        }
        return PreparationResult::Pending;
    }
    case PreloadPhase::TileCopy:{
        if(!p.raw)return PreparationResult::Failed;auto&plan=preload_tile_plan(p);
        const unsigned per_group=u32(h+44);
        while(p.tile_slot<p.tile_count&&(!plan.active[p.tile_slot]||plan.ids[p.tile_slot]/per_group!=p.tile_id))++p.tile_slot;
        if(p.tile_slot==p.tile_count){p.raw->first&=127u;p.raw=nullptr;++p.tile_next;p.phase=PreloadPhase::TileSelect;return PreparationResult::Pending;}
        const unsigned i=p.tile_slot++,id=plan.ids[i]%per_group;
        if(id>=p.raw->items)return PreparationResult::Failed;
        const auto*tile=tile_bytes(*p.raw)+id*TileBytes;auto*out=preload_output(p);
        const unsigned tx=(i%p.tile_columns)*TileSide,ty=(i/p.tile_columns)*TileSide;
        const unsigned tw=p.w-tx<TileSide?p.w-tx:TileSide,th=p.h-ty<TileSide?p.h-ty:TileSide;
        for(unsigned y=ty;y<ty+th;++y){
            unsigned oa=plan.row_output[y];const unsigned spans=u16(out+oa);oa+=2;
            for(unsigned j=0;j<spans;++j){
                const unsigned x=u16(out+oa),count=u16(out+oa+2),mode=out[oa+4],stride=mode?3:2;
                if(x>=tx+tw)break;
                oa+=5;const unsigned begin=x>tx?x:tx,end=x+count<tx+tw?x+count:tx+tw;
                for(unsigned pixel=begin;pixel<end;++pixel){
                    const unsigned pos=(y-ty)*TileSide+pixel-tx,destination=oa+(pixel-x)*stride;
                    out[destination]=tile[2+pos*2];out[destination+1]=tile[3+pos*2];
                    if(mode)out[destination+2]=tile[514+pos];p.tile_copied+=stride;
                }
                oa+=count*stride;
            }
        }
        return PreparationResult::Pending;
    }
    default:return PreparationResult::Failed;
    }
}
