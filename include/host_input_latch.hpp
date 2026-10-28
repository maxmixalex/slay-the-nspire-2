#pragma once
#include <stdint.h>

namespace spire {
// Resource reads and CPU rendering can outlast a tap. Remember matrix
// changes between updates without dispatching App callbacks from that work.
// The bounded FIFO preserves press/release and text order. In overload the
// newest entry becomes the actual final state, so no key remains held.
struct CalculatorInputLatch {
    static constexpr unsigned Capacity=32;
    uint16_t observed[8]{},samples[Capacity][8]{};
    uint16_t observed_pad=0,pad_samples[Capacity]{};
    uint8_t head=0,count=0;
    bool enabled=false;
    void reset(const uint16_t*matrix,bool enable,uint16_t pad=0){
        for(unsigned i=0;i<8;++i)observed[i]=matrix[i];
        observed_pad=pad;head=count=0;enabled=enable;
    }
    void sample(const uint16_t*matrix,uint16_t pad=0){
        if(!enabled)return;
        bool changed=pad!=observed_pad;for(unsigned i=0;i<8;++i)changed|=matrix[i]!=observed[i];
        if(!changed)return;
        const unsigned slot=(head+(count<Capacity?count:Capacity-1))%Capacity;
        for(unsigned i=0;i<8;++i)observed[i]=samples[slot][i]=matrix[i];
        observed_pad=pad_samples[slot]=pad;
        if(count<Capacity)++count;
    }
    bool pop(uint16_t*matrix,uint16_t*pad=nullptr){
        if(!count)return false;
        for(unsigned i=0;i<8;++i)matrix[i]=samples[head][i];
        if(pad)*pad=pad_samples[head];
        head=(head+1)%Capacity;--count;return true;
    }
    bool pending_pad()const{
        for(unsigned i=0;i<count;++i)if(pad_samples[(head+i)%Capacity])return true;
        return false;
    }
};
static_assert(sizeof(CalculatorInputLatch)<616,"Matrix and pad latching share one bounded FIFO");

// The calculator's read-only32768Hz down-counter throttles the complete
// matrix+pad sample, including many cheap rendering boundaries in one frame.
// Ordinary main-loop samples mark this same clock; wrap uses unsigned math.
struct CalculatorForegroundPoll {
    static constexpr uint32_t Interval=263; // ceil(8ms *32768Hz)
    uint32_t sampled_ticks=0;
    bool sampled=false;
    void mark(uint32_t now){sampled_ticks=now;sampled=true;}
    bool due(uint32_t now,bool enabled){
        if(!enabled||(sampled&&sampled_ticks-now<Interval))return false;
        mark(now);return true;
    }
};

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
