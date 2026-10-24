// The calculator file can wrap the unchanged canonical save in a small
// zero/literal stream. NSZ1 is only a host storage envelope: save()/load(),
// migration, revision, RNG and backup identities remain canonical NSV1.
// A size pass chooses this envelope only when it is strictly smaller. The
// fallback is the original raw file, so incompressible input never grows.
namespace run_storage {
constexpr unsigned HeaderBytes=8,BlockBytes=512,PacketBytes=128;
struct Packet {unsigned count;bool zero;};
static Packet packet(const uint8_t*data,unsigned size,unsigned at){
    if(at+1<size&&!data[at]&&!data[at+1]){
        unsigned count=2;while(count<PacketBytes&&at+count<size&&!data[at+count])++count;
        return {count,true};
    }
    unsigned count=1;
    while(count<PacketBytes&&at+count<size){
        if(at+count+1<size&&!data[at+count]&&!data[at+count+1])break;
        ++count;
    }
    return {count,false};
}
static unsigned encoded_size(const uint8_t*data,unsigned size){
    unsigned result=HeaderBytes;
    for(unsigned at=0;at<size;){const auto p=packet(data,size,at);result+=1+(p.zero?0:p.count);at+=p.count;}
    return result;
}
struct Writer {
    void*file;uint8_t bytes[BlockBytes];unsigned used=0;bool good=true;
    explicit Writer(void*f):file(f){}
    bool flush(){if(used){good=nd_write(bytes,1,used,file)==used;used=0;}return good;}
    bool byte(uint8_t value){if(!good)return false;bytes[used++]=value;return used<BlockBytes||flush();}
};
static bool write(void*file,const uint8_t*data,unsigned size){
    if(encoded_size(data,size)>=size)return nd_write(data,1,size,file)==size;
    Writer out(file);
    static constexpr char magic[]="NSZ1";
    for(unsigned i=0;i<4;++i)if(!out.byte(uint8_t(magic[i])))return false;
    for(unsigned i=0;i<4;++i)if(!out.byte(uint8_t(size>>(i*8))))return false;
    for(unsigned at=0;at<size;){
        const auto p=packet(data,size,at);if(!out.byte(uint8_t((p.zero?128:0)|(p.count-1))))return false;
        if(!p.zero)for(unsigned i=0;i<p.count;++i)if(!out.byte(data[at+i]))return false;
        at+=p.count;
    }
    return out.flush();
}
struct Reader {
    void*file;uint8_t bytes[BlockBytes];unsigned at=0,count=0;
    explicit Reader(void*f):file(f){}
    bool byte(uint8_t&value){
        if(at==count){count=nd_read(bytes,1,BlockBytes,file);at=0;if(!count)return false;}
        value=bytes[at++];return true;
    }
    bool end(){uint8_t extra=0;return !byte(extra);}
};
static bool read(void*file,spire::SaveData&out){
    out.size=0;uint8_t header[HeaderBytes];
    if(nd_read(header,1,sizeof header,file)!=sizeof header)return false;
    const bool packed=header[0]=='N'&&header[1]=='S'&&header[2]=='Z'&&header[3]=='1';
    if(!packed){
        for(unsigned i=0;i<sizeof header;++i)out.bytes[i]=header[i];
        const unsigned n=nd_read(out.bytes+sizeof header,1,sizeof out.bytes-sizeof header,file);
        uint8_t extra=0;if(nd_read(&extra,1,1,file)!=0)return false;
        out.size=n+sizeof header;return true;
    }
    uint32_t decoded=0;for(unsigned i=0;i<4;++i)decoded|=uint32_t(header[4+i])<<(i*8);
    if(decoded<16||decoded>sizeof out.bytes||(decoded&3u))return false;
    Reader input(file);unsigned at=0;
    while(at<decoded){
        uint8_t control;if(!input.byte(control))return false;
        const unsigned count=(control&127u)+1;
        if(count>decoded-at)return false;
        if(control&128u)for(unsigned i=0;i<count;++i)out.bytes[at++]=0;
        else for(unsigned i=0;i<count;++i)if(!input.byte(out.bytes[at++]))return false;
    }
    if(!input.end())return false;
    out.size=decoded;return true;
}
} // namespace run_storage

static bool save_game(const spire::State&s){
    spire::ResourceTransferLease transfer(spire::ResourceTransferUse::Save);if(!transfer)return false;
    auto&scratch=*transfer.save();if(!spire::save(s,scratch))return false;
    void*f=nd_open(temp_path,"wb");if(!f)return false;
    bool good=run_storage::write(f,scratch.bytes,unsigned(scratch.size));
    good=(nd_close(f)==0)&&good;if(!good){nd_unlink(temp_path);return false;}
    const int old=profile_file_present(save_path);
    if(old<0){nd_unlink(temp_path);return false;}
    if(old){
        if(!remove_profile_file(backup_path)||nd_rename(save_path,backup_path)!=0){nd_unlink(temp_path);return false;}
    }
    if(nd_rename(temp_path,save_path)!=0){
        if(old)nd_rename(backup_path,save_path);
        nd_unlink(temp_path);return false;
    }
    // Unlike optional UI history, the preceding run remains recoverable.
    return true;
}
static bool load_game(spire::State&s){
    spire::ResourceTransferLease transfer(spire::ResourceTransferUse::Save);if(!transfer)return false;
    void*f=nd_open(save_path,"rb");if(!f)return false;
    auto&scratch=*transfer.save();const bool read=run_storage::read(f,scratch);
    const bool closed=nd_close(f)==0;
    // Keep the existing primary-only policy. A partial decode or close error
    // never reaches the transactional canonical loader or publishes a State.
    return read&&closed&&spire::load(s,scratch.bytes,scratch.size);
}
