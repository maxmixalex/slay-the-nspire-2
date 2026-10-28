// String views must outlive eviction of the shared decoded animation/font
// group. Copy each demanded UTF-8 slice into the existing stable string cache.
struct GroupedStringRead {
    const Content* owner;
    const ResourceReader* reader;
    uint8_t* encoded;
};
static bool read_grouped_string(const void* context,uint32_t offset,void* output,unsigned length){
    const auto&read=*static_cast<const GroupedStringRead*>(context);
    uint8_t descriptor[GroupedStringDescriptorBytes];
    if(!read.reader->read(offset,descriptor,sizeof descriptor)||u32(descriptor+12)!=length)return false;
    const int64_t group_at=int64_t(offset)+int64_t(int32_t(u32(descriptor)));
    const uint32_t group_size=u32(descriptor+4),raw_at=u32(descriptor+8);
    if(group_at<0||uint64_t(group_at)>UINT32_MAX||group_size<33||group_size>ResourceTransferCapacity||
       !read.reader->contains(uint32_t(group_at),group_size)||
       !(uint64_t(group_at)+group_size<=offset||uint64_t(offset)+sizeof descriptor<=uint64_t(group_at)))return false;
    // Check the small-string bound before allocating or decoding a group;
    // this also rejects a malformed warm alias with a different group size.
    uint8_t header[32];
    if(!read.reader->read(uint32_t(group_at),header,sizeof header)||u32(header)!=0x31524752u||
       !u32(header+4)||u32(header+4)>GroupedStringRawLimit||u32(header+8)!=group_size-32||
       u32(header+12)!=14||!u32(header+16)||u32(header+16)>16384||
       u32(header+20)||u32(header+24)||u32(header+28))return false;
    TileGroup* group=load_retained_group(read.owner,*read.reader,uint32_t(group_at),group_size,read.encoded);
    if(!group||group->size>GroupedStringRawLimit||raw_at>group->size||length>group->size-raw_at)return false;
    auto*out=static_cast<uint8_t*>(output);const uint8_t*source=tile_bytes(*group)+raw_at;
    for(unsigned i=0;i<length;++i)out[i]=source[i];
    return true;
}
