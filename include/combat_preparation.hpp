#pragma once
#include <stdint.h>
namespace spire {
// Bounded preparation of the current battle's decoded working set. The
// presentation clock is paused while one cold image is decoded per update.
// Counters distinguish actual source work from the visible progress bar.
struct CombatPreparation {
    static constexpr unsigned Capacity=288,PageBudget=640,FirstCapacity=28;
    uint32_t images[Capacity]{},first[FirstCapacity]{};
    uint32_t seed=0,read_calls=0;
    uint64_t read_bytes=0;
    int floor=-1,combat=-1,encounter=-1;
    unsigned count=0,cursor=0,pages=0,first_count=0,cold_images=0,updates=0;
    bool initialized=false,active=false,shown=false,bounded=false;
    void reset(){*this=CombatPreparation{};}
};
}
