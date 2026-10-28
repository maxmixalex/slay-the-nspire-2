#pragma once
// Shared foreground file envelope. The canonical run/history codecs retain
// their own magic, schema and validation; this layer only preserves bytes.
// The host provides nd_read/nd_write before including this implementation.
namespace storage_codec {
constexpr unsigned HeaderBytes=8,BlockBytes=512,PacketBytes=128;
enum class ReadStatus:uint8_t { Complete,Invalid,IoFailure };
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
    void*file;uint8_t bytes[BlockBytes];unsigned at=0,count=0,remaining=0;
    bool bounded=false,io_failure=false;
    explicit Reader(void*f,unsigned physical=0):file(f),remaining(physical?physical-HeaderBytes:0),bounded(physical!=0){}
    bool byte(uint8_t&value){
        if(at==count){
            if(bounded&&!remaining)return false;
            const unsigned wanted=bounded&&remaining<BlockBytes?remaining:BlockBytes;
            count=nd_read(bytes,1,wanted,file);at=0;
            if(count>wanted||(bounded&&count!=wanted)){io_failure=true;return false;}
            if(bounded)remaining-=count;if(!count)return false;
        }
        value=bytes[at++];return true;
    }
    bool end(){uint8_t extra=0;return !byte(extra);}
};
static bool read(void*file,uint8_t*output,unsigned capacity,unsigned&size,unsigned minimum=HeaderBytes,bool aligned=false,
                 ReadStatus*status=nullptr,unsigned physical=0){
    size=0;
    const auto fail=[&](ReadStatus why){if(status)*status=why;return false;};
    if(!output||minimum<HeaderBytes||minimum>capacity||(physical&&physical<HeaderBytes))return fail(ReadStatus::Invalid);
    uint8_t header[HeaderBytes];if(nd_read(header,1,sizeof header,file)!=sizeof header)return fail(ReadStatus::IoFailure);
    const bool packed=header[0]=='N'&&header[1]=='S'&&header[2]=='Z'&&header[3]=='1';
    if(!packed){
        if(physical>capacity)return fail(ReadStatus::Invalid);
        for(unsigned i=0;i<sizeof header;++i)output[i]=header[i];
        const unsigned wanted=(physical?physical:capacity)-sizeof header;
        const unsigned n=nd_read(output+sizeof header,1,wanted,file);
        if(n>wanted||(physical&&n!=wanted))return fail(ReadStatus::IoFailure);
        uint8_t extra=0;if(nd_read(&extra,1,1,file)!=0)return fail(ReadStatus::Invalid);
        const unsigned total=n+sizeof header;
        if(total<minimum||(aligned&&(total&3u)))return fail(ReadStatus::Invalid);
        size=total;if(status)*status=ReadStatus::Complete;return true;
    }
    uint32_t decoded=0;for(unsigned i=0;i<4;++i)decoded|=uint32_t(header[4+i])<<(i*8);
    if(decoded<minimum||decoded>capacity||(aligned&&(decoded&3u)))return fail(ReadStatus::Invalid);
    Reader input(file,physical);unsigned at=0;
    while(at<decoded){
        uint8_t control;if(!input.byte(control))return fail(input.io_failure?ReadStatus::IoFailure:ReadStatus::Invalid);
        const unsigned count=(control&127u)+1;
        if(count>decoded-at)return fail(ReadStatus::Invalid);
        if(control&128u)for(unsigned i=0;i<count;++i)output[at++]=0;
        else for(unsigned i=0;i<count;++i)if(!input.byte(output[at++]))return fail(input.io_failure?ReadStatus::IoFailure:ReadStatus::Invalid);
    }
    if(!input.end())return fail(ReadStatus::Invalid);
    if(input.io_failure)return fail(ReadStatus::IoFailure);
    size=decoded;if(status)*status=ReadStatus::Complete;return true;
}
} // namespace storage_codec
