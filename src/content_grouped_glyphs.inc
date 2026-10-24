// Cold grouped glyphs borrow the existing transfer and decoded-group pools.
// Each requested slice is copied into the ordinary stable glyph cache before
// returning; a later image decode cannot invalidate that Glyph pixel view.
struct GroupedGlyphRead {
    const Content* owner;
    const ResourceReader* reader;
    uint8_t* encoded;
};
static bool read_grouped_glyph(const void* context,uint32_t offset,void* output,unsigned length){
    const auto&read=*static_cast<const GroupedGlyphRead*>(context);
    uint8_t descriptor[GroupedGlyphDescriptorBytes];
    if(!read.reader->read(offset,descriptor,sizeof descriptor)||u32(descriptor+12)!=length)return false;
    const int64_t group_at=int64_t(offset)+int64_t(int32_t(u32(descriptor)));
    const uint32_t group_size=u32(descriptor+4),raw_at=u32(descriptor+8);
    if(group_at<0||uint64_t(group_at)>UINT32_MAX||group_size<33||group_size>ResourceTransferCapacity||
       !read.reader->contains(uint32_t(group_at),group_size)||
       !(uint64_t(group_at)+group_size<=offset||uint64_t(offset)+sizeof descriptor<=uint64_t(group_at)))return false;
    TileGroup* group=load_retained_group(read.owner,*read.reader,uint32_t(group_at),group_size,read.encoded);
    if(!group||group->size>GroupedGlyphRawLimit||raw_at>group->size||length>group->size-raw_at)return false;
    auto*out=static_cast<uint8_t*>(output);const uint8_t*source=tile_bytes(*group)+raw_at;
    for(unsigned i=0;i<length;++i)out[i]=source[i];
    return true;
}
