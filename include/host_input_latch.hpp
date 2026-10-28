#pragma once
#include <stdint.h>
#include "calculator_pad_input.hpp"

namespace spire {
// Resource reads and CPU rendering can outlast a tap. Remember matrix
// changes between updates without dispatching App callbacks from that work.
// The bounded FIFO preserves press/release and text order. In overload the
// newest entry becomes the actual final state, so no key remains held.
struct CalculatorInputLatch {
    static constexpr unsigned Capacity=32;
    uint16_t observed[8]{},samples[Capacity][8]{};
    uint16_t observed_pad=0,pad_samples[Capacity]{};
    TouchSample observed_touch{},touch_samples[Capacity]{};
    uint32_t sample_times[Capacity]{};
    uint8_t head=0,count=0;
    bool enabled=false;
    void reset(const uint16_t*matrix,bool enable,uint16_t pad=0,TouchSample touch={}){
        for(unsigned i=0;i<8;++i)observed[i]=matrix[i];
        observed_pad=pad;observed_touch=touch;head=count=0;enabled=enable;
    }
    static bool same_touch(const TouchSample&a,const TouchSample&b){
        return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height&&
            a.contact==b.contact&&a.pressed==b.pressed&&a.valid==b.valid;
    }
    void sample(const uint16_t*matrix,uint16_t pad=0,TouchSample touch={},uint32_t milliseconds=0){
        if(!enabled)return;
        bool changed=pad!=observed_pad||!same_touch(touch,observed_touch);for(unsigned i=0;i<8;++i)changed|=matrix[i]!=observed[i];
        if(!changed)return;
        const unsigned slot=(head+(count<Capacity?count:Capacity-1))%Capacity;
        for(unsigned i=0;i<8;++i)observed[i]=samples[slot][i]=matrix[i];
        observed_pad=pad_samples[slot]=pad;
        observed_touch=touch_samples[slot]=touch;sample_times[slot]=milliseconds;
        if(count<Capacity)++count;
    }
    bool pop(uint16_t*matrix,uint16_t*pad=nullptr,TouchSample*touch=nullptr,uint32_t*milliseconds=nullptr){
        if(!count)return false;
        for(unsigned i=0;i<8;++i)matrix[i]=samples[head][i];
        if(pad)*pad=pad_samples[head];
        if(touch)*touch=touch_samples[head];if(milliseconds)*milliseconds=sample_times[head];
        head=(head+1)%Capacity;--count;return true;
    }
    bool pending_pad()const{
        for(unsigned i=0;i<count;++i)if(pad_samples[(head+i)%Capacity])return true;
        return false;
    }
};
static_assert(sizeof(CalculatorInputLatch)<=1152,"Keys and complete touch gestures share one bounded32-entry FIFO");

// The calculator's read-only32768Hz down-counter throttles the complete
// matrix+pad sample, including many cheap rendering boundaries in one frame.
// Ordinary main-loop samples mark this same clock; wrap uses unsigned math.
struct CalculatorForegroundPoll {
    static constexpr uint32_t Interval=263; // ceil(8ms *32768Hz)
    uint32_t sampled_ticks=0;
    uint32_t milliseconds=0;
    uint16_t fraction=0;
    bool sampled=false;
    void mark(uint32_t now){
        if(sampled){const uint64_t scaled=uint64_t(sampled_ticks-now)*1000u+fraction;
            milliseconds+=uint32_t(scaled>>15);fraction=uint16_t(scaled&32767u);}
        sampled_ticks=now;sampled=true;
    }
    bool due(uint32_t now,bool enabled){
        if(!enabled||(sampled&&sampled_ticks-now<Interval))return false;
        mark(now);return true;
    }
};

// Replay complete raw samples only in the ordinary foreground input loop.
// A new central contact's anchor must precede its motion and click, even if
// the entire gesture arrived during one cold draw/decode. Side clicks keep
// their original owner and never become pointer drags or clear a chosen card.
template<class App,class Matrix>
void dispatch_calculator_sample(CalculatorPadInput&keys,TouchpadCursor&cursor,
        TouchSample sample,uint16_t pad,uint32_t milliseconds,App&app,
        bool allow_repeat,Matrix dispatch_matrix){
    const bool navigation=keys.dispatched||pad;
    if(navigation){
        app.pointer_cancel(true);cursor.reset(app.pointer.x,app.pointer.y);
        // Prime the actual release baseline before a later queued contact.
        cursor.sample(sample,milliseconds);
    }else{
        const auto pointer=cursor.sample(sample,milliseconds);
        if(pointer.cancelled)app.pointer_cancel();
        if(pointer.moved||pointer.down||pointer.up||pointer.tap)app.pointer_move(pointer.x,pointer.y);
        if(pointer.down||pointer.tap)app.pointer_button(true);
        if(pointer.up||pointer.tap)app.pointer_button(false);
    }
    keys.dispatch_key(pad,milliseconds,app,allow_repeat);dispatch_matrix(allow_repeat);
}

// Raw motion can keep the FIFO busy while a key remains held. Repeat once
// per foreground frame, never once per historical sample. A newly dispatched
// press cannot also repeat in that frame, and any input captured by an App
// callback must be dispatched before a previously held state repeats again.
template<class App,class Matrix>
void repeat_calculator_frame(const CalculatorInputLatch&latch,CalculatorPadInput&keys,
        uint32_t milliseconds,App&app,bool pad_changed,bool matrix_changed,Matrix dispatch_matrix){
    if(latch.count)return;
    if(!pad_changed)keys.dispatch_key(keys.dispatched,milliseconds,app,true);
    if(!latch.count&&!matrix_changed)dispatch_matrix(true);
}

// Poll around bounded reads, including a short final read. Startup callers
// can keep their larger sequential reads until foreground input is enabled.
template<class Read,class Poll>
unsigned sampled_resource_read(void*output,unsigned count,void*file,Read read,Poll poll){
    auto*bytes=static_cast<uint8_t*>(output);unsigned done=0;poll();
    while(done<count){
        const unsigned remaining=count-done,chunk=remaining<8192?remaining:8192;
        const unsigned got=read(bytes+done,1,chunk,file);poll();
        if(got>chunk)return done;
        done+=got;if(got!=chunk)break;
    }
    return done;
}
} // namespace spire
