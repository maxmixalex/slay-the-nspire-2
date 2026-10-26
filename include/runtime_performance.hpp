#pragma once
#include <stddef.h>
#include <stdint.h>
#include "resource_source.hpp"
#include "runtime_render_parts.hpp"

namespace spire {

// Optional foreground diagnostics. Storage is fixed, and the hot path never
// formats text, allocates memory, writes files, or retains individual frames.
// Platform clocks supply unsigned deltas so one 32-bit counter wrap is safe.
struct RuntimeTiming {
    uint32_t count=0,minimum=0,maximum=0;
    uint64_t total=0;
    void add(uint32_t ticks) {
        if(count==UINT32_MAX)return;
        if(!count||ticks<minimum)minimum=ticks;
        if(ticks>maximum)maximum=ticks;
        ++count;total+=ticks;
    }
};
enum class RuntimeScene : uint8_t {
    Title,Character,Collection,Map,Combat,Room,Overlay,Other,CombatPreparation,Count
};
struct RuntimeFrameTiming {
    RuntimeTiming interval,update,render,present,resource;
    uint32_t over_50ms=0,over_100ms=0,over_250ms=0;
    uint64_t read_calls=0,read_bytes=0,read_seeks=0;
};
struct RuntimeResourceTiming {uint64_t ticks=0,calls=0,bytes=0,failures=0,seeks=0;};
struct RuntimePerformance {
    bool enabled=false;
    uint32_t clock_hz=32768,pack_bytes=0,hardware=0,ndless_revision=0;
    RuntimeTiming startup,first_render,first_present;
    RuntimeTiming startup_phases[(unsigned)ResourceStage::Count]{};
    RuntimeResourceTiming resources{};
    RuntimeFrameTiming scenes[(unsigned)RuntimeScene::Count]{};
    RuntimeTiming render_parts[(unsigned)RuntimeRenderPart::Count]{};
    uint32_t render_started[(unsigned)RuntimeRenderPart::Count]{};
    uint8_t render_active=0;

    void render_part(RuntimeRenderPart part,bool begin,uint32_t down_counter) {
        if(!enabled)return;
        const unsigned index=unsigned(part);
        if(index>=unsigned(RuntimeRenderPart::Count))return;
        const uint8_t bit=uint8_t(1u<<index);
        if(begin){
            if(render_active&bit)return;
            render_started[index]=down_counter;render_active|=bit;
        }else if(render_active&bit){
            render_parts[index].add(render_started[index]-down_counter);
            render_active&=uint8_t(~bit);
        }
    }

    void read(uint32_t ticks,uint32_t bytes,bool success,bool seek=false) {
        if(!enabled)return;
        resources.ticks+=ticks;++resources.calls;resources.bytes+=bytes;
        if(!success)++resources.failures;
        if(seek)++resources.seeks;
    }
    void frame(RuntimeScene scene,uint32_t interval,uint32_t update,
               uint32_t render,uint32_t present,const RuntimeResourceTiming&before) {
        if(!enabled)return;
        unsigned index=(unsigned)scene;if(index>=(unsigned)RuntimeScene::Count)index=(unsigned)RuntimeScene::Other;
        auto&s=scenes[index];
        if(s.interval.count==UINT32_MAX)return;
        s.interval.add(interval);s.update.add(update);s.render.add(render);s.present.add(present);
        uint64_t io=resources.ticks-before.ticks;
        s.resource.add(io>UINT32_MAX?UINT32_MAX:(uint32_t)io);
        s.read_calls+=resources.calls-before.calls;s.read_bytes+=resources.bytes-before.bytes;
        s.read_seeks+=resources.seeks-before.seeks;
        if((uint64_t)interval*1000u>(uint64_t)clock_hz*50u)++s.over_50ms;
        if((uint64_t)interval*1000u>(uint64_t)clock_hz*100u)++s.over_100ms;
        if((uint64_t)interval*1000u>(uint64_t)clock_hz*250u)++s.over_250ms;
    }

