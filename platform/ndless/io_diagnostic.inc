// Optional developer-only OS I/O trace. The ordinary launcher compiles this
// entire recorder away; a trace launcher is never a timing acceptance result.
#if defined(SPIRE_DEVICE_IO_TRACE)
namespace {
struct DeviceIoRecord {
    uint32_t started,ticks,offset,count,result;
    uint8_t operation,stage,file,flags;
};
struct DeviceIoFile {void*handle;uint32_t position;uint8_t id;bool known;};
static DeviceIoRecord device_io_records[2048];
static DeviceIoFile device_io_files[16];
static uint32_t device_io_count=0,device_io_dropped=0;
static uint8_t device_io_stage=0;
static bool device_io_active=false;
static bool device_io_equal(const char*a,const char*b){
    while(*a&&*a==*b){++a;++b;}return *a==*b;
}
static uint8_t device_io_name(const char*path){
    const char*base=path;for(const char*p=path;*p;++p)if(*p=='/')base=p+1;
    if(device_io_equal(base,spire::SharedDataFilename))return 0;
    for(unsigned i=0;i<spire::MultipartResourceSource::MaxParts;++i)
        if(device_io_equal(base,spire::ArtworkPartNames[i]))return uint8_t(i+1);
    for(unsigned i=1;i<=spire::LocaleLanguageCount;++i)
        if(device_io_equal(base,spire::locale_language_info(i).file))return uint8_t(8+i);
    return 25;
}
static DeviceIoFile* device_io_file(void*file){
    if(file)for(auto&item:device_io_files)if(item.handle==file)return &item;
    return nullptr;
}
static void device_io_add(unsigned operation,unsigned id,uint32_t offset,uint32_t count,
                          uint32_t result,uint32_t started,unsigned flags=0){
    if(!device_io_active)return;
    if(device_io_count>=sizeof device_io_records/sizeof device_io_records[0]){++device_io_dropped;return;}
    device_io_records[device_io_count++]={started,started-timer_ticks(),offset,count,result,
        uint8_t(operation),device_io_stage,uint8_t(id),uint8_t(flags)};
}
static void* device_io_open(const char*path,const char*mode){
    const uint32_t started=device_io_active?timer_ticks():0;
    void*file=nd_open(path,mode);
    const uint8_t id=device_io_active?device_io_name(path):255;
    device_io_add(1,id,0,0,file!=nullptr,started);
    if(device_io_active&&file){
        for(auto&item:device_io_files)if(!item.handle||item.handle==file){
            item={file,0,id,true};break;
        }
    }
    return file;
}
static int device_io_close(void*file){
    DeviceIoFile*item=device_io_file(file);const unsigned id=item?item->id:255;
    const uint32_t started=device_io_active?timer_ticks():0;
    const int result=nd_close(file);device_io_add(2,id,0,0,uint32_t(result),started);
    if(item)*item={};return result;
}
static int device_io_seek(void*file,long offset,int origin){
    DeviceIoFile*item=device_io_file(file);const unsigned id=item?item->id:255;
    const uint32_t started=device_io_active?timer_ticks():0;
    const int result=nd_seek(file,offset,origin);
    device_io_add(3,id,uint32_t(offset),0,uint32_t(result),started,unsigned(origin));
    if(item){
        if(!result&&origin==0){item->position=uint32_t(offset);item->known=true;}
        else if(!result&&origin==1&&item->known)item->position+=uint32_t(offset);
        else item->known=false;
    }
    return result;
}
static long device_io_tell(void*file){
    DeviceIoFile*item=device_io_file(file);const unsigned id=item?item->id:255;
    const uint32_t started=device_io_active?timer_ticks():0;
    const long result=nd_tell(file);device_io_add(4,id,0,0,uint32_t(result),started);
    if(item){item->known=result>=0;item->position=uint32_t(result);}
    return result;
}
static unsigned device_io_read(void*output,unsigned size,unsigned count,void*file){
    DeviceIoFile*item=device_io_file(file);const unsigned id=item?item->id:255;
    const uint32_t at=item&&item->known?item->position:UINT32_MAX;
    const uint32_t started=device_io_active?timer_ticks():0;
    const unsigned result=nd_read(output,size,count,file);
    device_io_add(5,id,at,size*count,size*result,started);
    if(item&&item->known)item->position+=size*result;
    return result;
}
static int device_io_stat(const char*path,spire::CalculatorFileMetadata*metadata){
    const uint32_t started=device_io_active?timer_ticks():0;
    const int result=nd_stat(path,metadata);
    device_io_add(8,device_io_active?device_io_name(path):255,0,0,
                  result<0?UINT32_MAX:metadata->size,started);
    return result;
}
static void device_trace_stage(unsigned value){
    if(device_io_stage==value)return;
    device_io_stage=uint8_t(value);const uint32_t now=device_io_active?timer_ticks():0;
    device_io_add(6,value,0,0,0,now);
}
static void device_trace_begin(bool enabled){device_io_active=enabled;}
static void device_trace_resource(void*file,uint32_t at,uint32_t count,uint32_t started,bool good){
    DeviceIoFile*item=device_io_file(file);device_io_add(7,item?item->id:255,at,count,good,started);
}
static void device_trace_flush(const char*launcher){
    if(!device_io_active)return;device_io_active=false;
    char path[512];
    if(!spire::installation_path(path,sizeof path,launcher,spire::InstallationDirectory::State,"nSpire2.io.tns"))return;
    void*file=nd_open(path,"wb");if(!file)return;
    const uint32_t header[]={0x314f494eu,1,sizeof(DeviceIoRecord),device_io_count,device_io_dropped,32768,0,0};
    bool good=nd_write(header,1,sizeof header,file)==sizeof header;
    const unsigned bytes=device_io_count*sizeof(DeviceIoRecord);
    if(good)good=nd_write(device_io_records,1,bytes,file)==bytes;
    good=nd_close(file)==0&&good;if(!good)nd_unlink(path);
}
}
#define nd_open device_io_open
#define nd_close device_io_close
#define nd_seek device_io_seek
#define nd_tell device_io_tell
#define nd_read device_io_read
#define nd_stat device_io_stat
#else
static inline void device_trace_stage(unsigned){}
static inline void device_trace_begin(bool){}
static inline void device_trace_resource(void*,uint32_t,uint32_t,uint32_t,bool){}
static inline void device_trace_flush(const char*){}
#endif