    // Emit one bounded JSON report on explicit diagnostic exit. The sink is
    // buffered here to avoid many small calculator filesystem writes. Failure
    // stops further writes; reporting must not alter game or save state.
    bool write(bool(*sink)(void*,const char*,size_t),void*context)const {
        struct Writer {
            bool(*sink)(void*,const char*,size_t);void*context;char buffer[256]{};unsigned used=0;bool good=true;
            void flush(){if(good&&used){good=sink(context,buffer,used);used=0;}}
            void ch(char c){if(!good)return;buffer[used++]=c;if(used==sizeof buffer)flush();}
            void text(const char*s){while(*s&&good)ch(*s++);}
            void number(uint64_t n){char digits[20];unsigned count=0;do{digits[count++]=(char)('0'+n%10u);n/=10u;}while(n);while(count)ch(digits[--count]);}
            void field(const char*key,uint64_t value){ch('"');text(key);text("\":");number(value);}
            void timing(const char*key,const RuntimeTiming&t){ch('"');text(key);text("\":{");field("count",t.count);ch(',');field("total",t.total);ch(',');field("min",t.minimum);ch(',');field("max",t.maximum);ch('}');}
        } w{sink,context};
        if(!sink)return false;
        w.text("{\"format\":\"nspire-runtime-perf-1\",\"units\":\"timer_ticks\",\"clock\":\"foreground_SP804\",\"startup_scope\":\"selected_resource_validation_and_app_init\",");
        w.field("clock_hz",clock_hz);w.ch(',');w.field("hardware",hardware);w.ch(',');w.field("ndless_revision",ndless_revision);w.ch(',');w.field("pack_bytes",pack_bytes);w.ch(',');
        w.timing("startup",startup);w.ch(',');w.timing("first_render",first_render);w.ch(',');w.timing("first_present",first_present);
        w.text(",\"startup_phases\":{");
        static const char*const phase_names[]={"header","integrity","directory","images","tile_index","glyphs","strings","ready"};
        for(unsigned i=0;i<(unsigned)ResourceStage::Count;++i){if(i)w.ch(',');w.timing(phase_names[i],startup_phases[i]);}
        w.ch('}');
        w.text(",\"render_parts\":{");
        static const char*const render_names[]={"random_water","random_light","random_dots"};
        for(unsigned i=0;i<unsigned(RuntimeRenderPart::Count);++i){if(i)w.ch(',');w.timing(render_names[i],render_parts[i]);}
        w.ch('}');
        w.text(",\"resource_total\":{");w.field("ticks",resources.ticks);w.ch(',');w.field("calls",resources.calls);w.ch(',');w.field("bytes_requested",resources.bytes);w.ch(',');w.field("failures",resources.failures);w.ch(',');w.field("seeks",resources.seeks);w.text("},\"scenes\":[");
        static const char*const names[]={"title","character","collection","map","combat","room","overlay","other","combat_preparation"};
        for(unsigned i=0;i<(unsigned)RuntimeScene::Count;++i){
            if(i)w.ch(',');const auto&s=scenes[i];w.text("{\"scene\":\"");w.text(names[i]);w.text("\",");
            w.timing("interval",s.interval);w.ch(',');w.timing("update",s.update);w.ch(',');w.timing("render",s.render);w.ch(',');w.timing("present",s.present);w.ch(',');w.timing("resource",s.resource);
            w.ch(',');w.field("over_50ms",s.over_50ms);w.ch(',');w.field("over_100ms",s.over_100ms);w.ch(',');w.field("over_250ms",s.over_250ms);w.ch(',');w.field("read_calls",s.read_calls);w.ch(',');w.field("read_bytes_requested",s.read_bytes);w.ch(',');w.field("read_seeks",s.read_seeks);w.ch('}');
        }
        w.text("]}\n");w.flush();return w.good;
    }
};
static_assert(sizeof(RuntimePerformance)<=2048,"Device diagnostics must use at most 2 KiB of fixed storage");
} // namespace spire
